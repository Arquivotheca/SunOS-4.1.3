#ident "@(#)vm_pvn.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

/*
 * VM - paged vnode.
 *
 * This file supplies vm support for the vnode operations that deal with pages.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/vmmeter.h>
#include <sys/vmsystm.h>
#include <sys/mman.h>
#include <sys/vfs.h>
#include <sys/debug.h>
#include <sys/trace.h>
#include <sys/ucred.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/rm.h>
#include <vm/pvn.h>
#include <vm/page.h>
#include <vm/seg_map.h>

int pvn_nofodklust = 0;

/*
 * Find the largest contiguous block which contains `addr' for file offset
 * `offset' in it while living within the file system block sizes (`vp_off'
 * and `vp_len') and the address space limits for which no pages currently
 * exist and which map to consecutive file offsets.
 */
struct page *
pvn_kluster(vp, off, seg, addr, offp, lenp, vp_off, vp_len, isra)
	struct vnode *vp;
	register u_int off;
	register struct seg *seg;
	register addr_t addr;
	u_int *offp, *lenp;
	u_int vp_off, vp_len;
	int isra;
{
	register int delta, delta2;
	register struct page *pp;
	struct page *plist = NULL;
	addr_t straddr;
	int bytesavail;
	u_int vp_end;

	ASSERT(off >= vp_off && off < vp_off + vp_len);

	/*
	 * We only want to do klustering/read ahead if there
	 * is more than minfree pages currently available.
	 */
	if (freemem - minfree > 0)
		bytesavail = ptob(freemem - minfree);
	else
		bytesavail = 0;

	if (bytesavail == 0) {
		if (isra)
			return ((struct page *)NULL);	/* ra case - give up */
		else
			bytesavail = PAGESIZE;		/* just pretending */
	}

	if (bytesavail < vp_len) {
		/*
		 * Don't have enough free memory for the
		 * max request, try sizing down vp request.
		 */
		delta = off - vp_off;
		vp_len -= delta;
		vp_off += delta;
		if (bytesavail < vp_len) {
			/*
			 * Still not enough memory, just settle for
			 * bytesavail which is at least PAGESIZE.
			 */
			vp_len = bytesavail;
		}
	}

	vp_end = vp_off + vp_len;
	ASSERT(off >= vp_off && off < vp_end);

	if (page_exists(vp, off))
		return ((struct page *)NULL);		/* already have page */

	if (vp_len <= PAGESIZE || pvn_nofodklust) {
		straddr = addr;
		*offp = off;
		*lenp = MIN(vp_len, PAGESIZE);
	} else {
		/* scan forward from front */
		for (delta = 0; off + delta < vp_end; delta += PAGESIZE) {
			/*
			 * Call back to the segment driver to verify that
			 * the klustering/read ahead operation makes sense.
			 */
			if ((*seg->s_ops->kluster)(seg, addr, delta))
				break;		/* page not file extension */
			if (page_exists(vp, off + delta))
				break;		/* already have this page */
		}
		delta2 = delta;

		/* scan back from front */
		for (delta = 0; off + delta > vp_off; delta -= PAGESIZE) {
			if (page_exists(vp, off + delta - PAGESIZE))
				break;		/* already have the page */
			/*
			 * Call back to the segment driver to verify that
			 * the klustering/read ahead operation makes sense.
			 */
			if ((*seg->s_ops->kluster)(seg, addr, delta - PAGESIZE))
				break;		/* page not eligible */
		}

		straddr = addr + delta;
		*offp = off = off + delta;
		*lenp = MAX(delta2 - delta, PAGESIZE);
		ASSERT(off >= vp_off);

		if ((vp_off + vp_len) < (off + *lenp)) {
			ASSERT(vp_end > off);
			*lenp = vp_end - off;
		}
	}

	/*
	 * Allocate pages for <vp, off> at <seg, addr> for delta bytes.
	 * Note that for the non-read ahead case we might not have the
	 * memory available right now so that rm_allocpage operation could
	 * sleep and someone else might race to this same spot if the
	 * vnode object was not locked before this routine was called.
	 */
	delta2 = *lenp;
	delta = roundup(delta2, PAGESIZE);
	pp = rm_allocpage(seg, straddr, (u_int)delta, 1); /* `pp' list kept */

	plist = pp;
	do {
		pp->p_intrans = 1;
		pp->p_pagein = 1;

#ifdef TRACE
		{
			addr_t taddr = straddr + (off - *offp);

			trace3(TR_SEG_KLUSTER, seg, taddr, isra);
			trace6(TR_SEG_ALLOCPAGE, seg, taddr, TRC_SEG_UNK,
			    vp, off, pp);
		}
#endif TRACE
		if (page_enter(pp, vp, off)) {		/* `pp' locked if ok */
			/*
			 * Oops - somebody beat us to the punch
			 * and has entered the page before us.
			 * To recover, we use pvn_fail to free up
			 * all the pages we have already allocated
			 * and we return NULL so that whole operation
			 * is attempted over again.  This should never
			 * happen if the caller of pvn_kluster does
			 * vnode locking to prevent multiple processes
			 * from creating the same pages as the same time.
			 */
			pvn_fail(plist, B_READ);
			return ((struct page *)NULL);
		}
		off += PAGESIZE;
	} while ((pp = pp->p_next) != plist);

	return (plist);
}

