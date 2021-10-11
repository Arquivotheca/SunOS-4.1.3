/*	@(#)seg_map.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - generic vnode mapping segment.
 *
 * The segmap driver is used only by the kernel to get faster (than seg_vn)
 * mappings [lower routine overhead; more persistent cache] to random
 * vnode/offsets.  Note than the kernel may (and does) use seg_vn as well.
 */


#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/ucred.h>
#include <sys/trace.h>
#include <sys/debug.h>
#include <sys/user.h>
#include <sys/kernel.h>

#include <machine/seg_kmem.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/rm.h>

/*
 * Private seg op routines.
 */
static	int segmap_free(/* seg */);
static	faultcode_t segmap_fault(/* seg, addr, len, type, rw */);
static	faultcode_t segmap_faulta(/* seg, addr */);
static	int segmap_checkprot(/* seg, addr, len, prot */);
static	int segmap_kluster(/* seg, addr, delta */);
static	int segmap_badop();

struct	seg_ops segmap_ops = {
	segmap_badop,		/* dup */
	segmap_badop,		/* unmap */
	segmap_free,
	segmap_fault,
	segmap_faulta,
	(int (*)()) NULL,	/* unload */
	segmap_badop,		/* setprot */
	segmap_checkprot,
	segmap_kluster,
	(u_int (*)()) NULL,	/* swapout */
	segmap_badop,		/* sync */
	segmap_badop,		/* incore */
	segmap_badop,		/* lockop */
	segmap_badop,		/* advise */
};

/*
 * Private segmap routines.
 */
static	void segmap_smapadd(/* smd, smp */);
static	void segmap_smapsub(/* smd, smp */);
static	void segmap_hashin(/* smd, smp, vp, off, flags */);
static	void segmap_hashout(/* smd, smp */);

/*
 * Statistics for segmap operations.
 */
struct segmapcnt {
	int	smc_fault;	/* number of segmap_faults */
	int	smc_faulta;	/* number of segmap_faultas */
	int	smc_getmap;	/* number of segmap_getmaps */
	int	smc_get_use;	/* # of getmaps that reuse an existing map */
	int	smc_get_reclaim; /* # of getmaps that do a reclaim */
	int	smc_get_reuse;	/* # of getmaps that reuse a slot */
	int	smc_rel_async;	/* # of releases that are async */
	int	smc_rel_write;	/* # of releases that write */
	int	smc_rel_free;	/* # of releases that free */
	int	smc_rel_abort;	/* # of releases that abort */
	int	smc_rel_dontneed; /* # of releases with dontneed set */
	int	smc_release;	/* # of releases with no other action */
	int	smc_pagecreate;	/* # of pagecreates */
} segmapcnt;

/*
 * Return number of map pages in segment.
 */
#define	MAP_PAGES(seg)		((seg)->s_size >> MAXBSHIFT)

/*
 * Translate addr into smap number within segment.
 */
#define	MAP_PAGE(seg, addr)	(((addr) - (seg)->s_base) >> MAXBSHIFT)

/*
 * Translate addr in seg into struct smap pointer.
 */
#define	GET_SMAP(seg, addr)	\
	&(((struct segmap_data *)((seg)->s_data))->smd_sm[MAP_PAGE(seg, addr)])

