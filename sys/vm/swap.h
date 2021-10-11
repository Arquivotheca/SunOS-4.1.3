/*	@(#)swap.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_swap_h
#define	_vm_swap_h

/*
 * VM - virtual swap device.
 */

struct	swapinfo {
	struct	vnode *si_vp;		/* vnode for this swap device */
	u_int	si_size;		/* size (bytes) of this swap device */
	struct	anon *si_anon;		/* pointer to anon array */
	struct	anon *si_eanon;		/* pointer to end of anon array */
	struct	anon *si_free;		/* anon free list for this vp */
	int	si_allocs;		/* # of conseq. allocs from this area */
	struct	swapinfo *si_next;	/* next swap area */
	short	*si_pid;		/* parallel pid array for memory tool */
};

#define	IS_SWAPVP(vp)	(((vp)->v_flag & VISSWAP) != 0)

#ifdef KERNEL
int	swap_init(/* vp */);
struct	anon *swap_alloc();
void	swap_free(/* ap */);
void	swap_xlate(/* ap, vpp, offsetp */);
struct	anon *swap_anon(/* vp, offset */);
#endif

#endif /*!_vm_swap_h*/