/*
 * Entry point to be use by page r/w subr's and other such routines which
 * want to report an error and abort a list of pages setup for pageio
 * which do not do though the normal pvn_done processing.
 */
void
pvn_fail(plist, flags)
	struct page *plist;
	int flags;
{
	static struct buf abort_buf;
	struct buf *bp;
	struct page *pp;
	int len;
	int s;

	len = 0;
	pp = plist;
	do {
		len += PAGESIZE;
	} while ((pp = pp->p_next) != plist);

	bp = &abort_buf;
	s = splimp();
	while (bp->b_pages != NULL) {
		(void) sleep((caddr_t)&bp->b_pages, PSWP+2);
	}
	(void) splx(s);
	/* ~B_PAGEIO is a flag to pvn_done not to pageio_done the bp */
	bp->b_flags = B_ERROR | B_ASYNC | (flags & ~B_PAGEIO);
	bp->b_pages = plist;
	bp->b_bcount = len;
	pvn_done(bp);			/* let pvn_done do all the work */
	if (bp->b_pages != NULL) {
		/* XXX - this should never happen, should it be a panic? */
		bp->b_pages = NULL;
	}
	wakeup((caddr_t)&bp->b_pages);
}

/*
 * Routine to be called when pageio's complete.
 * Can only be called from process context, not
 * from interrupt level.
 */