int
segmap_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segmap_data *smd;
	register struct smap *smp;
	struct segmap_crargs *a = (struct segmap_crargs *)argsp;
	register u_int i;
	u_int hashsz;
	addr_t segend;

	/*
	 * Make sure that seg->s_base and seg->s_base + seg->s_size
	 * are on MAXBSIZE aligned pieces of virtual memory.
	 *
	 * Since we assume we are creating a large segment
	 * (it's just segkmap), trimming off the excess at the
	 * beginning and end of the segment is considered safe.
	 */
	segend = (addr_t)((u_int)(seg->s_base + seg->s_size) & MAXBMASK);
	seg->s_base = (addr_t)roundup((u_int)(seg->s_base), MAXBSIZE);
	seg->s_size = segend - seg->s_base;

	i = MAP_PAGES(seg);

	smd = (struct segmap_data *)new_kmem_zalloc(
			sizeof (struct segmap_data), KMEM_SLEEP);
	smd->smd_prot = a->prot;
	smd->smd_sm = (struct smap *)new_kmem_zalloc(
		(u_int)(sizeof (struct smap) * i), KMEM_SLEEP);

	/*
	 * Link up all the slots.
	 */
	for (smp = &smd->smd_sm[i - 1]; smp >= smd->smd_sm; smp--)
		segmap_smapadd(smd, smp);

	/*
	 * Compute hash size rounding down to the next power of two.
	 */
	hashsz = MAP_PAGES(seg) / SMAP_HASHAVELEN;
	for (i = 0x80 << ((sizeof (int) - 1) * NBBY); i != 0; i >>= 1) {
		if ((hashsz & i) != 0) {
			smd->smd_hashsz = hashsz = i;
			break;
		}
	}
	smd->smd_hash = (struct smap **)new_kmem_zalloc(
			hashsz * sizeof (smd->smd_hash[0]), KMEM_SLEEP);

	seg->s_data = (char *)smd;
	seg->s_ops = &segmap_ops;

	return (0);
}

static int
segmap_free(seg)
	struct seg *seg;
{
	register struct segmap_data *smd = (struct segmap_data *)seg->s_data;

	kmem_free((caddr_t)smd->smd_hash, sizeof (smd->smd_hash[0]) *
	    smd->smd_hashsz);
	kmem_free((caddr_t)smd->smd_sm, sizeof (struct smap) * MAP_PAGES(seg));
	kmem_free((caddr_t)smd, sizeof (*smd));
}

/*
 * Do a F_SOFTUNLOCK call over the range requested.
 * The range must have already been F_SOFTLOCK'ed.
 */
static void
segmap_unlock(seg, addr, len, rw, smp)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum seg_rw rw;
	register struct smap *smp;
{
	register struct page *pp;
	register addr_t adr;
	u_int off;

	off = smp->sm_off + ((u_int)addr & MAXBOFFSET);
	for (adr = addr; adr < addr + len; adr += PAGESIZE, off += PAGESIZE) {
		/*
		 * For now, we just kludge here by finding the page
		 * ourselves since we would not find the page using
		 * page_find() if someone has page_abort()'ed it.
		 * XXX - need to redo things to avoid this mess.
		 */
		for (pp = page_hash[PAGE_HASHFUNC(smp->sm_vp, off)]; pp != NULL;
		    pp = pp->p_hash)
			if (pp->p_vnode == smp->sm_vp && pp->p_offset == off)
				break;
		if (pp == NULL || pp->p_pagein || pp->p_free)
			panic("segmap_unlock");
		if (rw == S_WRITE)
			pg_setmod(pp, 1);
		if (rw != S_OTHER) {
			trace4(TR_PG_SEGMAP_FLT, pp, pp->p_vnode, off, 1);
			pg_setref(pp, 1);
		}
		hat_unlock(seg, adr);
		PAGE_RELE(pp);
	}
}

/*
 * This routine is called via a machine specific fault handling
 * routine.  It is also called by software routines wishing to
 * lock or unlock a range of addresses.
 */
