/*	@(#)vm_anon.c	1.1 92/07/30	SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * VM - anonymous pages.
 *
 * This layer sits immediately above the vm_swap layer.  It manages
 * physical pages that have no permanent identity in the file system
 * name space, using the services of the vm_swap layer to allocate
 * backing storage for these pages.  Since these pages have no external
 * identity, they are discarded when the last reference is removed.
 *
 * An important function of this layer is to manage low-level sharing
 * of pages that are logically distinct but that happen to be
 * physically identical (e.g., the corresponding pages of the processes
 * resulting from a fork before one process or the other changes their
 * contents).  This pseudo-sharing is present only as an optimization
 * and is not to be confused with true sharing in which multiple
 * address spaces deliberately contain references to the same object;
 * such sharing is managed at a higher level.
 *
 * The key data structure here is the anon struct, which contains a
 * reference count for its associated physical page and a hint about
 * the identity of that page.  Anon structs typically live in arrays,
 * with an instance's position in its array determining where the
 * corresponding backing storage is allocated; however, the swap_xlate()
 * routine abstracts away this representation information so that the
 * rest of the anon layer need not know it.  (See the swap layer for
 * more details on anon struct layout.)
 *
 * In the future versions of the system, the association between an
 * anon struct and its position on backing store will change so that
 * we don't require backing store all anonymous pages in the system.
 * This is important for consideration for large memory systems.
 * We can also use this technique to delay binding physical locations
 * to anonymous pages until pageout/swapout time where we can make
 * smarter allocation decisions to improve anonymous klustering.
 *
 * Many of the routines defined here take a (struct anon **) argument,
 * which allows the code at this level to manage anon pages directly,
 * so that callers can regard anon structs as opaque objects and not be
 * concerned with assigning or inspecting their contents.
 *
 * Clients of this layer refer to anon pages indirectly.  That is, they
 * maintain arrays of pointers to anon structs rather than maintaining
 * anon structs themselves.  The (struct anon **) arguments mentioned
 * above are pointers to entries in these arrays.  It is these arrays
 * that capture the mapping between offsets within a given segment and
 * the corresponding anonymous backing storage address.
 */

#include <sys/param.h>
#include <sys/user.h>	/* XXX - for rusage */
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/vnode.h>
#include <sys/vmmeter.h>
#include <sys/trace.h>
#include <sys/debug.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/swap.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/pvn.h>
#include <vm/rm.h>
#include <vm/mp.h>

struct	anoninfo anoninfo;
#ifdef KMON_DEBUG
kmon_t	anon_lock;
#endif /* KMON_DEBUG */

int anon_resv_debug = 0;
int anon_enforce_resv = 1;

/*
 * Reserve anon space.
 * Return non-zero on success.
 */
int
anon_resv(size)
	u_int size;
{

	anoninfo.ani_resv += btopr(size);
	if (anoninfo.ani_resv > anoninfo.ani_max) {
		if (anon_enforce_resv)
			anoninfo.ani_resv -= btopr(size);
		else if (anon_resv_debug)
			printf("anon: swap space overcommitted by %d\n",
			    anoninfo.ani_resv - anoninfo.ani_max);
		return (!anon_enforce_resv);
	} else {
		return (1);
	}
}

/*
 * Give back an anon reservation.
 */
void
anon_unresv(size)
	u_int size;
{

	anoninfo.ani_resv -= btopr(size);
	if ((int)anoninfo.ani_resv < 0)
		printf("anon: reservations below zero???\n");
}

/*
 * Allocate an anon slot.
 */
struct anon *
anon_alloc()
{
	register struct anon *ap;

	kmon_enter(&anon_lock);
	ap = swap_alloc();
	if (ap != NULL) {
		anoninfo.ani_free--;
		ap->an_refcnt = 1;
		ap->un.an_page = NULL;
	}
	kmon_exit(&anon_lock);
	return (ap);
}

/*
 * Decrement the reference count of an anon page.
 * If reference count goes to zero, free it and
 * its associated page (if any).
 */
static void
anon_decref(ap)
	register struct anon *ap;
{
	register struct page *pp;
	struct vnode *vp;
	u_int off;

	if (--ap->an_refcnt == 0) {
		/*
		 * If there is a page for this anon slot we will need to
		 * call page_abort to get rid of the vp association and
		 * put the page back on the free list as really free.
		 */
		swap_xlate(ap, &vp, &off);
		pp = page_find(vp, off);
		/*
		 * XXX - If we have a page, wait for its keepcnt to become
		 * zero, re-verify the identity before aborting it and
		 * freeing the swap slot.  This ensures that any pending i/o
		 * always completes before the swap slot is freed.
		 */
		if (pp != NULL) {
			if (pp->p_keepcnt != 0) {
				page_wait(pp);
				if (pp->p_vnode == vp && pp->p_offset == off)
					page_abort(pp);
			} else {
				page_abort(pp);
			}
		}
		kmon_enter(&anon_lock);
		swap_free(ap);
		anoninfo.ani_free++;
		kmon_exit(&anon_lock);
	}
}