void
pvn_done(bp)
	register struct buf *bp;
{
	register struct page *pp;
	register int bytes;

	pp = bp->b_pages;

	/*
	 * Release any I/O mappings to the pages described by the
	 * buffer that are finished before processing the completed I/O.
	 */
	if ((bp->b_flags & B_REMAPPED) && (pp->p_nio <= 1))
		bp_mapout(bp);

	/*
	 * Handle of each page in the I/O operation.
	 */
	for (bytes = 0; bytes < bp->b_bcount; bytes += PAGESIZE) {
		struct vnode *vp;
		u_int off;
		register int s;

		if (pp->p_nio > 1) {
			/*
			 * There were multiple IO requests outstanding
			 * for this particular page.  This can happen
			 * when the file system block size is smaller
			 * than PAGESIZE.  Since there are more IO
			 * requests still outstanding, we don't process
			 * the page given on the buffer now.
			 */
			if (bp->b_flags & B_ERROR) {
				if (bp->b_flags & B_READ) {
					trace3(TR_PG_PVN_DONE, pp, pp->p_vnode,
					    pp->p_offset);
					page_abort(pp);	/* assumes no waiting */
				} else {
					pg_setmod(pp, 1);
				}
			}
			pp->p_nio--;
			break;
			/* real page locked for the other io operations */
		}

		pp = bp->b_pages;
		page_sub(&bp->b_pages, pp);

		vp = pp->p_vnode;
		off = pp->p_offset;
		pp->p_intrans = 0;
		pp->p_pagein = 0;

		PAGE_RELE(pp);
		/*
		 * Verify the page identity before checking to see
		 * if the page was freed by PAGE_RELE().  This must
		 * be protected by splvm() to prevent the page from
		 * being ripped away at interrupt level.
		 */
		s = splvm();
		if (pp->p_vnode != vp || pp->p_offset != off || pp->p_free) {
			(void) splx(s);
			continue;
		}
		(void) splx(s);

		/*
		 * Check to see if the page has an error.
		 */
		if ((bp->b_flags & (B_ERROR|B_READ)) == (B_ERROR|B_READ)) {
			page_abort(pp);
			continue;
		}

		/*
		 * Check if we are to be doing invalidation.
		 * XXX - Failed writes with B_INVAL set are
		 * not handled appropriately.
		 */
		if ((bp->b_flags & B_INVAL) != 0) {
			page_abort(pp);
			continue;
		}

		if ((bp->b_flags & (B_ERROR | B_READ)) == B_ERROR) {
			/*
			 * Write operation failed.  We don't want
			 * to abort (or free) the page.  We set
			 * the mod bit again so it will get
			 * written back again later when things
			 * are hopefully better again.
			 */
			pg_setmod(pp, 1);
		}

		if (bp->b_flags & B_FREE) {
			cnt.v_pgpgout++;
			if (pp->p_keepcnt == 0 && pp->p_lckcnt == 0) {
				/*
				 * Check if someone has reclaimed the
				 * page.  If no ref or mod, no one is
				 * using it so we can free it.
				 * The rest of the system is careful
				 * to use the ghost unload flag to unload
				 * translations set up for IO w/o
				 * affecting ref and mod bits.
				 */
				if (pp->p_mod == 0 && pp->p_mapping)
					hat_pagesync(pp);
				if (!pp->p_ref && !pp->p_mod) {
					if (pp->p_mapping)
						hat_pageunload(pp);
#ifdef	MULTIPROCESSOR
				}
				/*
				 * The page may have been modified
				 * between the hat_pagesync and
				 * the hat_pageunload, and hat_pageunload
				 * will have picked up final ref and mod
				 * bits from the PTEs. So, check 'em again.
				 */
				if (!pp->p_ref && !pp->p_mod) {
#endif	MULTIPROCESSOR
					page_free(pp,
					    (int)(bp->b_flags & B_DONTNEED));
					if ((bp->b_flags & B_DONTNEED) == 0)
						cnt.v_dfree++;
				} else {
					page_unlock(pp);
					cnt.v_pgrec++;
				}
			} else {
				page_unlock(pp);
			}
			continue;
		}

		page_unlock(pp);		/* a read or write */
	}

	/*
	 * Count pageout operations if applicable.  Release the
	 * buf struct associated with the operation if async & pageio.
	 */
	if (bp->b_flags & B_FREE)
		cnt.v_pgout++;
	if ((bp->b_flags & (B_ASYNC | B_PAGEIO)) == (B_ASYNC | B_PAGEIO))
		pageio_done(bp);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_DELWRI}
 * B_DELWRI indicates that this page is part of a kluster operation and
 * is only to be considered if it doesn't involve any waiting here.
 * Returns non-zero if page added to dirty list.
 *
 * NOTE:  The caller must ensure that the page is not on the free list.
 */