static faultcode_t
segmap_fault(seg, addr, len, type, rw)
	struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register struct segmap_data *smd;
	register struct smap *smp;
	register struct page *pp, **ppp;
	register struct vnode *vp;
	register u_int off;
	struct page *pl[btopr(MAXBSIZE) + 1];
	u_int prot;
	u_int addroff;
	addr_t adr;
	int err;

	segmapcnt.smc_fault++;

	smd = (struct segmap_data *)seg->s_data;
	smp = GET_SMAP(seg, addr);
	vp = smp->sm_vp;

	if (vp == NULL)
		return (FC_MAKE_ERR(EIO));

	addroff = (u_int)addr & MAXBOFFSET;
	if (addroff + len > MAXBSIZE)
		panic("segmap_fault length");
	off = smp->sm_off + addroff;

	/*
	 * First handle the easy stuff
	 */
	if (type == F_SOFTUNLOCK) {
		segmap_unlock(seg, addr, len, rw, smp);
		return (0);
	}

	trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_SEGKMAP);
	err = VOP_GETPAGE(vp, off, len, &prot, pl, MAXBSIZE, seg, addr, rw,
	    (struct ucred *)NULL);		/* XXX - need real cred val */

	if (err)
		return (FC_MAKE_ERR(err));

	prot &= smd->smd_prot;

	/*
	 * Handle all pages returned in the pl[] array.
	 * This loop is coded on the assumption that if
	 * there was no error from the VOP_GETPAGE routine,
	 * that the page list returned will contain all the
	 * needed pages for the vp from [off..off + len).
	 */
	for (ppp = pl; (pp = *ppp++) != NULL; ) {
		/*
		 * Verify that the pages returned are within the range
		 * of this segmap region.  Note that this is theoretically
		 * possible for pages outside this range to be returned,
		 * but it is not very unlikely.  If we cannot use the
		 * page here, just release it and go on to the next one.
		 */
		if (pp->p_offset < smp->sm_off ||
		    pp->p_offset >= smp->sm_off + MAXBSIZE) {
			PAGE_RELE(pp);
			continue;
		}

		adr = addr + (pp->p_offset - off);
		if (adr >= addr && adr < addr + len) {
			pg_setref(pp, 1);
			trace4(TR_PG_SEGMAP_FLT, pp, pp->p_vnode, pp->p_offset,
			    0);
			trace5(TR_SPG_FLT, u.u_ar0[PC], adr, vp, pp->p_offset,
			    TRC_SPG_SMAP);
			trace6(TR_SPG_FLT_PROC, time.tv_sec, time.tv_usec,
			    trs(u.u_comm,0), trs(u.u_comm,1),
			    trs(u.u_comm,2), trs(u.u_comm,3));
			if (type == F_SOFTLOCK) {
				/*
				 * Load up the translation keeping it
				 * locked and don't PAGE_RELE the page.
				 */
				hat_memload(seg, adr, pp, prot, 1);
				continue;
			}
		}
		/*
		 * Either it was a page outside the fault range or a
		 * page inside the fault range for a non F_SOFTLOCK -
		 * load up the hat translation and release the page.
		 */
		hat_memload(seg, adr, pp, prot, 0);
		PAGE_RELE(pp);
	}

	return (0);
}

/*
 * This routine is used to start I/O on pages asynchronously.
 */
static faultcode_t
segmap_faulta(seg, addr)
	struct seg *seg;
	addr_t addr;
{
	register struct smap *smp;
	int err;

	segmapcnt.smc_faulta++;
	smp = GET_SMAP(seg, addr);
	if (smp->sm_vp == NULL) {
		call_debug("segmap_faulta - no vp");
		return (FC_MAKE_ERR(EIO));
	}
	trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_SEGKMAP);
	err = VOP_GETPAGE(smp->sm_vp, smp->sm_off + (u_int)addr & MAXBOFFSET,
	    PAGESIZE, (u_int *)NULL, (struct page **)NULL, 0,
	    seg, addr, S_READ,
	    (struct ucred *)NULL);		/* XXX - need real cred val */
	if (err)
		return (FC_MAKE_ERR(err));
	return (0);
}

/*ARGSUSED*/
static int
segmap_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	struct segmap_data *smd = (struct segmap_data *)seg->s_data;

	return (((smd->smd_prot & prot) != prot) ? -1 : 0);
}

/*
 * Check to see if it makes sense to do kluster/read ahead to
 * addr + delta relative to the mapping at addr.  We assume here
 * that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * For segmap we always "approve" of this action from our standpoint.
 */
/*ARGSUSED*/
static int
segmap_kluster(seg, addr, delta)
	struct seg *seg;
	addr_t addr;
	int delta;
{

	return (0);
}

static
segmap_badop()
{

	panic("segmap_badop");
	/*NOTREACHED*/
}

/*
 * Special private segmap operations
 */

/*
 * Add smp to the free list on smd.  If the smp still has a vnode
 * association with it, then it is added to the end of the free list,
 * otherwise it is added to the front of the list.
 */
