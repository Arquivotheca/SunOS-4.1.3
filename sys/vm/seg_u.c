/*	@(#)seg_u.c	1.1 92/07/30	SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - u-area segment routines
 *
 * XXX:	This segment type should probably be recast as seg_stack
 *	instead of seg_u.  As the system evolves, we'll need to
 *	manage variable-sized stacks protected by red zones, some
 *	of which possibly are accompanied by u-areas.  For the moment
 *	the implementation copes only with "standard" u-areas,
 *	each with an embedded stack.  Doing so lets the implementation
 *	get away with much simpler space management code.
 *
 * Desired model:
 *	segu_data describes nproc u-areas and the segment ops
 *	manipulate individual slots in segu_data, so that (e.g.)
 *	copying a u-area upon process creation turns into
 *	transcribing parts of segu_data from one place to another.
 *
 * Red zone handling:
 *	The implementation maintains the invariant that the MMU mappings
 *	for unallocated slots are invalid.  This means that red zones
 *	come for free simply by avoiding establishing mappings over all
 *	red zone pages and by making sure that all mappings are invalidated
 *	at segu_release time.
 *
 *	Note also that we need neither pages nor swap space for red zones,
 *	so much of the code works over extents of SEGU_PAGES-1 instead
 *	of SEGU_PAGES.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ucred.h>
#include <sys/vnode.h>
#include <sys/kmem_alloc.h>
#include <sys/proc.h>	/* needed for debugging printouts only */
#include <sys/vmmeter.h>

#include <vm/anon.h>
#include <vm/rm.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_u.h>
#include <vm/swap.h>
#include <vm/hat.h>

/*
 * Ugliness to compensate for some machine dependency.
 */
#ifdef	i386bug
#define	UPAGE_PROT	(PROT_READ | PROT_USER)
#else	i386bug
#define	UPAGE_PROT	(PROT_READ | PROT_WRITE)
#endif	i386bug

int	segu_debug = 0;		/* patchable for debugging */

/*
 * Private seg op routines.
 *
 * The swapout operation is null because the generic swapout code
 * never attempts to swap out anything in the kernel's address
 * space.  Instead, clients swap the resources this driver manages
 * by calling segu_fault with a type argument of F_SOFTLOCK to swap
 * a slot in and with F_SOFTUNLOCK to swap one out.
 */
static	int segu_checkprot(/* seg, vaddr, len, prot */);
static	int segu_kluster(/* seg, vaddr, delta */);
static	int segu_badop();

struct	seg_ops segu_ops = {
	segu_badop,		/* dup */
	segu_badop,		/* unmap */
	segu_badop,		/* free */
	segu_fault,
	segu_badop,		/* faulta */
	(int (*)()) NULL,	/* unload */
	segu_badop,		/* setprot */
	segu_checkprot,
	segu_kluster,
	(u_int (*)()) NULL,	/* swapout */
	segu_badop,		/* sync */
	segu_badop,		/* incore */
	segu_badop,		/* lockop */
	segu_badop,		/* advise */
};

/*
 * Declarations of private routines for use by seg_u operations.
 */
static	int segu_getslot(/* seg, vaddr, len */);
static	int segu_softunlock(/* seg, vaddr, len, slot */);
static	int segu_softload(/* seg, vaddr, len, slot, lock */);

struct seg	*segu;

/*
 * XXX:	Global change needed -- set up MMU translations before
 *	keeping pages.
 */

static
segu_badop()
{

	panic("seg_badop");
	/* NOTREACHED */
}

/*
 * Handle a fault on an address corresponding to one of the
 * slots in the segu segment.
 */
faultcode_t
segu_fault(seg, vaddr, len, type, rw)
	struct seg	*seg;
	addr_t		vaddr;
	u_int		len;
	enum fault_type	type;
	enum seg_rw	rw;
{
	struct segu_segdata	*sdp = (struct segu_segdata *)seg->s_data;
	struct segu_data	*sup;
	int			slot;
	addr_t			vbase;
	int			err;

	/*
	 * Sanity checks.
	 */
	if (seg != segu)
		panic("segu_fault: wrong segment");
	if (type == F_PROT)
		panic("segu_fault: unexpected F_PROT fault");

	/*
	 * Verify that the range specified by vaddr and len falls
	 * completely within the mapped part of a single allocated
	 * slot, calculating the slot index and slot pointer while
	 * we're at it.
	 */
	slot = segu_getslot(seg, vaddr, len);
	if (slot == -1)
		return (FC_MAKE_ERR(EFAULT));
	sup = &sdp->usd_slots[slot];