static int
pvn_getdirty(pp, dirty, flags)
	register struct page *pp, **dirty;
	int flags;
{
	struct vnode *vp;
	u_int offset;

	ASSERT(pp->p_free == 0);
	vp = pp->p_vnode;
	offset = pp->p_offset;

	/*
	 * If page is logically locked, forget it.
	 *
	 * XXX - Can a page locked by some other process be
	 * written out or invalidated?
	 */
	if (pp->p_lckcnt != 0)
		return (0);

	if ((flags & B_DELWRI) != 0 && (pp->p_keepcnt != 0 || pp->p_lock)) {
		/*
		 * This is a klustering case that would
		 * cause us to block, just give up.
		 */
		return (0);
	}

	if (pp->p_intrans && (flags & (B_INVAL | B_ASYNC)) == B_ASYNC) {
		/*
		 * Don't bother waiting for an intrans page if we are not
		 * doing invalidation and this is an async operation
		 * (the page will be correct when the current io completes).
		 */
		return (0);
	}

	/*
	 * If i/o is in progress on the page or we have to
	 * invalidate or free the page, wait for the page keep
	 * count to go to zero.
	 */
	if (pp->p_intrans || (flags & (B_INVAL | B_FREE)) != 0) {
		if (pp->p_keepcnt != 0) {
			page_wait(pp);
			/*
			 * Re-verify page identity since it could have
			 * changed while we were sleeping.
			 */
			if (pp->p_vnode != vp || pp->p_offset != offset) {
				/*
				 * Lost the page - nothing to do?
				 */
				return (0);
			}
			/*
			 * The page has not lost its identity and hence
			 * should not be on the free list.
			 */
			ASSERT(pp->p_free == 0);
		}
	}

	page_lock(pp);

	/*
	 * If the page has mappings and it is not the case that the
	 * page is already marked dirty and we are going to unload
	 * the page below because we are going to free/invalidate
	 * it, then we sync current mod bits from the hat layer now.
	 */
	if (pp->p_mapping && !(pp->p_mod && (flags & (B_FREE | B_INVAL)) != 0))
		hat_pagesync(pp);

	if (pp->p_mod == 0) {
		if ((flags & (B_INVAL | B_FREE)) != 0) {
			if (pp->p_mapping)
				hat_pageunload(pp);
			if ((flags & B_INVAL) != 0) {
				page_abort(pp);
				return (0);
			}
			if (pp->p_free == 0) {
				if ((flags & B_FREE) != 0) {
					page_free(pp, (flags & B_DONTNEED));
					return (0);
				}
			}
		}
		page_unlock(pp);
		return (0);
	}

	/*
	 * Page is dirty, get it ready for the write back
	 * and add page to the dirty list.  First unload
	 * the page if we are going to free/invalidate it.
	 */
	if (pp->p_mapping && (flags & (B_FREE | B_INVAL)) != 0)
		hat_pageunload(pp);
	pg_setmod(pp, 0);
	pg_setref(pp, 0);
	trace3(TR_PG_PVN_GETDIRTY, pp, pp->p_vnode, pp->p_offset);
	pp->p_intrans = 1;
	/*
	 * XXX - The `p_pagein' bit is set for asynchronous or
	 * synchronous invalidates to prevent other processes
	 * from accessing the page in the window after the i/o is
	 * complete but before the page is aborted. If this is not
	 * done, updates to the page before it is aborted will be lost.
	 */
	pp->p_pagein = (flags & B_INVAL) ? 1 : 0;
	PAGE_HOLD(pp);
	page_sortadd(dirty, pp);
	return (1);
}