static void
segmap_smapadd(smd, smp)
	register struct segmap_data *smd;
	register struct smap *smp;
{

	if (smp->sm_refcnt != 0)
		panic("segmap_smapadd");

	if (smd->smd_free == (struct smap *)NULL) {
		smp->sm_next = smp->sm_prev = smp;
	} else {
		smp->sm_next = smd->smd_free;
		smp->sm_prev = (smd->smd_free)->sm_prev;
		(smd->smd_free)->sm_prev = smp;
		smp->sm_prev->sm_next = smp;
	}

	if (smp->sm_vp == (struct vnode *)NULL)
		smd->smd_free = smp;
	else
		smd->smd_free = smp->sm_next;

	/*
	 * XXX - need a better way to do this.
	 */
	if (smd->smd_want) {
		wakeup((caddr_t)&smd->smd_free);
		smd->smd_want = 0;
	}
}

/*
 * Remove smp from the smd free list.  If there is an old
 * mapping in effect there, then delete it.
 */
static void
segmap_smapsub(smd, smp)
	register struct segmap_data *smd;
	register struct smap *smp;
{

	if (smd->smd_free == smp)
		smd->smd_free = smp->sm_next;	/* go to next page */

	if (smd->smd_free == smp)
		smd->smd_free = NULL;		/* smp list is gone */
	else {
		smp->sm_prev->sm_next = smp->sm_next;
		smp->sm_next->sm_prev = smp->sm_prev;
	}
	smp->sm_prev = smp->sm_next = smp;	/* make smp a list of one */
	smp->sm_refcnt = 1;
}

static void
segmap_hashin(smd, smp, vp, off)
	register struct segmap_data *smd;
	register struct smap *smp;
	struct vnode *vp;
	u_int off;
{
	register struct smap **hpp;

	/*
	 * Funniness here - we don't increment the ref count on the vnode
	 * even though we have another pointer to it here.  The reason
	 * for this is that we don't want the fact that a seg_map
	 * entry somewhere refers to a vnode to prevent the vnode
	 * itself from going away.  This is because this reference
	 * to the vnode is a "soft one".  In the case where a mapping
	 * is being used by a rdwr [or directory routine?] there already
	 * has to be a non-zero ref count on the vnode.  In the case
	 * where the vp has been freed and the the smap structure is
	 * on the free list, there are no pages in memory that can
	 * refer to the vnode.  Thus even if we reuse the same
	 * vnode/smap structure for a vnode which has the same
	 * address but represents a different object, we are ok.
	 */
	smp->sm_vp = vp;
	smp->sm_off = off;

	hpp = &smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
	smp->sm_hash = *hpp;
	*hpp = smp;
}

static void
segmap_hashout(smd, smp)
	register struct segmap_data *smd;
	register struct smap *smp;
{
	register struct smap **hpp, *hp;
	struct vnode *vp;

	vp = smp->sm_vp;
	hpp = &smd->smd_hash[SMAP_HASHFUNC(smd, vp, smp->sm_off)];
	for (;;) {
		hp = *hpp;
		if (hp == NULL)
			panic("segmap_hashout");
		if (hp == smp)
			break;
		hpp = &hp->sm_hash;
	}

	*hpp = smp->sm_hash;
	smp->sm_hash = NULL;
	smp->sm_vp = NULL;
	smp->sm_off = 0;
}

/*
 * Special public segmap operations
 */

/*
 * Create pages (without using VOP_GETPAGE) and load up tranlations to them.
 * If softlock is TRUE, then set things up so that it looks like a call
 * to segmap_fault with F_SOFTLOCK.
 */