	vbase = seg->s_base + ptob(SEGU_PAGES) * slot;

	/*
	 * The F_SOFTLOCK and F_SOFTUNLOCK cases have more stringent
	 * range requirements: the given range must exactly coincide
	 * with the slot's mapped portion.
	 */
	if (type == F_SOFTLOCK || type == F_SOFTUNLOCK) {
		if (vaddr != segu_stom(vbase) || len != ptob(SEGU_PAGES - 1))
			return (FC_MAKE_ERR(EFAULT));
	}

	if (type == F_SOFTLOCK) {
		/*
		 * Somebody is trying to lock down this slot, e.g., as
		 * part of swapping in a u-area contained in the slot.
		 */

		/*
		 * It is erroneous to attempt to lock when already locked.
		 *
		 * XXX:	Possibly this shouldn't be a panic.  It depends
		 *	on what assumptions we're willing to let clients
		 *	make.
		 */
		if (sup->su_flags & SEGU_LOCKED)
			panic("segu_fault: locking locked slot");

		err = segu_softload(seg, segu_stom(vbase),
				ptob(SEGU_PAGES - 1), slot, 1);
		if (err)
			return (FC_MAKE_ERR(err));

		sup->su_flags |= SEGU_LOCKED;
		return (0);
	}

	if (type == F_INVAL) {
		/*
		 * Normal fault. The processing required
		 * is quite similar to that for the F_SOFTLOCK case in that
		 * we have to drag stuff in and make sure it's mapped.  It
		 * differs in that we don't lock it down.
		 */

		if (segu_debug)
			printf("segu_fault(%x, %x, %d)\n", vaddr, len, type);

		/*
		 * If the slot is already locked, the only way we
		 * should fault is by referencing the red zone.
		 *
		 * XXX:	Probably should tighten this check and verify
		 *	that it's really a red zone reference.
		 * XXX:	Is this the most appropriate error code?
		 */
		if (sup->su_flags & SEGU_LOCKED)
			return (FC_MAKE_ERR(EINVAL));

		err = segu_softload(seg, vaddr, len, slot, 0);
		return (err ? FC_MAKE_ERR(err) : 0);
	}

	if (type == F_SOFTUNLOCK) {
		/*
		 * Somebody is trying to swap out this slot, e.g., as
		 * part of swapping out a u-area contained in this slot.
		 */

		/*
		 * It is erroneous to attempt to unlock when not
		 * currently locked.
		 */
		if (!(sup->su_flags & SEGU_LOCKED))
			panic("segu_fault: unlocking unlocked slot");
		sup->su_flags &= ~SEGU_LOCKED;

		err = segu_softunlock(seg, vaddr, len, slot, rw);
		return (err ? FC_MAKE_ERR(err) : 0);
	}

	panic("segu_fault: bogus fault type");
	/* NOTREACHED */
}

/*
 * Check that the given protections suffice over the range specified by
 * vaddr and len.  For this segment type, the only issue is whether or
 * not the range lies completely within the mapped part of an allocated slot.
 *
 * We let segu_getslot do all the dirty work.
 */
/* ARGSUSED */
static int
segu_checkprot(seg, vaddr, len, prot)
	struct seg	*seg;
	addr_t		vaddr;
	u_int		len;
	u_int		prot;
{
	register int	slot = segu_getslot(seg, vaddr, len);

	return (slot == -1 ? -1 : 0);
}

/*
 * Check to see if it makes sense to do kluster/read ahead to
 * addr + delta relative to the mapping at addr.  We assume here
 * that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * For seg_u we always "approve" of this action from our standpoint.
 */
/* ARGSUSED */
static int
segu_kluster(seg, addr, delta)
	struct seg	*seg;
	addr_t		addr;
	int		delta;
{
	return (0);
}


/*
 * Segment operations specific to the seg_u segment type.
 */

/*
 * Finish creating the segu segment by setting up its private state
 * information.  Called once at boot time after segu has been allocated
 * and hooked into the kernel address space.
 *
 * Note that we have no need for the argsp argument, since everything
 * we need to set up our private information is contained in the common
 * segment information.  (This may change at such time as we generalize
 * the implementation to deal with variable size allocation units.)
 */