/*
 * Run down the vplist and handle all pages whose offset is >= off.
 * Returns a list of dirty kept pages all ready to be written back.
 *
 * Assumptions:
 *	The vp is already locked by the VOP_PUTPAGE routine calling this.
 *	That the VOP_GETPAGE also locks the vp, and thus no one can
 *	    add a page to the vp list while the vnode is locked.
 *	Flags are {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
struct page *
pvn_vplist_dirty(vp, off, flags)
	register struct vnode *vp;
	u_int off;
	int flags;
{
	register struct page *pp;
	register struct page *ppnext;
	register struct page *ppsav;
	register struct page *ppnextnext;
	register int ppsav_wasfree, pp_wasfree;
	register int ppsav_age, pp_age;
	struct page *dirty;
	register int s;
	int on_iolist;

	s = splvm();
	if (vp->v_type == VSOCK || vp->v_type == VCHR ||
	    (pp = vp->v_pages) == NULL) {
		(void) splx(s);
		return ((struct page *)NULL);
	}

#define	PAGE_RECLAIM(pp, wasfree, age) \
{ \
	if ((pp)->p_free) { \
		age = (pp)->p_age; \
		page_reclaim(pp); \
		wasfree = 1; \
	} else { \
		age = wasfree = 0; \
	} \
}
#define	PAGE_REFREE(pp, wasfree, age) \
{ \
	if (wasfree && (pp)->p_keepcnt == 0 && (pp)->p_mapping == NULL) \
		page_free(pp, age); \
}

	/*
	 * Traverse the page list.  We have to be careful since pages
	 * can be removed from the vplist while we are looking at it
	 * (a page being pulled off the free list for something else,
	 * or an async io operation completing and the page and/or
	 * bp is marked for invalidation) so have to be careful determining
	 * that we have examined all the pages.  We use ppsav to point
	 * to the first page that stayed on the vp list after calling
	 * pvn_getdirty and we PAGE_RECLAIM and PAGE_HOLD to prevent it
	 * from going away on us.  When we PAGE_UNKEEP the page, it will
	 * go back to the free list if that's where we got it from.  We
	 * also need to PAGE_RECLAIM and PAGE_HOLD the next pp in the
	 * vplist to prevent it from going away while we are traversing
	 * the list.
	 */

	ppnext = NULL;
	ppsav = NULL;
	ppsav_age = ppsav_wasfree = 0;
	pp_age = pp_wasfree = 0;

	dirty = NULL;
	if (pp->p_vpnext != pp)
		ppnext = pp->p_vpnext;
	else
		ppnext = NULL;

	for (;;) {
		/* Reclaim and hold the next page */
		if (ppnext != NULL) {
			if (ppnext->p_free)
				page_reclaim(ppnext);
			PAGE_HOLD(ppnext);
		}

		if (pp != NULL) {
			PAGE_RECLAIM(pp, pp_wasfree, pp_age);

			/* Process the current page */
			if (pp->p_offset >= off) {
				(void) splx(s);
				on_iolist = pvn_getdirty(pp, &dirty, flags);
				s = splvm();
			} else
				on_iolist = 0;

			if (pp->p_vnode == vp) {
				/*
				 * If the page identity hasn't changed and
				 * it isn't dirty, free it if reclaimed
				 * from the free list.
				 */
				if (!on_iolist && !pp->p_free)
					PAGE_REFREE(pp, pp_wasfree, pp_age);

				/*
				 * If we haven't found a marker before,
				 * use the current page as our marker.
				 */
				if (ppsav == NULL) {
					ppsav = pp;
					PAGE_RECLAIM(ppsav, ppsav_wasfree,
					    ppsav_age);
					PAGE_HOLD(ppsav);
				}
			}
		}

		/* If no pages left on list, we're done */
		if (ppnext == NULL)
			break;

		/* Compute the "next" next page */
		if (ppnext->p_vpnext != ppnext && ppnext->p_vpnext != ppsav)
			ppnextnext = ppnext->p_vpnext;
		else
			ppnextnext = NULL;

		/* Release the next page */
		PAGE_RELE(ppnext);

		/* If releasing the next page freed it, ignore it */
		if (ppnext->p_free) {
			ASSERT(pp->p_vnode == NULL);
			ppnext = NULL;
		}
		/* Move forward to look at next page */
		pp = ppnext;
		ppnext = ppnextnext;
	}

	if (ppsav != NULL) {
		PAGE_RELE(ppsav);
		if (!ppsav->p_free)
			PAGE_REFREE(ppsav, ppsav_wasfree, ppsav_age);
	}
	(void) splx(s);
	return (dirty);
}
#undef	PAGE_RECLAIM
#undef	PAGE_REFREE

/*
 * Used when we need to find a page but don't care about free pages.
 */
static struct page *
pvn_pagefind(vp, off)
	register struct vnode *vp;
	register u_int off;
{
	register struct page *pp;
	register int s;

	s = splvm();
	pp = page_exists(vp, off);
	if (pp != NULL && pp->p_free)
		pp = NULL;
	(void) splx(s);
	return (pp);
}

