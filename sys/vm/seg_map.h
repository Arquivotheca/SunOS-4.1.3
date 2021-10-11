/*	@(#)seg_map.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_seg_map_h
#define	_vm_seg_map_h

struct segmap_crargs {
	u_int	prot;
};

/*
 * Each smap struct represents a MAXBSIZE sized mapping to the
 * <sm_vp, sm_off> given in the structure.  The location of the
 * the structure in the array gives the virtual address of the
 * mapping.
 */
struct	smap {
	struct	vnode *sm_vp;		/* vnode pointer (if mapped) */
	u_int	sm_off;			/* file offset for mapping */
	/*
	 * These next 4 entries can be coded as
	 * u_shorts if we are tight on memory.
	 */
	u_int	sm_refcnt;		/* reference count for uses */
	struct	smap *sm_hash;		/* hash pointer */
	struct	smap *sm_next;		/* next pointer */
	struct	smap *sm_prev;		/* previous pointer */
};

/*
 * (Semi) private data maintained by the segmap driver per SEGMENT mapping
 */
struct	segmap_data {
	struct	smap *smd_sm;		/* array of smap structures */
	struct	smap *smd_free;		/* free list head pointer */
	u_char	smd_prot;		/* protections for all smap's */
	u_char	smd_want;		/* smap want flag */
	u_int	smd_hashsz;		/* power-of-two hash table size */
	struct	smap **smd_hash;	/* pointer to hash table */
};

/*
 * These are flags used on release.  Some of these might get handled
 * by segment operations needed for msync (when we figure them out).
 * SM_ASYNC modifies SM_WRITE.  SM_DONTNEED modifies SM_FREE.  SM_FREE
 * and SM_INVAL are mutually exclusive.
 */
#define	SM_WRITE	0x01		/* write back the pages upon release */
#define	SM_ASYNC	0x02		/* do the write asynchronously */
#define	SM_FREE		0x04		/* put pages back on free list */
#define	SM_INVAL	0x08		/* invalidate page (no caching) */
#define	SM_DONTNEED	0x10		/* less likely to be needed soon */

#define	MAXBSHIFT	13		/* log2(DEVBSIZE) */
#define	MAXBOFFSET	(MAXBSIZE - 1)
#define	MAXBMASK	(~MAXBOFFSET)

/*
 * SMAP_HASHAVELEN is the average length desired for this chain, from
 * which the size of the smd_hash table is derived at segment create time.
 * SMAP_HASHVPSHIFT is defined so that 1 << SMAP_HASHVPSHIFT is the
 * approximate size of a vnode struct.
 */
#define	SMAP_HASHAVELEN		4
#define	SMAP_HASHVPSHIFT	6

#define	SMAP_HASHFUNC(smd, vp, off) \
	((((off) >> MAXBSHIFT) + ((int)(vp) >> SMAP_HASHVPSHIFT)) & \
		((smd)->smd_hashsz - 1))

#ifdef KERNEL
int	segmap_create(/* seg, argsp */);

/*
 * Special seg_map segment operations
 */
void	segmap_pagecreate(/* seg, addr, len, softlock */);
addr_t	segmap_getmap(/* seg, vp, off */);
int	segmap_release(/* seg, addr, flags */);

extern struct seg *segkmap;	/* the kernel generic mapping segment */
extern struct seg_ops segmap_ops;
#endif KERNEL

#endif /*!_vm_seg_map_h*/
