/*	@(#)vm_as.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - address spaces.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/mman.h>

#include <machine/mmu.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>

/*
 * Variables for maintaining the free list of address space structures.
 */
static struct as *as_freelist;
static int as_freeincr = 8;

/*
 * Find a segment containing addr.  as->a_seglast is used as a
 * cache to remember the last segment hit we had here.  We
 * first check to see if seglast is another hit, and if not we
 * determine whether to start from the head of the segment list
 * (as->a_segs) or from seglast and in which direction to search.
 */
struct seg *
as_segat(as, addr)
	register struct as *as;
	register addr_t addr;
{
	register struct seg *seg, *sseg;
	register forward;

	if (as->a_segs == NULL)		/* address space has no segments */
		return (NULL);
	if (as->a_seglast == NULL)
		as->a_seglast = as->a_segs;
	seg = as->a_seglast;
	forward = 0;
	if (seg->s_base <= addr) {
		if (addr < (seg->s_base + seg->s_size))
			return (seg);	/* seglast contained addr */
		sseg = as->a_segs->s_prev;
		if ((addr - seg->s_base) >
				((sseg->s_base + sseg->s_size) - addr)) {
			seg = sseg;
			sseg = as->a_seglast;
		} else {
			seg = as->a_seglast->s_next;
			sseg = as->a_segs;
			forward++;
		}
	} else {
		if ((addr - as->a_segs->s_base) > (seg->s_base - addr)) {
			seg = seg->s_prev;
			sseg = as->a_segs->s_prev;
		} else {
			sseg = seg;
			seg = as->a_segs;
			forward++;
		}
	}
	do {
		if (seg->s_base <= addr &&
				addr < (seg->s_base + seg->s_size)) {
			as->a_seglast = seg;
			return (seg);
		}
		if (forward) {
			seg = seg->s_next;
			if (seg->s_base > addr)
				break;
		} else {
			seg = seg->s_prev;
			if (addr > (seg->s_base + seg->s_size))
				break;
		}
	} while (seg != sseg);
	return (NULL);
}

/*
 * Allocate and initialize an address space data structure.
 * We call hat_alloc to allow any machine dependent
 * information in the hat structure to be initialized.
 */
struct as *
as_alloc()
{
	struct as *as;

	as = (struct as *)new_kmem_fast_alloc((caddr_t *)&as_freelist,
	    sizeof (*as_freelist), as_freeincr, KMEM_SLEEP);
	bzero((caddr_t)as, sizeof (*as));
	hat_alloc(as);
	return (as);
}

/*
 * Free an address space data structure.
 * Need to free the hat first and then
 * all the segments on this as and finally
 * the space for the as struct itself.
 */
void
as_free(as)
	struct as *as;
{
	hat_free(as);
	while (as->a_segs != NULL)
		seg_free(as->a_segs);
	kmem_fast_free((caddr_t *)&as_freelist, (caddr_t)as);
}

struct as *
as_dup(as)
	register struct as *as;
{
	register struct as *newas;
	register struct seg *seg, *sseg, *newseg;

	newas = as_alloc();
	sseg = seg = as->a_segs;
	if (seg != NULL) {
		do {
			newseg = seg_alloc(newas, seg->s_base, seg->s_size);
			if (newseg == NULL) {
				as_free(newas);
				return (NULL);
			}
			if ((*seg->s_ops->dup)(seg, newseg)) {
				as_free(newas);
				return (NULL);
			}
			seg = seg->s_next;
		} while (seg != sseg);
	}
	return (newas);
}

/*
 * Add a new segment to the address space, sorting
 * it into the proper place in the linked list.
 */