/*
 * Duplicate references to size bytes worth of anon pages.
 * Used when duplicating a segment that contains private anon pages.
 * This code assumes that procedure calling this one has already used
 * hat_chgprot() to disable write access to the range of addresses that
 * that *old actually refers to.
 */
void
anon_dup(old, new, size)
	register struct anon **old, **new;
	u_int size;
{
	register int i;

	i = btopr(size);
	while (i-- > 0) {
		if ((*new = *old) != NULL)
			(*new)->an_refcnt++;
		old++;
		new++;
	}
}

/*
 * Free a group of "size" anon pages, size in bytes,
 * and clear out the pointers to the anon entries.
 */
void
anon_free(app, size)
	register struct anon **app;
	u_int size;
{
	register int i;

	i = btopr(size);
	while (i-- > 0) {
		if (*app != NULL) {
			anon_decref(*app);
			*app = NULL;
		}
		app++;
	}
}

/*
 * Return the kept page(s) and protections back to the segment driver.
 */
int
anon_getpage(app, protp, pl, plsz, seg, addr, rw, cred)
	struct anon **app;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	register struct page *pp, **ppp;
	register struct anon *ap = *app;
	struct vnode *vp;
	u_int off;
	int err;
	extern int nopagereclaim;
	register int s;

	swap_xlate(ap, &vp, &off);
again:
	pp = ap->un.an_page;
	/*
	 * If the anon pointer has a page associated with it,
	 * see if it looks ok after raising priority to prevent
	 * it from being ripped away at interrupt level if on the
	 * free list.  If the page is being paged in, wait for it
	 * to finish as we must return a list of pages since this
	 * routine acts like the VOP_GETPAGE routine does.
	 */
	s = splvm();
	if (pp != NULL && pp->p_vnode == vp && pp->p_offset == off &&
	    !pp->p_gone && pl != NULL) {
		if (pp->p_intrans && (pp->p_pagein || nopagereclaim)) {
			(void) splx(s);
			page_wait(pp);
			goto again;		/* try again */
		}
		if (pp->p_free)
			page_reclaim(pp);
		(void) splx(s);
		PAGE_HOLD(pp);
		if (ap->an_refcnt == 1)
			*protp = PROT_ALL;
		else
			*protp = PROT_ALL & ~PROT_WRITE;
		pl[0] = pp;
		pl[1] = NULL;
		/* no one else accounted for it so we must */
		u.u_ru.ru_minflt++;
		return (0);
	}
	(void) splx(s);

	/*
	 * Simply treat it as a vnode fault on the anon vp.
	 */
	trace3(TR_SEG_GETPAGE, seg, addr, TRC_SEG_ANON);
	err = VOP_GETPAGE(vp, off, PAGESIZE, protp, pl, plsz,
	    seg, addr, rw, cred);
	if (err == 0 && pl != NULL) {
		for (ppp = pl; (pp = *ppp++) != NULL; ) {
			if (pp->p_offset == off) {
				ap->un.an_page = pp;
				break;
			}
		}
		if (ap->an_refcnt != 1)
			*protp &= ~PROT_WRITE;	/* make read-only */
	}
	return (err);
}

int npagesteal;

/*
 * Turn a reference to an object or shared anon page
 * into a private page with a copy of the data from the
 * original page.  The original page is always kept, locked
 * and loaded in the MMU by the caller.  This routine unlocks
 * the translation and releases the original page, if it isn't
 * being stolen, before returning to the caller.
 */
