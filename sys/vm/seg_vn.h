/*	@(#)seg_vn.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_seg_vn_h
#define	_vm_seg_vn_h

#include <vm/mp.h>

/*
 * Structure who's pointer is passed to the segvn_create routine
 */
struct segvn_crargs {
	struct	vnode *vp;	/* vnode mapped from */
	u_int	offset;		/* starting offset of vnode for mapping */
	struct	ucred *cred;	/* creditials */
	u_char	type;		/* type of sharing done */
	u_char	prot;		/* protections */
	u_char	maxprot;	/* maximum protections */
	struct	anon_map *amp;	/* anon mapping to map to */
};

/*
 * The anon_map structure is used by the seg_vn driver to manage
 * unnamed (anonymous) memory.   When anonymous memory is shared,
 * then the different segvn_data structures will point to the
 * same anon_map structure.  Also, if a segment is unmapped
 * in the middle where an anon_map structure exists, the
 * newly created segment will also share the anon_map structure,
 * although the two segments will use different ranges of the
 * anon array.  When mappings are private (or shared with
 * a reference count of 1), an unmap operation will free up
 * a range of anon slots in the array given by the anon_map
 * structure.  Because of fragmentation due to this unmapping,
 * we have to store the size of anon array in the anon_map
 * structure so that we can free everything when the referernce
 * count goes to zero.
 */
struct anon_map {
	u_int	refcnt;		/* reference count on this structure */
	u_int	size;		/* size in bytes mapped by the anon array */
	struct	anon **anon;	/* pointer to an array of anon * pointers */
	u_int	swresv;		/* swap space reserved for this anon_map */
	u_int	flags;		/* anon_map flags (see below) */
};

/* anon_map flags */
#define	AMAP_LOCKED	0x01	/* anon_map is locked */
#define	AMAP_WANT	0x02	/* some process waiting on lock */

/*
 * Lock and unlock anon_map if the segment has private pages.  This
 * is necessary to ensure that operations on the anon array (e.g., growing
 * the array, or allocating an anon slot and assigning a page) are atomic.
 */
#define	AMAP_LOCK(amp) { \
	while ((amp)->flags & AMAP_LOCKED) { \
		(amp)->flags |= AMAP_WANT; \
		(void) sleep((caddr_t)(amp), PAMAP); \
	} \
	(amp)->flags |= AMAP_LOCKED; \
	masterprocp->p_swlocks++; \
}

#define	AMAP_UNLOCK(amp) { \
	(amp)->flags &= ~AMAP_LOCKED; \
	masterprocp->p_swlocks--; \
	if ((amp)->flags & AMAP_WANT) { \
		(amp)->flags &= ~AMAP_WANT; \
		wakeup((caddr_t)(amp)); \
	} \
}

/*
 * (Semi) private data maintained by the seg_vn driver per segment mapping
 */
struct	segvn_data {
	kmon_t	lock;
	u_char	pageprot;	/* true if per page protections present */
	u_char	prot;		/* current segment prot if pageprot == 0 */
	u_char	maxprot;	/* maximum segment protections */
	u_char	type;		/* type of sharing done */
	struct	vnode *vp;	/* vnode that segment mapping is to */
	u_int	offset;		/* starting offset of vnode for mapping */
	u_int	anon_index;	/* starting index into anon_map anon array */
	struct	anon_map *amp;	/* pointer to anon share structure, if needed */
	struct	vpage *vpage;	/* per-page information, if needed */
	struct	ucred *cred;	/* mapping creditials */
	u_int	swresv;		/* swap space reserved for this segment */
	u_char	advice;		/* madvise flags for segment */
	u_char	pageadvice;	/* true if per page advice set */
};

#ifdef KERNEL
int	segvn_create(/* seg, argsp */);

extern	struct seg_ops segvn_ops;

/*
 * Provided as short hand for creating user zfod segments
 */
extern	caddr_t zfod_argsp;
extern	caddr_t kzfod_argsp;
#endif KERNEL

#endif /*!_vm_seg_vn_h*/