enum as_res
as_addseg(as, new)
	register struct as *as;
	register struct seg *new;
{
	register struct seg *seg;
	register addr_t base;

	seg = as->a_segs;
	if (seg == NULL) {
		new->s_next = new->s_prev = new;
		as->a_segs = new;
	} else {
		/*
		 * Figure out where to add the segment to keep list sorted
		 */
		base = new->s_base;
		do {
			if (base < seg->s_base) {
				if (base + new->s_size > seg->s_base)
					return (A_BADADDR);
				break;
			}
			if (base < seg->s_base + seg->s_size)
				return (A_BADADDR);
			seg = seg->s_next;
		} while (seg != as->a_segs);

		new->s_next = seg;
		new->s_prev = seg->s_prev;
		seg->s_prev = new;
		new->s_prev->s_next = new;

		if (base < as->a_segs->s_base)
			as->a_segs = new;		/* new is at front */
	}
	return (A_SUCCESS);
}

/*
 * Handle a ``fault'' at addr for size bytes.
 */
faultcode_t
as_fault(as, addr, size, type, rw)
	struct as *as;
	addr_t addr;
	u_int size;
	enum fault_type type;
	enum seg_rw rw;
{
	register struct seg *seg;
	register addr_t raddr;			/* rounded addr counter */
	register u_int rsize;			/* rounded size counter */
	register u_int ssize;
	register addr_t addrsav;
	struct seg *segsav;
	faultcode_t res = 0;

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (FC_NOMAP);

	addrsav = raddr;
	segsav = seg;

	for (; rsize != 0; rsize -= ssize, raddr += ssize) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base) {
				res = FC_NOMAP;
				break;
			}
		}
		if (raddr + rsize > seg->s_base + seg->s_size)
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;
		res = (*seg->s_ops->fault)(seg, raddr, ssize, type, rw);
		if (res != 0)
			break;
	}

	/*
	 * If we failed and we were locking, unlock the pages we faulted.
	 * (Maybe we should just panic if we are SOFTLOCKing
	 * or even SOFTUNLOCKing right here...)
	 */
	if (res != 0 && type == F_SOFTLOCK) {
		for (seg = segsav; addrsav < raddr; addrsav += ssize) {
			if (addrsav >= seg->s_base + seg->s_size)
				seg = seg->s_next;	/* goto next seg */
			/*
			 * Now call the fault routine again to perform the
			 * unlock using S_OTHER instead of the rw variable
			 * since we never got a chance to touch the pages.
			 */
			if (raddr > seg->s_base + seg->s_size)
				ssize = seg->s_base + seg->s_size - addrsav;
			else
				ssize = raddr - addrsav;
			(void) (*seg->s_ops->fault)(seg, addrsav, ssize,
			    F_SOFTUNLOCK, S_OTHER);
		}
	}

	return (res);
}

/*
 * Asynchronous ``fault'' at addr for size bytes.
 */
faultcode_t
as_faulta(as, addr, size)
	struct as *as;
	addr_t addr;
	u_int size;
{
	register struct seg *seg;
	register addr_t raddr;			/* rounded addr counter */
	register u_int rsize;			/* rounded size counter */
	faultcode_t res;

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (FC_NOMAP);
	for (; rsize != 0; rsize -= PAGESIZE, raddr += PAGESIZE) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base)
				return (FC_NOMAP);
		}
		res = (*seg->s_ops->faulta)(seg, raddr);
		if (res != 0)
			return (res);
	}
	return (0);
}

/*
 * Set the virtual mapping for the interval from [addr : addr + size)
 * in address space `as' to have the specified protection.
 * It is ok for the range to cross over several segments,
 * as long as they are contiguous.
 */
enum as_res
as_setprot(as, addr, size, prot)
	struct as *as;
	addr_t addr;
	u_int size;
	u_int prot;
{
	register struct seg *seg;
	register u_int ssize;
	register addr_t raddr;			/* rounded addr counter */
	register u_int rsize;			/* rounded size counter */
	enum as_res res = A_SUCCESS;

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (A_BADADDR);
	for (; rsize != 0; rsize -= ssize, raddr += ssize) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base) {
				res = A_BADADDR;
				break;
			}
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;
		if ((*seg->s_ops->setprot)(seg, raddr, ssize, prot) != 0)
			res = A_OPFAIL;		/* keep on going */
	}
	return (res);
}