/* ARGSUSED */
int
segu_create(seg, argsp)
	register struct seg	*seg;
	caddr_t			argsp;
{
	register u_int			numslots;
	register int			i;
	register struct segu_segdata	*sdp;

	/*
	 * Trim the segment's size down to the largest multiple of
	 * SEGU_PAGES that's no larger than the original value.
	 *
	 * XXX:	Does it matter that we're discarding virtual address
	 *	space off the end with no record of how much there was?
	 */
	numslots = seg->s_size / ptob(SEGU_PAGES);
	seg->s_size = numslots * ptob(SEGU_PAGES);

	/*
	 * Allocate segment-specific information.
	 */
	seg->s_data = new_kmem_alloc(sizeof (struct segu_segdata), KMEM_SLEEP);
	sdp = (struct segu_segdata *)seg->s_data;

	/*
	 * Allocate the slot array.
	 */
	sdp->usd_slots = (struct segu_data *)new_kmem_alloc(
			numslots * sizeof (struct segu_data), KMEM_SLEEP);

	/*
	 * Set up the slot free list, marking each slot as unallocated.
	 * Note that the list must be sorted in ascending address order.
	 */
	sdp->usd_slots[0].su_flags = 0;
	for (i = 1; i < numslots; i++) {
		sdp->usd_slots[i - 1].su_next = &sdp->usd_slots[i];
		sdp->usd_slots[i].su_flags = 0;
	}
	sdp->usd_slots[numslots - 1].su_next = NULL;
	sdp->usd_free = sdp->usd_slots;

	seg->s_ops = &segu_ops;
	return (0);
}

/*
 * Allocate resources for a single slot.
 *
 * When used for u-area, called at process creation time.
 */
addr_t
segu_get()
{
	struct segu_segdata	*sdp = (struct segu_segdata *)segu->s_data;
	struct page		*pp;
	addr_t			vbase;
	addr_t			va;
	struct segu_data	*sup;
	int			slot;
	int			i;

	/*
	 * Allocate virtual space.  This amounts to grabbing a free slot.
	 */
	if ((sup = sdp->usd_free) == NULL)
		return (NULL);
	sdp->usd_free = sup->su_next;
	slot = sup - sdp->usd_slots;

	vbase = segu->s_base + ptob(SEGU_PAGES) * slot;

	/*
	 * If this slot has anon resources left over from its last use, free
	 * them.  (Normally, segu_release will have cleaned up; however, i/o
	 * in progress at the time of the call prevents it from doing so.)
	 */
	if (sup->su_flags & SEGU_HASANON) {
		anon_free(sup->su_swaddr, ptob(SEGU_PAGES));
		anon_unresv(ptob(SEGU_PAGES - 1));
		sup->su_flags &= ~SEGU_HASANON;
	}

	/*
	 * Reserve sufficient swap space for this slot.  We'll
	 * actually allocate it in the loop below, but reserving it
	 * here allows us to back out more gracefully than if we
	 * had an allocation failure in the body of the loop.
	 *
	 * Note that we don't need swap space for the red zone page.
	 */
	if (anon_resv(ptob(SEGU_PAGES - 1)) == 0) {
		if (segu_debug)
			printf("segu_get: no swap space available\n");
		sup->su_next = sdp->usd_free;
		sdp->usd_free = sup;
		return (NULL);
	}

	/*
	 * Allocate pages, avoiding allocating one for the red zone.
	 */
	pp = rm_allocpage(segu, segu_stom(vbase), ptob(SEGU_PAGES - 1), 1);
	if (pp == NULL) {
		if (segu_debug)
			printf("segu_get: no pages available\n");
		/*
		 * Give back the resources we've acquired.
		 */
		anon_unresv(ptob(SEGU_PAGES - 1));
		sup->su_next = sdp->usd_free;
		sdp->usd_free = sup;
		return (NULL);
	}

	/*
	 * Allocate swap space.
	 *
	 * Because the interface for getting swap slots is designed
	 * to handle only one page at a time, we must deal with each
	 * page in the u-area individually instead of allocating a
	 * contiguous chunk of swap space for the whole thing as we
	 * would prefer.
	 *
	 * This being the case, we actually do more in this loop than
	 * simply allocate swap space.  As we handle each page, we
	 * complete its setup.
	 */
	for (i = 0, va = vbase; i < SEGU_PAGES; i++, va += ptob(1)) {
		register struct anon	*ap;
		struct vnode		*vp;
		u_int			off;
		struct	page		*opp;

		/*
		 * If this page is the red zone page, we don't need swap
		 * space for it.  Note that we skip over the code that
		 * establishes MMU mappings, so that the page remains
		 * invalid.
		 */
		if (i == SEGU_REDZONE) {
			sup->su_swaddr[i] = NULL;
			continue;
		}

		/*
		 * Sanity check.
		 */
		if (pp == NULL)
			panic("segu_get: not enough pages");

		/*
		 * Get a swap slot.
		 */
		if ((ap = anon_alloc()) == NULL)
			panic("segu_get: swap allocation failure");
		sup->su_swaddr[i] = ap;

		/*
		 * Tie the next page to the swap slot.
		 */
		swap_xlate(ap, &vp, &off);
		while (page_enter(pp, vp, off)) {
			/*
			 * The page was already tied to something
			 * else that we have no record of.  Since
			 * the page we wish be named by <vp, off>
			 * already exists, we abort the old page.
			 */
			struct page	*p1 = page_find(vp, off);

			if (p1 != NULL) {
				page_wait(p1);
				if (p1->p_vnode == vp && p1->p_offset == off)
					page_abort(p1);
			}
		}

		/*
		 * Page_enter has set the page's lock bit.  Since it's
		 * kept as well, this is just a nuisance.
		 */
		page_unlock(pp);

		/*
		 * Mark the page for long term keep and release the
		 * short term claim that rm_allocpage established.
		 *
		 * XXX:	When page_pp_lock returns a success/failure
		 *	indication, we'll probably want to panic if
		 *	it fails.
		 */
		(void) page_pp_lock(pp, 0, 1);

		/*
		 * Load and lock an MMU translation for the page.
		 */
		hat_memload(segu, va, pp, UPAGE_PROT, 1);

		/*
		 * Prepare to use the next page.
		 */
		opp = pp;
		page_sub(&pp, pp);
		PAGE_RELE(opp);
	}

	/*
	 * Finally, mark this slot as allocated, locked, and in posession
	 * of anon resources.
	 */
	sup->su_flags = SEGU_ALLOCATED | SEGU_LOCKED | SEGU_HASANON;

	/*
	 * Return the address of the base of the mapped part of
	 * the slot.
	 */
	return (segu_stom(vbase));
}