int pvn_range_noklust = 0;

/*
 * Use page_find's and handle all pages for this vnode whose offset
 * is >= off and < eoff.  This routine will also do klustering up
 * to offlo and offhi up until a page which is not found.  We assume
 * that offlo <= off and offhi >= eoff.
 *
 * Returns a list of dirty kept pages all ready to be written back.
 */
struct page *
pvn_range_dirty(vp, off, eoff, offlo, offhi, flags)
	register struct vnode *vp;
	u_int off, eoff;
	u_int offlo, offhi;
	int flags;
{
	struct page *dirty = NULL;
	register struct page *pp;
	register u_int o;
	register struct page *(*pfind)();

	ASSERT(offlo <= off && offhi >= eoff);

	off &= PAGEMASK;
	eoff = (eoff + PAGEOFFSET) & PAGEMASK;

	/*
	 * If we are not invalidating pages, use the routine,
	 * pvn_pagefind(), to prevent reclaiming them from the
	 * free list.
	 */
	if ((flags & B_INVAL) == 0)
		pfind = pvn_pagefind;
	else
		pfind = page_find;

	/* first do all the pages from [off..eoff] */
	for (o = off; o < eoff; o += PAGESIZE) {
		pp = (*pfind)(vp, o);
		if (pp != NULL) {
			(void) pvn_getdirty(pp, &dirty, flags);
		}
	}

	if (pvn_range_noklust)
		return (dirty);

	/* now scan backwards looking for pages to kluster */
	for (o = off - PAGESIZE; (int)o >= 0 && o >= offlo; o -= PAGESIZE) {
		pp = (*pfind)(vp, o);
		if (pp == NULL)
			break;		/* page not found */
		if (pvn_getdirty(pp, &dirty, flags | B_DELWRI) == 0)
			break;		/* page not added to dirty list */
	}

	/* now scan forwards looking for pages to kluster */
	for (o = eoff; o < offhi; o += PAGESIZE) {
		pp = (*pfind)(vp, o);
		if (pp == NULL)
			break;		/* page not found */
		if (pvn_getdirty(pp, &dirty, flags | B_DELWRI) == 0)
			break;		/* page not added to dirty list */
	}

	return (dirty);
}

/*
 * Take care of invalidating all the pages for vnode vp going to size
 * vplen.  This includes zero'ing out zbytes worth of file beyond vplen.
 * This routine should only be called with the vp locked by the file
 * system code so that more pages cannot be added when sleep here.
 */