/*
 * Check to make sure that the interval from [addr : addr + size)
 * in address space `as' has at least the specified protection.
 * It is ok for the range to cross over several segments, as long
 * as they are contiguous.
 */
enum as_res
as_checkprot(as, addr, size, prot)
	struct as *as;
	addr_t addr;
	u_int size;
	u_int prot;
{
	register struct seg *seg;
	register u_int ssize;
	register addr_t raddr;			/* rounded addr counter */
	register u_int rsize;			/* rounded size counter */

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (A_BADADDR);
	for (; rsize != 0; rsize -= ssize, raddr += ssize) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base)
				return (A_BADADDR);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;
		if ((*seg->s_ops->checkprot)(seg, raddr, ssize, prot) != 0)
			return (A_OPFAIL);
	}
	return (A_SUCCESS);
}

enum as_res
as_unmap(as, addr, size)
	register struct as *as;
	addr_t addr;
	u_int size;
{
	register struct seg *seg, *seg_next;
	register addr_t raddr, eaddr;
	register u_int ssize;
	addr_t obase;

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	eaddr = (addr_t)(((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK);

	seg = as->a_segs;
	if (seg != NULL) {
		for (; raddr < eaddr; seg = seg_next) {
			/*
			 * Save next segment pointer since seg can be
			 * destroyed during the segment unmap operation.
			 * We also have to save the old base below.
			 */
			seg_next = seg->s_next;

			if (raddr >= seg->s_base + seg->s_size) {
				if (seg->s_base >= seg_next->s_base)
					break;		/* looked at all segs */
				continue;		/* not there yet */
			}

			if (eaddr <= seg->s_base)
				break;			/* all done */

			if (raddr < seg->s_base)
				raddr = seg->s_base;	/* skip to seg start */

			if (eaddr > (seg->s_base + seg->s_size))
				ssize = seg->s_base + seg->s_size - raddr;
			else
				ssize = eaddr - raddr;

			obase = seg->s_base;
			if ((*seg->s_ops->unmap)(seg, raddr, ssize) != 0)
				return (A_OPFAIL);
			raddr += ssize;

			/*
			 * Carefully check to see if we
			 * have looked at all the segments.
			 */
			if (as->a_segs == NULL || obase >= seg_next->s_base)
				break;
		}
	}

	return (A_SUCCESS);
}

int
as_map(as, addr, size, crfp, argsp)
	struct as *as;
	addr_t addr;
	u_int size;
	int (*crfp)();
	caddr_t argsp;
{
	register struct seg *seg;
	enum as_res res;
	int error;

	seg = seg_alloc(as, addr, size);
	if (seg == NULL)
		return (ENOMEM);

	/*
	 * Remember that this was the most recently touched segment.
	 * If the create routine merges this segment into an existing
	 * segment, seg_free will adjust the a_seglast hint.
	 */
	as->a_seglast = seg;
	error = (*crfp)(seg, argsp);
	/*
	 * If some error occurred during the create function, destroy
	 * this segment.  Otherwise, if the address space is locked,
	 * establish memory locks for the new segment.  Translate
	 * error returns as appropriate.
	 */
	if (error)
		seg_free(seg);
	else if (as->a_paglck) {
		res = as_ctl(as, seg->s_base, seg->s_size, MC_LOCK, (caddr_t)0);
		if (res == A_RESOURCE)
			error = EAGAIN;
		else if (res != A_SUCCESS)
			error = EIO;
		if (error)
			(void) as_unmap(as, addr, size);
	}
	return (error);
}

/*
 * Find a hole of at least size minlen within [base, base+len).
 * If flags specifies AH_HI, the hole will have the highest possible address
 * in the range. Otherwise, it will have the lowest possible address.
 * If flags specifies AH_CONTAIN, the hole will contain the address addr.
 * If an adequate hole is found, base and len are set to reflect the part of
 * the hole that is within range, and A_SUCCESS is returned. Otherwise,
 * A_OPFAIL is returned.
 * XXX This routine is not correct when base+len overflows addr_t.
 */
/* VARARGS5 */
enum as_res
as_hole(as, minlen, basep, lenp, flags, addr)
	struct as *as;
	register u_int minlen;
	addr_t *basep;
	u_int *lenp;
	int flags;
	addr_t addr;
{
	register addr_t	lobound = *basep;
	register addr_t	hibound = lobound + *lenp;
	register struct seg *sseg = as->a_segs;
	register struct seg *lseg, *hseg;
	register addr_t lo, hi;
	register int forward;

	if (sseg == NULL)
		if (valid_va_range(basep, lenp, minlen, flags & AH_DIR))
			return (A_SUCCESS);
		else
			return (A_OPFAIL);

	/*
	 * Set up to iterate over all the inter-segment holes in the given
	 * direction.  lseg is NULL for the lowest-addressed hole and hseg is
	 * NULL for the highest-addressed hole.  If moving backwards, we reset
	 * sseg to denote the highest-addressed segment.
	 */
	forward = (flags & AH_DIR) == AH_LO;
	if (forward) {
		lseg = NULL;
		hseg = sseg;
	} else {
		sseg = sseg->s_prev;
		hseg = NULL;
		lseg = sseg;
	}
	for (;;) {
		/*
		 * Set lo and hi to the hole's boundaries.  (We should really
		 * use MAXADDR in place of hibound in the expression below,
		 * but can't express it easily; using hibound in its place is
		 * harmless.)
		 */
		lo = (lseg == NULL) ? 0 : lseg->s_base + lseg->s_size;
		hi = (hseg == NULL) ? hibound : hseg->s_base;
		/*
		 * If the iteration has moved past the interval from lobound
		 * to hibound it's pointless to continue.
		 */
		if ((forward && lo > hibound) || (!forward && hi < lobound))
			break;
		else if (lo > hibound || hi < lobound)
			goto cont;
		/*
		 * Candidate hole lies at least partially within the allowable
		 * range.  Restrict it to fall completely within that range,
		 * i.e., to [max(lo, lobound), min(hi, hibound)).
		 */
		if (lo < lobound)
			lo = lobound;
		if (hi > hibound)
			hi = hibound;
		/*
		 * Verify that the candidate hole is big enough and meets
		 * hardware constraints.
		 */
		*basep = lo;
		*lenp = hi - lo;
		if (valid_va_range(basep, lenp, minlen,
		    forward ? AH_LO : AH_HI) &&
		    ((flags & AH_CONTAIN) == 0 ||
		    (*basep <= addr && *basep + *lenp > addr)))
			return (A_SUCCESS);

	cont:
		/*
		 * Move to the next hole.
		 */
		if (forward) {
			lseg = hseg;
			if (lseg == NULL)
				break;
			hseg = hseg->s_next;
			if (hseg == sseg)
				hseg = NULL;
		} else {
			hseg = lseg;
			if (hseg == NULL)
				break;
			lseg = lseg->s_prev;
			if (lseg == sseg)
				lseg = NULL;
		}
	}
	return (A_OPFAIL);
}

/*
 * Return the next range within [base, base+len) that is backed
 * with "real memory".  Skip holes and non-seg_vn segments.
 * We're lazy and only return one segment at a time.
 */
enum as_res
as_memory(as, basep, lenp)
	struct as *as;
	addr_t *basep;
	u_int *lenp;
{
	register struct seg *seg, *sseg, *cseg = NULL;
	register addr_t addr, eaddr, segend;

	/* XXX - really want as_segatorabove? */
	if (as->a_seglast == NULL)
		as->a_seglast = as->a_segs;

	addr = *basep;
	eaddr = addr + *lenp;
	sseg = seg = as->a_seglast;
	if (seg != NULL) {
		do {
			if (seg->s_ops != &segvn_ops)
				continue;
			if (seg->s_base <= addr &&
			    addr < (segend = (seg->s_base + seg->s_size))) {
				/* found a containing segment */
				as->a_seglast = seg;
				*basep = addr;
				if (segend > eaddr)
					*lenp = eaddr - addr;
				else
					*lenp = segend - addr;
				return (A_SUCCESS);
			} else if (seg->s_base > addr) {
				if (cseg == NULL ||
				    cseg->s_base > seg->s_base)
					/* save closest seg above */
					cseg = seg;
			}
		} while ((seg = seg->s_next) != sseg);
	}
	if (cseg == NULL)		/* ??? no segments in address space? */
		return (A_OPFAIL);

	/*
	 * Only found a close segment, see if there's
	 * a valid range we can return.
	 */
	if (cseg->s_base > eaddr)
		return (A_BADADDR);	/* closest segment is out of range */
	as->a_seglast = cseg;
	*basep = cseg->s_base;
	if (cseg->s_base + cseg->s_size > eaddr)
		*lenp = eaddr - cseg->s_base;	/* segment contains eaddr */
	else
		*lenp = cseg->s_size;	/* segment is between addr and eaddr */
	return (A_SUCCESS);
}

/*
 * Swap the pages associated with the address space as out to
 * secondary storage, returning the number of bytes actually
 * swapped.
 *
 * If we are not doing a "hard" swap (i.e. we're just getting rid
 * of a deawood process) unlock the segu making it available to be
 * paged out.
 *
 * The value returned is intended to correlate well with the process's
 * memory requirements.  Its usefulness for this purpose depends on
 * how well the segment-level routines do at returning accurate
 * information.
 */
u_int
as_swapout(as, hardswap)
	struct as *as;
	short hardswap;
{
	register struct seg *seg, *sseg;
	register u_int swpcnt = 0;

	/*
	 * Kernel-only processes have given up their address
	 * spaces.  Of course, we shouldn't be attempting to
	 * swap out such processes in the first place...
	 */
	if (as == NULL)
		return (0);

	/*
	 * Free all mapping resources associated with the address
	 * space.  The segment-level swapout routines capitalize
	 * on this unmapping by scavanging pages that have become
	 * unmapped here.
	 */
	hat_free(as);

	/*
	 * Call the swapout routines of all segments in the address
	 * space to do the actual work, accumulating the amount of
	 * space reclaimed.
	 */
	sseg = seg = as->a_segs;
	if (hardswap && seg != NULL) {
		do {
			register struct seg_ops *ov = seg->s_ops;

		/* for "soft" swaps, should we sync out segment instead? XXX */
			if (ov->swapout != NULL)
				swpcnt += (*ov->swapout)(seg);
		} while ((seg = seg->s_next) != sseg);
	}

	return (swpcnt);
}

/*
 * Determine whether data from the mappings in interval [addr : addr + size)
 * are in the primary memory (core) cache.
 */
enum as_res
as_incore(as, addr, size, vec, sizep)
	struct as *as;
	addr_t addr;
	u_int size;
	char *vec;
	u_int *sizep;
{
	register struct seg *seg;
	register u_int ssize;
	register addr_t raddr;		/* rounded addr counter */
	register u_int rsize;		/* rounded size counter */
	u_int isize;			/* iteration size */

	*sizep = 0;
	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = ((((u_int)addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;
	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (A_BADADDR);
	for (; rsize != 0; rsize -= ssize, raddr += ssize) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;
			if (raddr != seg->s_base)
				return (A_BADADDR);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;
		*sizep += isize =
		    (*seg->s_ops->incore)(seg, raddr, ssize, vec);
		if (isize != ssize)
			return (A_OPFAIL);
		vec += btoc(ssize);
	}
	return (A_SUCCESS);
}

/*
 * Cache control operations over the interval [addr : addr + size) in
 * address space "as".
 */
enum as_res
as_ctl(as, addr, size, func, arg)
	struct as *as;
	addr_t addr;
	u_int size;
	int func;
	caddr_t arg;
{
	register struct seg *seg;	/* working segment */
	register struct seg *fseg;	/* first segment of address space */
	register u_int ssize;		/* size of seg */
	register addr_t raddr;		/* rounded addr counter */
	register u_int rsize;		/* rounded size counter */
	enum as_res res;		/* recursive result */
	int r;				/* local result */

	/*
	 * Normalize addresses and sizes.
	 */
	raddr = (addr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	/*
	 * If these are address space lock/unlock operations, loop over
	 * all segments in the address space, as appropriate.
	 */
	if ((func == MC_LOCKAS) || (func == MC_UNLOCKAS)) {
		if (func == MC_UNLOCKAS)
			as->a_paglck = 0;
		else {
			if ((int)arg & MCL_FUTURE)
				as->a_paglck = 1;
			if (((int)arg & MCL_CURRENT) == 0)
				return (A_SUCCESS);
		}
		for (fseg = NULL, seg = as->a_segs; seg != fseg;
		    seg = seg->s_next) {
			if (fseg == NULL)
				fseg = seg;
			if ((res = as_ctl(as, seg->s_base, seg->s_size,
			    func == MC_LOCKAS ? MC_LOCK : MC_UNLOCK,
			    (caddr_t)0)) != A_SUCCESS)
				return (res);
		}
		return (A_SUCCESS);
	}

	/*
	 * Get initial segment.
	 */
	if ((seg = as_segat(as, raddr)) == NULL)
		return (A_BADADDR);

	/*
	 * Loop over all segments.  If a hole in the address range is
	 * discovered, then fail.  For each segment, perform the appropriate
	 * control operation.
	 */

	while (rsize != 0) {

		/*
		 * Make sure there's no hole, calculate the portion
		 * of the next segment to be operated over.
		 */
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;
			if (raddr != seg->s_base)
				return (A_BADADDR);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		/*
		 * Dispatch on specific function.
		 */
		switch (func) {

		/*
		 * Synchronize cached data from mappings with backing
		 * objects.
		 */
		case MC_SYNC:
			if (r = (*seg->s_ops->sync)
			    (seg, raddr, ssize, (u_int)arg))
				return (r == EPERM ? A_RESOURCE : A_OPFAIL);
			break;

		/*
		 * Lock pages in memory.
		 */
		case MC_LOCK:
			if (r = (*seg->s_ops->lockop)(seg, raddr, ssize, func))
				return (r == EAGAIN ? A_RESOURCE : A_OPFAIL);
			break;

		/*
		 * Unlock mapped pages.
		 */
		case MC_UNLOCK:
			(void) (*seg->s_ops->lockop)(seg, raddr, ssize, func);
			break;

		/*
		 * Store VM advise for mapped pages in segment layer
		 */
		case MC_ADVISE:
			(void) (*seg->s_ops->advise)(seg, raddr, ssize, arg);
			break;

		/*
		 * Can't happen.
		 */
		default:
			panic("as_ctl");
		}
		rsize -= ssize;
		raddr += ssize;
	}
	return (A_SUCCESS);
}


/*
 * Inform the as of translation information associated with the given addr.
 * This is currently only called if a_hatcallback == 1.
 */
void
as_hatsync(as, addr, ref, mod, flags)
	struct as *as;
	addr_t addr;
	u_int ref;
	u_int mod;
	u_int flags;
{
	struct seg *seg;

	if (seg = as_segat(as, addr))
		seg->s_ops->hatsync(seg, addr, ref, mod, flags);
}
