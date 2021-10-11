/*      @(#)loinfo.h 1.1 92/07/30  Copyright 1987 Sun Microsystems, Inc.     */

/*
 * Loopback mount info - one per mount 
 */

#ifndef _lofs_loinfo_h
#define _lofs_loinfo_h

struct loinfo {
	struct vfs	*li_realvfs;	/* real vfs of mount */
	struct vfs	*li_mountvfs;	/* loopback vfs */
	struct vnode	*li_rootvp;	/* root vnode of this vfs */
	int		 li_mflag;	/* mount flags to inherit */
	int		 li_refct;	/* # outstanding vnodes */
	int		li_depth;	/* depth of loopback mounts */
	struct lfsnode	*li_lfs;	/* list of other vfss */
};

/* inheritable mount flags - propagated from real vfs to loopback */
#define	INHERIT_VFS_FLAG	(VFS_RDONLY|VFS_NOSUID)

/*
 * lfsnodes are allocated as new real vfs's are encountered
 * when looking up things in a loopback name space
 * It contains a new vfs which is paired with the real vfs
 * so that vfs ops (fsstat) can get to the correct real vfs
 * given just a loopback vfs
 */
struct lfsnode {
	struct lfsnode *lfs_next;	/* next in loinfo list */
	int		lfs_refct;	/* # outstanding vnodes */
	struct vfs     *lfs_realvfs;	/* real vfs */
	struct vfs	lfs_vfs;	/* new loopback vfs */
};

#define	vtoli(VFSP)	((struct loinfo *)((VFSP)->vfs_data))

struct vfs *lo_realvfs();

extern struct vfsops lo_vfsops;

#endif /*!_lofs_loinfo_h*/