void
segmap_pagecreate(seg, addr, len, softlock)
	struct seg *seg;
	register addr_t addr;
	u_int len;
	int softlock;
{
	register struct page *pp;
	register u_int off;
	struct smap *smp;
	struct vnode *vp;
	addr_t eaddr;
	u_int prot;

	segmapcnt.smc_pagecreate++;

	eaddr = addr + len;
	addr = (addr_t)((u_int)addr & PAGEMASK);
	smp = GET_SMAP(seg, addr);
	vp = smp->sm_vp;
	off = smp->sm_off + ((u_int)addr & MAXBOFFSET);
	prot = ((struct segmap_data *)seg->s_data)->smd_prot;

	for (; addr < eaddr; addr += PAGESIZE, off += PAGESIZE) {
		pp = page_lookup(vp, off);
		if (pp == NULL) {
			pp = rm_allocpage(segkmap, addr, PAGESIZE, 1);
			trace6(TR_SEG_ALLOCPAGE, segkmap, addr,
				TRC_SEG_SEGKMAP, vp, off, pp);
			if (page_enter(pp, vp, off))
				panic("segmap_page_create page_enter");
			page_unlock(pp);
			if (softlock) {
				hat_memload(segkmap, addr, pp, prot, 1);
			} else {
				hat_memload(segkmap, addr, pp, prot, 0);
				PAGE_RELE(pp);
			}
		} else {
			if (softlock) {
				PAGE_HOLD(pp);
				hat_memload(segkmap, addr, pp, prot, 1);
			} else {
				hat_memload(segkmap, addr, pp, prot, 0);
			}
		}
	}
}


addr_t
segmap_getmap(seg, vp, off)
	struct seg *seg;
	struct vnode *vp;
	u_int off;
{
	register struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	register struct smap *smp;

	segmapcnt.smc_getmap++;

	if ((off & MAXBOFFSET) != 0)
		panic("segmap_getmap bad offset");

	/*
	 * XXX - keep stats for hash function
	 */
	for (smp = smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
	    smp != NULL; smp = smp->sm_hash)
		if (smp->sm_vp == vp && smp->sm_off == off)
			break;

	if (smp != NULL) {
		if (vp->v_count == 0)		/* XXX - debugging */
			call_debug("segmap_getmap vp count of zero");
		if (smp->sm_refcnt != 0) {
			segmapcnt.smc_get_use++;
			smp->sm_refcnt++;		/* another user */
		} else {
			segmapcnt.smc_get_reclaim++;
			segmap_smapsub(smd, smp);	/* reclaim */
		}
	} else {
		/*
		 * Allocate a new slot and set it up.
		 */
		while ((smp = smd->smd_free) == NULL) {
			/*
			 * XXX - need a better way to do this.
			 */
			smd->smd_want = 1;
			(void) sleep((caddr_t)&smd->smd_free, PSWP+2);
		}
		segmap_smapsub(smd, smp);
		if (smp->sm_vp != (struct vnode *)NULL) {
			/*
			 * Destroy old vnode association and unload any
			 * hardware translations to the old object.
			 */
			segmapcnt.smc_get_reuse++;
			segmap_hashout(smd, smp);
			hat_unload(seg, seg->s_base + ((smp - smd->smd_sm) *
			    MAXBSIZE), MAXBSIZE);
		}
		segmap_hashin(smd, smp, vp, off);
	}

	trace5(TR_SEG_GETMAP, seg, (u_int)(seg->s_base +
		(smp - smd->smd_sm) * MAXBSIZE) & PAGEMASK,
		TRC_SEG_SEGKMAP, vp, off);
	return (seg->s_base + ((smp - smd->smd_sm) * MAXBSIZE));
}



/*
 * Same as segmap_getmap(), with the following condition added
 * if (a new mapping is created) 
 *    prefault the translation 
 */
