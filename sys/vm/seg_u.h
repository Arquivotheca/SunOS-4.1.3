/*	@(#)seg_u.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * VM - U-area segment management
 *
 * This file contains definitions related to the u-area segment type.
 *
 * In its most general form, this segment type provides an interface
 * for managing stacks that are protected by red zones, with the size
 * of each stack independently specifiable.  The current implementation
 * is restricted in the following way.
 * 1)	It assumes that all stacks are the same size.  In particular,
 *	it assumes that the stacks it manages are actually traditional
 *	u-areas, each containing a stack at one end.
 *
 * The segment driver manages a contiguous chunk of virtual space,
 * carving it up into individual stack instances as required, and
 * associating physical storage, MMU mappings, and swap space with
 * each individual stack instance.
 *
 * As a matter of nomenclature, the individual allocation units are
 * referred to as "slots".
 */

#ifndef _vm_seg_u_h
#define	_vm_seg_u_h

/*
 * The number of pages covered by a single seg_u slot.
 *
 * This value is the number of (software) pages in the u-area
 * (including the stack in the u-area) plus an additional page
 * for a stack red zone.  If the seg_u implementation is ever
 * generalized to allow variable-size stack allocation, this
 * define will have to change.
 */
#define	SEGU_PAGES	(UPAGES/CLSIZE + 1)

/*
 * XXX:	This define belongs elsewhere, probably in <machine/param.h>.
 */
#define	STACK_GROWTH_DOWN


/*
 * Index of the red zone page and macros for interconverting between
 * the base address of a slot and the base address of its accessible
 * portion.  (Nomenclature: Slot TO Mapped and vice versa.)
 */
#ifdef	STACK_GROWTH_DOWN

#define	SEGU_REDZONE	0
#define	segu_stom(v)	((v) + ptob(1))
#define	segu_mtos(v)	((v) - ptob(1))

#else	STACK_GROWTH_DOWN

#define	SEGU_REDZONE	(SEGU_PAGES - 1)
#define	segu_stom(v)	(v)
#define	segu_mtos(v)	(v)

#endif	STACK_GROWTH_DOWN


/*
 * Private information per overall segu segment (as opposed
 * to per slot within segment)
 *
 * XXX:	We may wish to modify the free list to handle it as a queue
 *	instead of a stack; this possibly could reduce the frequency
 *	of cache flushes.  If so, we would need a list tail pointer
 *	as well as a list head pointer.
 */
struct segu_segdata {
	/*
	 * info needed:
	 *	- slot vacancy info
	 *	- a way of getting to state info for each slot
	 */
	struct	segu_data *usd_slots;	/* array of segu_data structs, */
					/*   one per slot */
	struct	segu_data *usd_free;	/* slot free list head */
};

/*
 * Private per-slot information.
 */
struct segu_data {
	struct	segu_data *su_next;		/* free list link */
	struct	anon *su_swaddr[SEGU_PAGES];	/* disk address of u area */
						/*   when swapped */
	u_int	su_flags;			/* state info: see below */
};

/*
 * Flag bits
 *
 * When the SEGU_LOCKED bit is set, all the resources associated with the
 * corresponding slot are locked in place, so that referencing addresses
 * in the slot's range will not cause a fault.  Clients using this driver
 * to manage a u-area lock down the slot when the corresponding process
 * becomes runnable and unlock it when the process is swapped out.
 */
#define	SEGU_ALLOCATED	0x01		/* slot is in use */
#define	SEGU_LOCKED	0x02		/* slot's resources locked */
#define	SEGU_HASANON	0x04		/* slot has anon resources */


#ifdef	KERNEL
extern struct seg	*segu;

/*
 * Public routine declarations not part of the segment ops vector go here.
 */
int	segu_create(/* seg, argsp */);
addr_t	segu_get();
void	segu_release(/* vaddr */);

/*
 * We allow explicit calls to segu_fault, even though it's part
 * of the segu ops vector.
 */
faultcode_t	segu_fault(/* seg, vaddr, len, type, rw */);
#endif	KERNEL

#endif /*!_vm_seg_u_h*/