struct page *
anon_private(app, seg, addr, opp, oppflags)
	struct anon **app;
	struct seg *seg;
	addr_t addr;
	struct page *opp;
	u_int oppflags;
{
	register struct anon *old = *app;
	register struct anon *new;
	register struct page *pp;
	struct vnode *vp;
	u_int off;

	ASSERT(opp->p_mapping);
	ASSERT(opp->p_keepcnt);

	new = anon_alloc();
	if (new == (struct anon *)NULL) {
		rm_outofanon();
		hat_unlock(seg, addr);
		PAGE_RELE(opp);
		return ((struct page *)NULL);	/* out of swap space */
	}
	*app = new;

	swap_xlate(new, &vp, &off);
again:
	pp = page_lookup(vp, off);

	if (pp == NULL && (oppflags & STEAL_PAGE) &&
	    opp->p_keepcnt == 1 && opp->p_mod == 0) {
		pp = opp;
		hat_unlock(seg, addr);		/* unlock translation */
		hat_pageunload(pp);		/* unload all translations */
		page_hashout(pp);		/* destroy old name for page */
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off))	/* rename as anon page */
			panic("anon private steal");
		new->un.an_page = pp;
		pg_setmod(pp, 1);
		page_unlock(pp);
		/*
		 * If original page is ``locked'', relinquish
		 * claim for the extra page.
		 */
		if (oppflags & LOCK_PAGE)
			page_subclaim(1);
		npagesteal++;
		return (pp);
	}

	if (pp == NULL) {
		/*
		 * Normal case, need to allocate new page frame.
		 */
		pp = rm_allocpage(seg, addr, PAGESIZE, 1);
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off)) {
			PAGE_RELE(pp);
			goto again;		/* try again */
		}
	} else {
		/*
		 * Already found a page with the right identity -- just
		 * use it if the `keepcnt' is 0.  If not, wait for the
		 * `keepcnt' to become 0, re-verify the identity before
		 * using the page.
		 */
		if (pp->p_keepcnt != 0) {
			page_wait(pp);
			if (pp->p_vnode != vp || pp->p_offset != off)
				goto again;
		}
		page_lock(pp);
		PAGE_HOLD(pp);
	}
	new->un.an_page = pp;

	/*
	 * Now copy the contents from the original page which
	 * is loaded and locked in the MMU by the caller to
	 * prevent yet another page fault.
	 */
	pp->p_intrans = pp->p_pagein = 1;
	pagecopy(addr, pp);
	pp->p_intrans = pp->p_pagein = 0;
	pg_setmod(pp, 1);		/* mark as modified */
	page_unlock(pp);

	/*
	 * If original page is ``locked'', relinquish claim
	 * for an extra page reserved for the private copy
	 * in case of a copy-on-write.  Lock the new page
	 * ignoring the current reservation check.
	 */
	if (oppflags & LOCK_PAGE) {
		if (old == NULL)
			page_pp_unlock(opp, 1);
		else
			page_pp_unlock(opp, 0);
		(void) page_pp_lock(pp, 0, 0);
	}

	/*
	 * Unlock translation to the original page since
	 * it can be unloaded if the page is aborted.
	 */
	hat_unlock(seg, addr);

	/*
	 * Ok, now release the original page, or else the
	 * process will sleep forever in anon_decref()
	 * waiting for the `keepcnt' to become 0.
	 */
	PAGE_RELE(opp);

	/*
	 * If we copied away from an anonymous page, then
	 * we are one step closer to freeing up an anon slot.
	 */
	if (old != NULL)
		anon_decref(old);
	return (pp);
}

/*
 * Allocate a zero-filled anon page.
 */
struct page *
anon_zero(seg, addr, app)
	struct seg *seg;
	addr_t addr;
	struct anon **app;
{
	register struct anon *ap;
	register struct page *pp;
	struct vnode *vp;
	u_int off;

	*app = ap = anon_alloc();
	if (ap == NULL) {
		rm_outofanon();
		return ((struct page *)NULL);
	}

	swap_xlate(ap, &vp, &off);
again:
	pp = page_lookup(vp, off);

	if (pp == NULL) {
		/*
		 * Normal case, need to allocate new page frame.
		 */
		pp = rm_allocpage(seg, addr, PAGESIZE, 1);
		trace6(TR_SEG_ALLOCPAGE, seg, addr, TRC_SEG_ANON, vp, off, pp);
		if (page_enter(pp, vp, off)) {
			PAGE_RELE(pp);
			goto again;		/* try again */
		}
	} else {
		/*
		 * Already found a page with the right identity -- just
		 * use it if the `keepcnt' is 0.  If not, wait for the
		 * `keepcnt' to become 0, re-verify the identity before
		 * using the page.
		 */
		if (pp->p_keepcnt != 0) {
			page_wait(pp);
			if (pp->p_vnode != vp || pp->p_offset != off)
				goto again;
		}
		page_lock(pp);
		PAGE_HOLD(pp);
	}
	ap->un.an_page = pp;

	pagezero(pp, 0, PAGESIZE);
	cnt.v_zfod++;
	pg_setmod(pp, 1);	/* mark as modified so pageout writes back */
	page_unlock(pp);
	return (pp);
}

/*
 * This gets calls by the seg_vn driver unload routine
 * which is called by the hat code when it decides to
 * unload a particular mapping.
 */
void
anon_unloadmap(ap, ref, mod)
	struct anon *ap;
	u_int ref, mod;
{
	struct vnode *vp;
	u_int off;

	swap_xlate(ap, &vp, &off);
	pvn_unloadmap(vp, off, ref, mod);
}
