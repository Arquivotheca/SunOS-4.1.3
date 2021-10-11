/*	@(#)lo_vfsops.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/uio.h>
#include <lofs/loinfo.h>
#include <lofs/lnode.h>
#undef NFS
#include <sys/mount.h>

struct vnode *makelonode();
extern struct vnodeops lo_vnodeops;
#ifdef	LODEBUG
extern int lodebug;
#endif	LODEBUG

/*
 * lo vfs operations.
 */
static int lo_mount();
static int lo_unmount();
static int lo_root();
static int lo_statfs();
static int lo_sync();
extern int lo_badop();

struct vfsops lo_vfsops = {
	lo_mount,
	lo_unmount,
	lo_root,
	lo_statfs,
	lo_sync,
	lo_badop,	/* vget */
	lo_badop,	/* mountroot */
	lo_badop,	/* swapvp */
};

/*
 * lo mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
static int
lo_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	struct vnode *srootvp = NULL;	/* the server's root */
	struct vnode *realrootvp;
	struct lo_args args;		/* lo mount arguments */
	struct loinfo *li;

	/*
	 * get arguments
	 */
	error = copyin(data, (caddr_t)&args, (u_int)(sizeof args));
	if (error) {
		return (error);
	}

	/*
	 * Find real root, and make vfs point to real vfs
	 */
	error = lookupname(args.fsdir, UIOSEG_USER, FOLLOW_LINK,
	    (struct vnode **)0, &realrootvp);
	if (!error) {
		/*
		 * allocate a vfs info struct and attach it
		 */
		li = (struct loinfo *)
			new_kmem_zalloc(sizeof (*li), KMEM_SLEEP);
		li->li_realvfs = realrootvp->v_vfsp;
		li->li_mountvfs = vfsp;
		li->li_mflag = vfsp->vfs_flag & INHERIT_VFS_FLAG;
		vfsp->vfs_flag |= (li->li_realvfs->vfs_flag & INHERIT_VFS_FLAG);
		li->li_refct = 0;
		vtoli(vfsp) = li;
		vfsp->vfs_bsize = li->li_realvfs->vfs_bsize;
		if (realrootvp->v_op == &lo_vnodeops) {
			li->li_depth = 1 + vtoli(realrootvp->v_vfsp)->li_depth;
		} else {
			li->li_depth = 0;
		}

		/*
		 * Make the root vnode
		 */
		srootvp = makelonode(realrootvp, li);
		if (srootvp->v_flag & VROOT) {
			VN_RELE(srootvp);
			kmem_free((caddr_t)li, sizeof (*li));
			return (EBUSY);
		}
		srootvp->v_flag |= VROOT;
		li->li_rootvp = srootvp;

#ifdef LODEBUG
		lo_dprint(lodebug, 4,
		    "lo_mount: vfs %x realvfs %x root %x realroot %x li %x\n",
		    vfsp, li->li_realvfs, srootvp, realrootvp, li);
#endif
	}
	return (error);
}

/*
 * Undo loopback mount
 */
static int
lo_unmount(vfsp)
	struct vfs *vfsp;
{
	struct loinfo *li;

	li = vtoli(vfsp);
#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_unmount(%x) li %x\n", vfsp, li);
#endif
	VFS_SYNC(vfsp);
	if (li->li_refct != 1 || li->li_rootvp->v_count != 1) {
#ifdef LODEBUG
		lo_dprint(lodebug, 4, "refct %d v_ct %d\n", li->li_refct,
		    li->li_rootvp->v_count);
#endif
		return (EBUSY);
	}
	VN_RELE(li->li_rootvp);
	kmem_free((caddr_t)li, (u_int)sizeof (*li));
	return (0);
}

/*
 * find root of lo
 */
static int
lo_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{

	*vpp = (struct vnode *)vtoli(vfsp)->li_rootvp;
#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_root(0x%x) = %x\n", vfsp, *vpp);
#endif
	(*vpp)->v_count++;
	return (0);
}

/*
 * Get file system statistics.
 */
static int
lo_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lostatfs %x\n", vfsp);
#endif
	return (VFS_STATFS(lo_realvfs(vfsp), sbp));
}

/*
 * Flush any pending I/O.
 */
static int
lo_sync(vfsp)
	struct vfs *vfsp;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_sync: %x\n", vfsp);
#endif
	if (vfsp != (struct vfs *)NULL)
		return (VFS_SYNC(lo_realvfs(vfsp)));
	return (0);
}