void
pvn_vptrunc(vp, vplen, zbytes)
	register struct vnode *vp;
	register u_int vplen;
	u_int zbytes;
{
	register struct page *pp;
	register int s;

	if (vp->v_pages == NULL || vp->v_type == VCHR || vp->v_type == VSOCK)
		return;

	/*
	 * Simple case - abort all the pages on the vnode
	 */
	if (vplen == 0) {
		s = splvm();
		while ((pp = vp->v_pages) != (struct page *)NULL) {
			/*
			 * When aborting these pages, we make sure that
			 * we wait to make sure they are really gone.
			 */
			if (pp->p_keepcnt != 0) {
				(void) splx(s);
				page_wait(pp);
				s = splvm();
				if (pp->p_vnode != vp)
					continue;
			} else {
				if (pp->p_free)
					page_reclaim(pp);
			}
			page_lock(pp);
			page_abort(pp);
		}
		(void) splx(s);
		return;
	}

	/*
	 * Tougher case - have to find all the pages on the
	 * vnode which need to be aborted or partially zeroed.
	 */

	/*
	 * First we get the last page and handle the partially
	 * zeroing via kernel mappings.  This will make the page
	 * dirty so that we know that when this page is written
	 * back, the zeroed information will go out with it.  If
	 * the page is not currently in memory, then the kzero
	 * operation will cause it to be brought it.  We use kzero
	 * instead of bzero so that if the page cannot be read in
	 * for any reason, the system will not panic.  We need
	 * to zero out a minimum of the fs given zbytes, but we
	 * might also have to do more to get the entire last page.
	 */
	if (zbytes != 0) {
		addr_t addr;

		if ((zbytes + (vplen & MAXBOFFSET)) > MAXBSIZE)
			panic("pvn_vptrunc zbytes");
		addr = segmap_getmap(segkmap, vp, vplen & MAXBMASK);
		(void) kzero(addr + (vplen & MAXBOFFSET),
		    MAX(zbytes, PAGESIZE - (vplen & PAGEOFFSET)));
		(void) segmap_release(segkmap, addr, SM_WRITE | SM_ASYNC);
	}

	/*
	 * Synchronously abort all pages on the vp list which are
	 * beyond the new length.  The algorithm here is to start
	 * scanning at the beginning of the vplist until there
	 * are no pages with an offset >= vplen.  If we find such
	 * a page, we wait for it if it is kept for any reason and
	 * then we abort it after verifying that it is still a page
	 * that needs to go away.  We assume here that the vplist
	 * is not messed with at interrupt level.
	 */

	s = splvm();
again:
	for (pp = vp->v_pages; pp != NULL; pp = pp->p_vpnext) {
		if (pp->p_offset >= vplen) {
			/* need to abort this page */
			if (pp->p_keepcnt != 0) {
				(void) splx(s);
				page_wait(pp);
				s = splvm();
				/* verify page identity again */
				if (pp->p_vnode != vp || pp->p_offset < vplen)
					goto again;
			} else {
				if (pp->p_free)
					page_reclaim(pp);
			}
			page_lock(pp);
			page_abort(pp);
			goto again;		/* start over again */
		}
		if (pp == pp->p_vpnext || vp->v_pages == pp->p_vpnext)
			break;
	}
	(void) splx(s);
}

/*
 * This routine is called when the low level address translation
 * code decides to unload a translation.  It calls back to the
 * segment driver which in many cases ends up here.
 */
/*ARGSUSED*/
void
pvn_unloadmap(vp, offset, ref, mod)
	struct vnode *vp;
	u_int offset;
	u_int ref, mod;
{

	/*
	 * XXX - what is the pvn code going to do w/ this information?
	 * This guy gets called for each loaded page when a executable
	 * using the segvn driver terminates...
	 */
}

/*
 * Handles common work of the VOP_GETPAGE routines when more than
 * one page must be returned by calling a file system specific operation
 * to do most of the work.  Must be called with the vp already locked
 * by the VOP_GETPAGE routine.
 */
int
pvn_getpages(getapage, vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	int (*getapage)();
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	register addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	register struct page **ppp;
	register u_int o, eoff;
	u_int sz;
	int err;

	ASSERT(plsz >= len);		/* insure that we have enough space */

	/*
	 * Loop one page at a time and let getapage function fill
	 * in the next page in array.  We only allow one page to be
	 * returned at a time (except for the last page) so that we
	 * don't have any problems with duplicates and other such
	 * painful problems.  This is a very simple minded algorithm,
	 * but it does the job correctly.  We hope that the cost of a
	 * getapage call for a resident page that we might have been
	 * able to get from an earlier call doesn't cost too much.
	 */
	ppp = pl;
	sz = PAGESIZE;
	eoff = off + len;
	for (o = off; o < eoff; o += PAGESIZE, addr += PAGESIZE) {
		if (o + PAGESIZE >= eoff) {
			/*
			 * Last time through - allow the all of
			 * what's left of the pl[] array to be used.
			 */
			sz = plsz - (o - off);
		}
		err = (*getapage)(vp, o, protp, ppp, sz, seg, addr, rw, cred);
		if (err) {
			/*
			 * Release any pages we already got.
			 */
			if (o > off && pl != NULL) {
				for (ppp = pl; *ppp != NULL; *ppp++ = NULL) {
					PAGE_RELE(*ppp);
				}
			}
			break;
		}
		if (pl != NULL)
			ppp++;
	}

	return (err);
}
