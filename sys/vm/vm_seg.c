/*	@(#)vm_seg.c	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * VM - segment management.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/mmu.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/mp.h>

/*
 * Variables for maintaining the free list of segment structures.
 */
static struct seg *seg_freelist;
static int seg_freeincr = 24;

/*
 * Allocate a segment to cover [base, base+size)
 * and attach it to the specified address space.
 */
struct seg *
seg_alloc(as, base, size)
	struct as *as;
	register addr_t base;
	register u_int size;
{
	register struct seg *new;
	addr_t segbase;
	u_int segsize;

	segbase = (addr_t)((u_int)base & PAGEMASK);
	segsize =
	    (((u_int)(base + size) + PAGEOFFSET) & PAGEMASK) - (u_int)segbase;

	if (!valid_va_range(&segbase, &segsize, segsize, AH_LO))
		return ((struct seg *)NULL);	/* bad virtual addr range */

	new = (struct seg *)new_kmem_fast_alloc((caddr_t *)&seg_freelist,
	    sizeof (*seg_freelist), seg_freeincr, KMEM_SLEEP);
	bzero((caddr_t)new, sizeof (*new));
	if (seg_attach(as, segbase, segsize, new) < 0) {
		kmem_fast_free((caddr_t *)&seg_freelist, (caddr_t)new);
		return ((struct seg *)NULL);
	}
	/* caller must fill in ops, data */
	return (new);
}

/*
 * Attach a segment to the address space.  Used by seg_alloc()
 * and for kernel startup to attach to static segments.
 */
int
seg_attach(as, base, size, seg)
	struct as *as;
	addr_t base;
	u_int size;
	struct seg *seg;
{

	seg->s_as = as;
	seg->s_base = base;
	seg->s_size = size;
	if (as_addseg(as, seg) == A_SUCCESS)
		return (0);
	return (-1);
}

/*
 * Free the segment from its associated as,
 */
void
seg_free(seg)
	register struct seg *seg;
{
	register struct as *as = seg->s_as;

	if (as->a_segs == seg)
		as->a_segs = seg->s_next;		/* go to next seg */

	if (as->a_segs == seg)
		as->a_segs = NULL;			/* seg list is gone */
	else {
		seg->s_prev->s_next = seg->s_next;
		seg->s_next->s_prev = seg->s_prev;
	}

	if (as->a_seglast == seg)
		as->a_seglast = as->a_segs;

	/*
	 * If the segment private data field is NULL,
	 * then segment driver is not attached yet.
	 */
	if (seg->s_data != NULL)
		(*seg->s_ops->free)(seg);

	kmem_fast_free((caddr_t *)&seg_freelist, (caddr_t)seg);
}

/*
 * Translate addr into page number within segment.
 */
u_int
seg_page(seg, addr)
	struct seg *seg;
	addr_t addr;
{

	return ((u_int)((addr - seg->s_base) >> PAGESHIFT));
}

/*
 * Return number of pages in segment.
 */
u_int
seg_pages(seg)
	struct seg *seg;
{

	return ((u_int)((seg->s_size + PAGEOFFSET) >> PAGESHIFT));
}