/*
 * Reclaim resources for a single slot.
 *
 * When used for u-area, called at process destruction time.  Guaranteed not
 * to sleep, so that it can be called while running on the interrupt stack.
 *
 * N.B.: Since this routine deallocates all of the slot's resources,
 * callers can't count on the resources remaining accessible.  In
 * particular, any stack contained in the slot will vanish, so we'd
 * better not be running on that stack.
 *
 * N.B.: Since the routine can't sleep, it must defer deallocation of anon
 * resources associated with pages that have i/o in progress.  (Anon_decref
 * calls page_abort, which will sleep until the i/o is complete.)
 *
 * We can't simply undo everything that segu_get did directly,
 * because someone else may have acquired a reference to one or
 * more of the associated pages in the meantime.
 */
void
segu_release(vaddr)
	addr_t	vaddr;
{
	struct segu_segdata	*sdp = (struct segu_segdata *)segu->s_data;
	addr_t			vbase = segu_mtos(vaddr);
	addr_t			va;
	struct segu_data	*sup;
	struct segu_data	**supp;
	int			slot;
	int			i;
	int			doing_io = 0;
	register int		locked;

	/*
	 * Get the slot corresponding to this virtual address.
	 */
	if ((slot = segu_getslot(segu, vaddr, 1)) == -1)
		panic("segu_release: bad addr");
	sup = &sdp->usd_slots[slot];

	/*
	 * XXX:	Do we need to lock this slot's pages while we're
	 *	messing with them?  What can happen once we decrement
	 *	the keep count below?
	 */

	/*
	 * Examine the slot's pages looking for i/o in progress.
	 * While doing so, undo locks.
	 */
	locked = sup->su_flags & SEGU_LOCKED;
	for (i = 0, va = vbase; i < SEGU_PAGES; i++, va += ptob(1)) {
		register struct page	*pp;
		struct vnode		*vp;
		u_int			off;
		register int		s;

		if (i == SEGU_REDZONE)
			continue;

		if (locked)
			hat_unlock(segu, va);

		/*
		 * Find the page associated with this part of the
		 * slot, tracking it down through its associated swap
		 * space.
		 */
		swap_xlate(sup->su_swaddr[i], &vp, &off);

		/*
		 * Prevent page status from changing.
		 */
		s = splvm();

		if ((pp = page_exists(vp, off)) == NULL) {
			/*
			 * The page no longer exists; this is fine
			 * unless we had it locked.
			 */
			if (locked)
				panic("segu_release: missing locked page");
			else
				continue;
		}

		/*
		 * See whether the page is quiescent.
		 */
		if (pp->p_keepcnt != 0)
			doing_io = 1;

		/*
		 * Make this page available to vultures.
		 */
		if (locked)
			page_pp_unlock(pp, 0);

		(void) splx(s);
	}

	/*
	 * Unload the mmu translations for this slot.
	 */
	hat_unload(segu, vaddr, ptob(SEGU_PAGES - 1));

	/*
	 * Provided that all of the pages controlled by this segment are
	 * quiescent, release our claim on the associated anon resources and
	 * swap space.
	 */
	if (!doing_io) {
		anon_free(sup->su_swaddr, ptob(SEGU_PAGES));
		anon_unresv(ptob(SEGU_PAGES - 1));
		sup->su_flags &= ~SEGU_HASANON;
	} else
		sup->su_flags |= SEGU_HASANON;

	/*
	 * Mark the slot as unallocated and unlocked and put it back on the
	 * free list.  Keep the free list sorted by slot address, to minimize
	 * fragmentation of seg_u's virtual address range.  (This makes a
	 * difference on some architectures; e.g., by making it possible to
	 * use fewer page table entries.)  This code counts on the slot
	 * address being a monotonically increasing function of indices of
	 * entries in the usd_slots array.
	 */
	sup->su_flags &= ~(SEGU_ALLOCATED|SEGU_LOCKED);
	for (supp = &sdp->usd_free; *supp != NULL && *supp < sup;
	    supp = &(*supp)->su_next)
		continue;
	sup->su_next = *supp;
	*supp = sup;
}


