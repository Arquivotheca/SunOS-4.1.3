/*	@(#)seg_dev.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_seg_dev_h
#define	_vm_seg_dev_h

/*
 * Structure who's pointer is passed to the segvn_create routine
 */
struct segdev_crargs {
	int	(*mapfunc)();	/* map function to call */
	u_int	offset;		/* starting offset */
	dev_t	dev;		/* device number */
	u_char	prot;		/* protection */
	u_char	maxprot;	/* maximum protection */
};

/*
 * (Semi) private data maintained by the seg_dev driver per segment mapping
 */
struct	segdev_data {
	int	(*mapfunc)();	/* really returns struct pte, not int */
	u_int	offset;		/* device offset for start of mapping */
	dev_t	dev;		/* device number (for mapfunc) */
	u_char	pageprot;	/* true if per page protections present */
	u_char	prot;		/* current segment prot if pageprot == 0 */
	u_char	maxprot;	/* maximum segment protections */
	struct	vpage *vpage;	/* per-page information, if needed */
};

#ifdef KERNEL
int	segdev_create(/* seg, argsp */);
#endif KERNEL

#endif /*!_vm_seg_dev_h*/
