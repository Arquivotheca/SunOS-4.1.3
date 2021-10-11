/*	@(#)seg.h 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _vm_seg_h
#define	_vm_seg_h
#include <vm/faultcode.h>
#include <vm/mp.h>

/*
 * VM - Segments.
 */

/*
 * An address space contains a set of segments, managed by drivers.
 * Drivers support mapped devices, sharing, copy-on-write, etc.
 *
 * The seg structure contains a lock to prevent races, the base virtual
 * address and size of the segment, a back pointer to the containing
 * address space, pointers to maintain a circularly doubly linked list
 * of segments in the same address space, and procedure and data hooks
 * for the driver.  The seg list on the address space is sorted by
 * ascending base addresses and overlapping segments are not allowed.
 *
 * After a segment is created, faults may occur on pages of the segment.
 * When a fault occurs, the fault handling code must get the desired
 * object and set up the hardware translation to the object.  For some
 * objects, the fault handling code also implements copy-on-write.
 *
 * When the hat wants to unload a translation, it can call the unload
 * routine which is responsible for processing reference and modify bits.
 */
struct seg {
	kmon_t	s_lock;
	addr_t	s_base;			/* base virtual address */
	u_int	s_size;			/* size in bytes */
	struct	as *s_as;		/* containing address space */
	struct	seg *s_next;		/* next seg in this address space */
	struct	seg *s_prev;		/* prev seg in this address space */
	struct	seg_ops {
		int	(*dup)(/* seg, newsegp */);
		int	(*unmap)(/* seg, addr, len */);
		int	(*free)(/* seg */);
		faultcode_t	(*fault)(/* seg, addr, len, type, rw */);
		faultcode_t	(*faulta)(/* seg, addr */);
		int	(*hatsync)(/* seg, addr, ref, mod, flags */);
		int	(*setprot)(/* seg, addr, size, prot */);
		int	(*checkprot)(/* seg, addr, size, prot */);
		int	(*kluster)(/* seg, addr, delta */);
		u_int	(*swapout)(/* seg */);
		int	(*sync)(/* seg, addr, size, flags */);
		int	(*incore)(/* seg, addr, size, vec */);
		int	(*lockop)(/* seg, addr, size, op */);
		int	(*advise)(/* seg, addr, size, behav */);
	} *s_ops;
	caddr_t	s_data;			/* private data for instance */
};

/*
 * Fault information passed to the seg fault handling routine.
 * The F_SOFTLOCK and F_SOFTUNLOCK are used by software
 * to lock and unlock pages for physical I/O.
 */
enum fault_type {
	F_INVAL,		/* invalid page */
	F_PROT,			/* protection fault */
	F_SOFTLOCK,		/* software requested locking */
	F_SOFTUNLOCK,		/* software requested unlocking */
};

/*
 * seg_rw gives the access type for a fault operation
 */
enum seg_rw {
	S_OTHER,		/* unknown or not touched */
	S_READ,			/* read access attempted */
	S_WRITE,		/* write access attempted */
	S_EXEC,			/* execution access attempted */
};

#ifdef KERNEL
/*
 * Generic segment operations
 */
struct	seg *seg_alloc(/* as, base, size */);
int	seg_attach(/* as, base, size, seg */);
void	seg_free(/* seg */);
u_int	seg_page(/* seg, addr */);
u_int	seg_pages(/* seg */);
#endif KERNEL
#endif /*!_vm_seg_h*/