/*
 * Private routines for use by seg_u operations.
 */

/*
 * Verify that the range designated by vaddr and len lies completely
 * within the mapped part of a single allocated slot.  If so, return
 * the slot's index; otherwise return -1.
 */
static int
segu_getslot(seg, vaddr, len)
	register struct seg	*seg;
	addr_t			vaddr;
	u_int			len;
{
	register int			slot;
	register struct segu_segdata	*sdp;
	register struct segu_data	*sup;
	addr_t				vlast;
	addr_t				vmappedbase;

	sdp = (struct segu_segdata *)seg->s_data;

	/*
	 * Make sure the base is in range of the segment as a whole.
	 */
	if (vaddr < seg->s_base || vaddr >= seg->s_base + seg->s_size)
		return (-1);

	/*
	 * Figure out what slot the address lies in.
	 */
	slot = (vaddr - seg->s_base) / ptob(SEGU_PAGES);
	sup = &sdp->usd_slots[slot];

	/*
	 * Make sure the end of the range falls in the same slot.
	 */
	vlast = vaddr + len - 1;
	if ((vlast - seg->s_base) / ptob(SEGU_PAGES) != slot)
		return (-1);

	/*
	 * Nobody has any business touching this slot if it's not currently
	 * allocated.
	 */
	if (!(sup->su_flags & SEGU_ALLOCATED))
		return (-1);

	/*
	 * Finally, verify that the range is completely in the mapped part
	 * of the slot.
	 */
	vmappedbase = segu_stom(seg->s_base + ptob(SEGU_PAGES) * slot);
	if (vaddr < vmappedbase || vlast >= vmappedbase + ptob(SEGU_PAGES - 1))
		return (-1);

	return (slot);
}

/*
 * Unlock intra-slot resources in the range given by vaddr and len.
 * Assumes that the range is known to fall entirely within the mapped
 * part of the slot given as argument and that the slot itself is
 * allocated.
 */
static int
segu_softunlock(seg, vaddr, len, slot, rw)
	struct seg	*seg;
	addr_t		vaddr;
	u_int		len;
	int		slot;
	enum seg_rw	rw;
{
	struct segu_segdata	*sdp = (struct segu_segdata *)segu->s_data;
	register struct segu_data
				*sup = &sdp->usd_slots[slot];
	register addr_t		va;
	addr_t			vlim;
	register u_int		i;

