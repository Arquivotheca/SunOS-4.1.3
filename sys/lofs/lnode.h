/*      lnode.h 1.1 92/07/30  Copyright 1987 Sun Microsystems, Inc.     */

/*
 * Loop-back file information structure.
 */

#ifndef _lofs_lnode_h
#define _lofs_lnode_h

/*
 * The lnode is the "inode" for loop-back files.  It contains
 * all the information necessary to handle loop-back file on the
 * client side.
 */
struct lnode {
	struct lnode	*lo_next;	/* link for hash chain */
	struct vnode	lo_vnode;	/* place holder vnode for file */
	struct vnode	*lo_vp;		/* pointer to real vnode */
};

/*
 * Convert between vnode and lnode
 */
#define	ltov(lp)	(&((lp)->lo_vnode))
#define	vtol(vp)	((struct lnode *)((vp)->v_data))
#define	realvp(vp)	(vtol(vp)->lo_vp)

#endif /*!_lofs_lnode_h*/
