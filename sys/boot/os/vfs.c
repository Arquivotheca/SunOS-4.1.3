
/* @(#)vfs.c 1.1 92/07/30 SMI */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/vfs.h>
#undef	KERNEL
#include <sys/socket.h>
#include "boot/systm.h"
#define	KERNEL
#undef NFS
#include <sys/mount.h>
#undef KERNEL
#include <sys/pathname.h>
#define	KERNEL
#include "boot/vnode.h"
#ifdef UFS
#include "boot/inode.h"
#endif
#include <sys/bootconf.h>

#include <stand/saio.h>

#undef u
struct	user	u;
static struct proc _p;

/*
 * vfs global data
 */
struct vnode *rootdir;		  /* pointer to root vnode */

struct vfs *rootvfs;		    /* pointer to root vfs. This is */
					/* also the head of the vfs list */
/*
 * Convert mode formats to vnode types
 */
enum vtype mftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};

static	int	dump_debug = 10;
extern	struct	vfssw	*getfstype();


/*
 * vfs_mountroot is called by main (boot.c) to
 * mount the root filesystem.
 */
void
vfs_mountroot()
{
	register int error;
	struct vfssw *vsw;

	u.u_error = 0;

	if (rootvfs != (struct vfs *)0) {
		printf("root on %s fstype %s\n",
		    rootfs.bo_name, rootfs.bo_fstype);
		return;
	}
	/*
	 * Setup credentials
	 */
	u.u_procp = &_p;
	u.u_cred = crget();
	u.u_uid = 0;		/* root user */
	u.u_gid = 1;		/* root group */
	u.u_groups[0] = 1;	/* root group */
	u.u_groups[1] = -1;	/* end of groups */
	u.u_groups[2] = -1;	/* end of groups */
	u.u_groups[3] = -1;	/* end of groups */
	u.u_groups[4] = -1;	/* end of groups */
	u.u_groups[5] = -1;	/* end of groups */
	u.u_groups[6] = -1;	/* end of groups */
	u.u_groups[7] = -1;	/* end of groups */

	u.u_ruid = 0;		/* root user */
	u.u_rgid = 1;		/* root group */
	u.u_groups[1] = -1;	/* end of groups */

	rootvfs = (struct vfs *)kmem_alloc(sizeof (struct vfs));
	vsw = getfstype("root", rootfs.bo_fstype);
	if (vsw) {
		VFS_INIT(rootvfs, vsw->vsw_ops, (caddr_t)0);
		error = VFS_MOUNTROOT(rootvfs, &rootvp, rootfs.bo_name);
	} else {

		/*
		 * Step through the filesystem types til we find one that
		 * will mount a root filesystem.
		 * If error panic.
		 */
		for (vsw = vfssw; vsw <= &vfssw[MOUNT_MAXTYPE]; vsw++){
			if (vsw->vsw_ops) {
				VFS_INIT(rootvfs, vsw->vsw_ops, (caddr_t)0);
				error = VFS_MOUNTROOT(rootvfs, &rootvp,
				    rootfs.bo_name);
				if (!error) {
					break;
				}
			}
		}
	}

	if (error) {
		printf("Boot: unable to mount root (error 0x%x)\n", error);
		kmem_free(rootvfs, sizeof (struct vfs));
		rootvfs = (struct vfs *)0;
		u.u_error = error;
		return;
	} else {
#ifdef	DUMP_DEBUG
		dprint(dump_debug, 6,
			"vfs_mountroot: mounted root rootvfs 0x%x\n",
			rootvfs);
#endif /* DUMP_DEBUG */
	}

	/*
	 * Get vnode for '/'.
	 * Setup rootdir, u.u_rdir and u.u_cdir to point to it.
	 * These are used by lookuppn so that it knows where
	 * to start from '/' or '.'.
	 */
	error = VFS_ROOT(rootvfs, &rootdir);
	if (error)	{
		printf("Boot: cannot get root vnode (error 0x%x)\n", error);
		u.u_error = error;
		return;
	}
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	/*
	 * I needed the root vnode here.
	 */
	u.u_rdir = rootdir;
	rootfs.bo_vp = rootvp;
	(void) strcpy(rootfs.bo_fstype, vsw->vsw_name);
	rootfs.bo_size = 0;
	rootfs.bo_flags = BO_VALID;

	printf("root on %s fstype %s\n", rootfs.bo_name, rootfs.bo_fstype);

}

/*
 * vfs_add is called by a specific filesystem's mount routine to add
 * the new vfs into the vfs list and to cover the mounted on vnode.
 * The vfs is also locked so that lookuppn will not venture into the
 * covered vnodes subtree.
 * coveredvp is zero if this is the root.
 */
int
vfs_add(coveredvp, vfsp, mflag)
	register struct vnode *coveredvp;
	register struct vfs *vfsp;
	int mflag;
{
	register int error;

	error = vfs_lock(vfsp);
	if (error)
		return (error);
	if (coveredvp != (struct vnode *)0) {
		/*
		 * Return EBUSY if the covered vp is already mounted on.
		 */
		if (coveredvp->v_vfsmountedhere != (struct vfs *)0) {
			printf("Boot: root vnode busy\n");
			vfs_unlock(vfsp);
			return (EBUSY);
		}
		/*
		 * Put the new vfs on the vfs list after root.
		 * Point the covered vnode at the new vfs so lookuppn
		 * (vfs_lookup.c) can work its way into the new file system.
		 */
		vfsp->vfs_next = rootvfs->vfs_next;
		rootvfs->vfs_next = vfsp;
		coveredvp->v_vfsmountedhere = vfsp;
	} else {
		dprint(dump_debug, 6, "vfs_add: real root\n");
		/*
		 * This is the root of the whole world.
		 */
		rootvfs = vfsp;
		vfsp->vfs_next = (struct vfs *)0;
	}
	vfsp->vfs_vnodecovered = coveredvp;
	if (mflag & M_RDONLY) {
		vfsp->vfs_flag |= VFS_RDONLY;
	} else {
		vfsp->vfs_flag &= ~VFS_RDONLY;
	}
	if (mflag & M_NOSUID) {
		vfsp->vfs_flag |= VFS_NOSUID;
	} else {
		vfsp->vfs_flag &= ~VFS_NOSUID;
	}
	return (0);
}

/*
 * Lock a filesystem to prevent access to it while mounting and unmounting.
 * Returns error if already locked.
 * XXX This totally inadequate for unmount right now - srk
 */
int
vfs_lock(vfsp)
	register struct vfs *vfsp;
{

	if (vfsp->vfs_flag & VFS_MLOCK)
		return (EBUSY);
	vfsp->vfs_flag |= VFS_MLOCK;
	return (0);
}

/*
 * Unlock a locked filesystem.
 * Panics if not locked
 */
void
vfs_unlock(vfsp)
	register struct vfs *vfsp;
{

	if ((vfsp->vfs_flag & VFS_MLOCK) == 0)
		panic("vfs_unlock");
	vfsp->vfs_flag &= ~VFS_MLOCK;
	/*
	 * Wake anybody waiting for the lock to clear
	 */
	if (vfsp->vfs_flag & VFS_MWAIT) {
		vfsp->vfs_flag &= ~VFS_MWAIT;
		wakeup((caddr_t)vfsp);
	}
}
