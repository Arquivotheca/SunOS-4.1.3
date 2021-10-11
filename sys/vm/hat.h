/*	@(#)hat.h  1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_hat_h
#define	_vm_hat_h

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the machine independent interfaces to
 * the hardware address translation management routines.  Other
 * machine specific interfaces and structures are defined
 * in <machine/vm_hat.h>.  The hat layer manages the address
 * translation hardware as a cache driven by calls from the
 * higher levels of the VM system.
 */

#include <machine/vm_hat.h>

#ifdef KERNEL
/*
 * One time hat initialization
 */
void	hat_init();

/*
 * Operations on hat resources for an address space:
 *	- initialize any needed hat structures for the address space
 *	- free all hat resources now owned by this address space
 *
 * N.B. - The hat structure is guaranteed to be zeroed when created.
 * The hat layer can choose to define hat_alloc as a macro to avoid
 * a subroutine call if this is sufficient initialization.
 */
#ifndef hat_alloc
void	hat_alloc(/* as */);
#endif
void	hat_free(/* as */);

/*
 * Operations on a named address with in a segment:
 *	- load/lock the given page struct
 *	- load/lock the given page frame number
 *	- unlock the given address
 *
 * (Perhaps we need an interface to load several pages at once?)
 */
void	hat_memload(/* seg, addr, pp, prot, lock */);
void	hat_devload(/* seg, addr, pf, prot, lock */);
void	hat_unlock(/* seg, addr */);

/*
 * Operations over an address range:
 *	- change protections
 *	- change mapping to refer to a new segment
 *	- unload mapping
 */
void	hat_chgprot(/* seg, addr, len, prot */);
void	hat_newseg(/* seg, addr, len, nseg */);
void	hat_unload(/* seg, addr, len */);

/*
 * Operations that work on all active translation for a given page:
 *	- unload all translations to page
 *	- get hw stats from hardware into page struct and reset hw stats
 */
void	hat_pageunload(/* pp */);
void	hat_pagesync(/* pp */);

/*
 * Operations that return physical page numbers (ie - used by mapin):
 *	- return the pfn for kernel virtual address
 *	- return the pfn for arbitrary virtual address
 */
u_int	hat_getkpfnum(/* addr */);
/*
 * XXX - This one is not yet implemented - not yet needed
 * u_int hat_getpfnum(as, addr);
 */

#endif KERNEL

#endif /*!_vm_hat_h*/