addr_t
segmap_getmapflt(seg, vp, off)
 	struct seg *seg;
 	struct vnode *vp;
 	u_int off;
{
	register struct segmap_data *smd = (struct segmap_data *)seg->s_data;
 	register struct smap *smp;
 
 	segmapcnt.smc_getmap++;
 
 	if ((off & MAXBOFFSET) != 0)
 		panic("segmap_getmap bad offset");
 
 	/*
 	 * XXX - keep stats for hash function
 	 */
 	for (smp = smd->smd_hash[SMAP_HASHFUNC(smd, vp, off)];
 	    smp != NULL; smp = smp->sm_hash)
 		if (smp->sm_vp == vp && smp->sm_off == off)
 			break;
 
 	if (smp != NULL) {
 		if (vp->v_count == 0)		/* XXX - debugging */
 			call_debug("segmap_getmap vp count of zero");
 		if (smp->sm_refcnt != 0) {
 			segmapcnt.smc_get_use++;
 			smp->sm_refcnt++;		/* another user */
 		} else {
 			segmapcnt.smc_get_reclaim++;
 			segmap_smapsub(smd, smp);	/* reclaim */
 		}
 	} else {
 		/*
 		 * Allocate a new slot and set it up.
 		 */
 		while ((smp = smd->smd_free) == NULL) {
 			/*
 			 * XXX - need a better way to do this.
 			 */
 			smd->smd_want = 1;
 			(void) sleep((caddr_t)&smd->smd_free, PSWP+2);
 		}
 		segmap_smapsub(smd, smp);
 		if (smp->sm_vp != (struct vnode *)NULL) {
 			/*
 			 * Destroy old vnode association and unload any
 			 * hardware translations to the old object.
 			 */
 			segmapcnt.smc_get_reuse++;
 			segmap_hashout(smd, smp);
 			hat_unload(seg, seg->s_base + ((smp - smd->smd_sm) *
 			    MAXBSIZE), MAXBSIZE);
 		}
 		segmap_hashin(smd, smp, vp, off);
 
 		/*
 		 * Prefault the translation 
		 */
 		(void)as_fault(&kas,
 			seg->s_base + (smp - smd->smd_sm) * MAXBSIZE,
 			MAXBSIZE, F_INVAL, S_READ);
 	}
 
 	trace5(TR_SEG_GETMAP, seg, (u_int)(seg->s_base +
 		(smp - smd->smd_sm) * MAXBSIZE) & PAGEMASK,
 		TRC_SEG_SEGKMAP, vp, off);
 	return (seg->s_base + ((smp - smd->smd_sm) * MAXBSIZE));
}
 

int
segmap_release(seg, addr, flags)
	struct seg *seg;
	addr_t addr;
	u_int flags;
{
	register struct segmap_data *smd = (struct segmap_data *)seg->s_data;
	register struct smap *smp;
	int error;

	if (addr < seg->s_base || addr >= seg->s_base + seg->s_size ||
	    ((u_int)addr & MAXBOFFSET) != 0)
		panic("segmap_release addr");

	smp = &smd->smd_sm[MAP_PAGE(seg, addr)];
	trace4(TR_SEG_RELMAP, seg, addr, TRC_SEG_SEGKMAP, smp->sm_refcnt);

	/*
	 * Need to call VOP_PUTPAGE if any flags (except SM_DONTNEED)
	 * are set.
	 */
	if ((flags & ~SM_DONTNEED) != 0) {
		int bflags = 0;

		if (flags & SM_WRITE)
			segmapcnt.smc_rel_write++;
		if (flags & SM_ASYNC) {
			bflags |= B_ASYNC;
			segmapcnt.smc_rel_async++;
		}
		if (flags & SM_INVAL) {
			bflags |= B_INVAL;
			segmapcnt.smc_rel_abort++;
		}
		if (smp->sm_refcnt == 1) {
			/*
			 * We only bother doing the FREE and DONTNEED flags
			 * if no one else is still referencing this mapping.
			 */
			if (flags & SM_FREE) {
				bflags |= B_FREE;
				segmapcnt.smc_rel_free++;
			}
			if (flags & SM_DONTNEED) {
				bflags |= B_DONTNEED;
				segmapcnt.smc_rel_dontneed++;
			}
		}
		error = VOP_PUTPAGE(smp->sm_vp, smp->sm_off, MAXBSIZE, bflags,
		    (struct ucred *)NULL);	/* XXX - need real cred val */
	} else {
		segmapcnt.smc_release++;
		error = 0;
	}

	if (--smp->sm_refcnt == 0) {
		if (flags & SM_INVAL) {
			hat_unload(seg, addr, MAXBSIZE);
			segmap_hashout(smd, smp);	/* remove map info */
		}
		segmap_smapadd(smd, smp);		/* add to free list */
	}

	return (error);
}
