/*	@(#)vpage.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _vm_vpage_h
#define	_vm_vpage_h

/*
 * VM - Information per virtual page.
 */
struct vpage {
	u_int	vp_prot: 4;		/* see <sys/mman.h> prot flags */
	u_int	vp_advice: 3;		/* see <sys/mman.h> madvise flags */
	u_int	vp_pplock: 1;		/* physical page locked by me */
	/*
	 * The following two are for use with a
	 * local page replacement algorithm (someday).
	 */
	u_int	vp_ref: 1;		/* reference bit */
	u_int	vp_mod: 1;		/* (maybe) modify bit, from hat */
	u_int	vp_ski_ref: 1;		/* ski reference bit */
	u_int	vp_ski_mod: 1;		/* ski modified bit */
	u_int   : 4;
};

#endif /*!_vm_vpage_h*/