	/*
	 * Loop through the pages in the given range.
	 */
	va = (addr_t)((u_int)vaddr & PAGEMASK);
	len = roundup(len, ptob(1));
	vlim = va + len;
	/* Calculate starting page index within slot. */
	i = (va - (seg->s_base + slot * ptob(SEGU_PAGES))) / ptob(1);
	for ( ; va < vlim; va += ptob(1), i++) {
		register struct page	*pp;
		struct vnode		*vp;
		u_int			off;

		/*
		 * Unlock our MMU translation for this page.
		 *
		 * XXX:	Is there any problem with attempting to unlock
		 *	a translation that isn't locked?
		 */
		hat_unlock(seg, va);

		/*
		 * Unload it.
		 */
		hat_unload(seg, va, ptob(1));

		/*
		 * Find the page associated with this part of the
		 * slot, tracking it down through its associated swap
		 * space.
		 */
		swap_xlate(sup->su_swaddr[i], &vp, &off);
		if ((pp = page_find(vp, off)) == NULL)
			panic("segu_softunlock: missing page");

		/*
		 * Release our long-term claim on the page.
		 */
		page_pp_unlock(pp, 0);

		/*
		 * If we're "hard" swapping (i.e. we need pages) and
		 * nobody's using the page any more and it's dirty,
		 * unlocked, and not kept, push it asynchronously rather
		 * than waiting for the pageout daemon to find it.
		 */
		hat_pagesync(pp);
		if (rw == S_WRITE && pp->p_mapping == NULL &&
		    pp->p_keepcnt == 0 && !pp->p_lock && pp->p_mod) {
			/*
			 * XXX:	Want most powerful credentials we can
			 *	get.  Punt for now.
			 */
			(void) VOP_PUTPAGE(vp, off, ptob(1), B_ASYNC | B_FREE,
				(struct ucred *)NULL);
		}
	}

	return (0);
}

/*
 * Load and possibly lock intra-slot resources in the range given
 * by vaddr and len.  Assumes that the range is known to fall entirely
 * within the mapped part of the slot given as argument and that the
 * slot itself is allocated.
 */
static int
segu_softload(seg, vaddr, len, slot, lock)
	struct seg	*seg;
	addr_t		vaddr;
	u_int		len;
	int		slot;
	int		lock;
{
	struct segu_segdata	*sdp = (struct segu_segdata *)segu->s_data;
	register struct segu_data
				*sup = &sdp->usd_slots[slot];
	register addr_t		va;
	addr_t			vlim;
	register u_int		i;

	/*
	 * Loop through the pages in the given range.
	 */
	va = (addr_t)((u_int)vaddr & PAGEMASK);
	vaddr = va;
	len = roundup(len, ptob(1));
	vlim = va + len;
	/* Calculate starting page index within slot. */
	i = (va - (seg->s_base + slot * ptob(SEGU_PAGES))) / ptob(1);
	for ( ; va < vlim; va += ptob(1), i++) {
		struct page	*pl[2];
		struct vnode	*vp;
		u_int		off;
		register int	err;

		/*
		 * Summon the page.  If it's not resident, arrange
		 * for synchronous i/o to pull it in.
		 *
		 * XXX:	Need read credentials value; for now we punt.
		 */
		swap_xlate(sup->su_swaddr[i], &vp, &off);
		err = VOP_GETPAGE(vp, off, ptob(1), (u_int *)NULL,
			pl, ptob(1), seg, va, S_READ, (struct ucred *)NULL);
		if (err) {
			/*
			 * Back out of what we've done so far.
			 */
			(void) segu_softunlock(seg, vaddr, (u_int)(va - vaddr),
			    slot, S_OTHER);
			return (err);
		}
		cnt.v_swpin++;
		/*
		 * The returned page list will have exactly one entry,
		 * which is returned to us already kept.
		 */

		/*
		 * Load an MMU translation for the page.
		 */
		hat_memload(seg, va, pl[0], UPAGE_PROT, lock);

		/*
		 * If we're locking down resources, we need to increment
		 * the page's long term keep count.  In any event, we
		 * need to decrement the (short term) keep count.
		 *
		 * XXX:	When page_pp_lock returns a success/failure
		 *	indication, we'll probably want to panic if
		 *	it fails.
		 */
		if (lock)
			(void) page_pp_lock(pl[0], 0, 1);
		PAGE_RELE(pl[0]);
	}

	return (0);
}
