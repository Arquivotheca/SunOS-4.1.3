#ident "@(#)vfs.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/time.h>
#include <sys/kernel.h>
#undef	NFSSERVER
#include <sys/mount.h>
#include <sys/pathname.h>
#include <sys/unistd.h>
#include <sys/vnode.h>
#ifdef	UFS
#include <ufs/inode.h>
#endif
#include <sys/bootconf.h>

#include <vm/page.h>
#include <vm/swap.h>

/*
 * vfs global data
 */
struct	vnode *rootdir;			/* pointer to root vnode */

struct	vfs *rootvfs;			/* pointer to root vfs. This is */
					/* also the head of the vfs list */
/*
 * Convert mode formats to vnode types
 */
enum	vtype mftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};

/*
 * System calls
 */

/*
 * Translate mount ype numbers from old order to new order for backward
 * compatability with old mount command
 */
static int fixtype[] =
	{ MOUNT_UFS, MOUNT_NFS, MOUNT_PC, MOUNT_UFS};

/*
 * mount system call
 */
mount(uap)
	register struct a {
		char	*type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
{
	struct pathname pn;
	struct vnode *vp;
	struct vfs *vfsp;
	struct vfssw *vs;
	extern struct vnodeops ufs_vnodeops;
	int saved_flag;

	/*
	 * Must be super user
	 */
	if (!suser())
		return;
	/*
	 * Get vnode to be covered
	 */
	u.u_error = lookupname(uap->dir, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;

	if (vp->v_vfsp->vfs_flag & VFS_NOSUB) {
		VN_RELE(vp);
		u.u_error = EREMOTE;
		return;
	}
	dnlc_purge();
	if ((uap->flags & M_SYS5) && !(uap->flags & M_REMOUNT)) {
		/*
		 * This test is sort of bogus as you can cover up a
		 * directory by mounting at the next higher level spot
		 * in the directory tree.  If we want to keep this
		 * somewhat bogus test, then we will need to add
		 * a call to a routine which gets rid of any current
		 * pages / segmap mappings for this vnode.  This
		 * routine will need to exist for unmount anyway.
		 */
		if (vp->v_count != 1) {
			VN_RELE(vp);
			u.u_error = EBUSY;
			return;
		}
	}
	if (vp->v_type != VDIR) {
		VN_RELE(vp);
		u.u_error = ENOTDIR;
		return;
	}
	if ((uap->flags & M_NEWTYPE) || (uap->flags & M_REMOUNT)) {
		u.u_error = pn_get(uap->type, UIO_USERSPACE, &pn);
		if (u.u_error) {
			VN_RELE(vp);
			return;
		}
		for (vs = vfssw; vs < vfsNVFS; vs++) {
			if (vs->vsw_name != 0 &&
			    strcmp(pn.pn_path, vs->vsw_name) == 0) {
				break;
			}
		}
		if (vs == vfsNVFS) {
			u.u_error = ENODEV;
			VN_RELE(vp);
			pn_free(&pn);
			return;
		}
		pn_free(&pn);
	} else if ((int)uap->type >= sizeof (fixtype)/sizeof (int) ||
	    (vs = &vfssw[fixtype[(int)uap->type]])->vsw_ops == 0) {
		u.u_error = ENODEV;
		VN_RELE(vp);
		return;
	}
	/*
	 * If this is a remount, we don't want to create a new VFS.
	 * Instead, we pass the existing one with a remount flag.
	 */
	if (uap->flags & M_REMOUNT) {
		for (vfsp = rootvfs;
		    vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next) {
			if ((vp->v_vfsp == vfsp) && (vp->v_flag & VROOT) &&
			    (vp->v_vfsmountedhere == (struct vfs *)0))
				break;
		}
		if (vfsp == (struct vfs *)0) {
			printf ("mount: no entry\n");
			u.u_error = ENOENT;
			VN_RELE(vp);
			return;
		}
		u.u_error = vfs_lock(vfsp);
		if (u.u_error) {
			VN_RELE(vp);
			return;
		}
		u.u_error = pn_get(uap->dir, UIO_USERSPACE, &pn);
		if (u.u_error) {
			VN_RELE(vp);
			vfs_unlock(vfsp);
			return;
		}
		/*
		 * Disallow making file systems read-only.
		 * Ignore other flags.
		 */
		if (uap->flags & M_RDONLY) {
			printf ("mount: can't remount ro\n");
			u.u_error = EINVAL;
			vfs_unlock(vfsp);
			VN_RELE(vp);
			pn_free(&pn);
			return;
		}
		saved_flag = vfsp->vfs_flag;
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
#ifdef	UFS
		if (vp->v_op == &ufs_vnodeops)
			ILOCK(VTOI(vp));
#endif
	} else {
		u.u_error = pn_get(uap->dir, UIO_USERSPACE, &pn);
		if (u.u_error) {
			VN_RELE(vp);
			return;
		}
		vfsp = (struct vfs *)new_kmem_alloc(
				sizeof (struct vfs), KMEM_SLEEP);
		VFS_INIT(vfsp, vs->vsw_ops, (caddr_t)0);

		/*
		 * Mount the filesystem.
		 * Lock covered vnode (XXX this currently only works if
		 * it is type ufs)
		 */
#ifdef	UFS
		if (vp->v_op == &ufs_vnodeops)
			ILOCK(VTOI(vp));
#endif
		u.u_error = vfs_add(vp, vfsp, uap->flags);
		if (u.u_error) {
#ifdef	UFS
			if (vp->v_op == &ufs_vnodeops)
				IUNLOCK(VTOI(vp));
#endif
			pn_free(&pn);
			kmem_free((caddr_t)vfsp, sizeof (struct vfs));
			VN_RELE(vp);
			return;
		}
	}
	u.u_error = VFS_MOUNT(vfsp, pn.pn_path, uap->data);
#ifdef	UFS
	if (vp->v_op == &ufs_vnodeops)
		IUNLOCK(VTOI(vp));
#endif
	pn_free(&pn);
	if (!u.u_error) {
		vfs_unlock(vfsp);
		if (uap->flags & M_REMOUNT) {
			vfsp->vfs_flag &= ~VFS_REMOUNT;
			VN_RELE(vp);
		}
#ifdef	VFSSTATS
		else {
			struct vfsstats *vfsstats;

			vfsstats = (struct vfsstats *)new_kmem_zalloc(
					sizeof (struct vfsstats), KMEM_SLEEP);
			vfsstats->vs_time = time.tv_sec;
			vfsp->vfs_stats = (caddr_t)vfsstats;
		}
#endif
	} else {
		/*
		 * There was an error, return filesystem to previous
		 * state if remount, otherwise clean up vfsp
		 */
		if (uap->flags & M_REMOUNT) {
			vfsp->vfs_flag = saved_flag;
			vfs_unlock(vfsp);
		} else {
			vfs_remove(vfsp);
			kmem_free((caddr_t)vfsp, sizeof (struct vfs));
		}
		VN_RELE(vp);
	}
}

/*
 * Sync system call - sync all vfs types
 */
sync()
{
	register struct vfssw *vfsswp;

	for (vfsswp = vfssw; vfsswp < vfsNVFS; vfsswp++) {
		if (vfsswp->vsw_ops == (struct vfsops *)NULL)
			continue;		/* not configured in */
		/*
		 * Call the vfs sync operation with a NULL
		 * pointer to force all vfs's of the given
		 * type to be flushed back to main storage.
		 */
		(*vfsswp->vsw_ops->vfs_sync)((struct vfs *)NULL);
	}
}

/*
 * get pathconf info
 */
pathconf(uap)
	struct a {
		char *path;
		int what;
	} *uap;
{
	struct vnode *vp;
	int ans = -1;

	u.u_error = lookupname(uap->path, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;
	u.u_error = VOP_CNTL(vp, uap->what, 0, &ans, 0, CNTL_INT32);
	u.u_rval1 = ans;
	VN_RELE(vp);
}

fpathconf(uap)
	struct a {
		int fd;
		int what;
	} *uap;
{
	struct file *fp;
	int ans = -1;

	/*
	 * Forgive me, oh lord, for surely I sin.
	 * Pipes (unnamed) have no vnode_ops,
	 * so I have to catch this here.
	 */
	if (uap->what == _PC_PIPE_BUF) {
		if ((fp = getf(uap->fd)) == NULL)
			return;
		if (fp->f_type == DTYPE_SOCKET &&
		    ((struct socket *)fp->f_data)->so_state & SS_PIPE) {
			u.u_rval1 = PIPE_BUF;
			return;
		}
	}
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0) {
		struct vnode *vp = (struct vnode*)fp->f_data;
		u.u_error = VOP_CNTL(vp, uap->what, 0, &ans, 0, CNTL_INT32);
	}
	u.u_rval1 = ans;
}

/*
 * get filesystem statistics
 */
statfs(uap)
	struct a {
		char *path;
		struct statfs *buf;
	} *uap;
{
	struct vnode *vp;

	u.u_error = lookupname(uap->path, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;
	cstatfs(vp->v_vfsp, uap->buf);
	VN_RELE(vp);
}

fstatfs(uap)
	struct a {
		int fd;
		struct statfs *buf;
	} *uap;
{
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		cstatfs(((struct vnode *)fp->f_data)->v_vfsp, uap->buf);
}

cstatfs(vfsp, ubuf)
	struct vfs *vfsp;
	struct statfs *ubuf;
{
	struct statfs sb;

	bzero((caddr_t)&sb, sizeof (sb));
	u.u_error = VFS_STATFS(vfsp, &sb);
	if (u.u_error)
		return;
	/*
	 * XXX FIX IN 5.0
	 *
	 * Ugly hack to cater to programs that multiply # of blocks
	 * by block size and thus overflow when the FS size exceeds 2GB.
	 * Copy the true size information to the spares, then put a
	 * ceiling on the returned sizes.
	 */
#define	INT_MAX	0x7fffffff
	sb.f_spare[0] = sb.f_blocks;
	sb.f_spare[1] = sb.f_bfree;
	sb.f_spare[2] = sb.f_bavail;
	if (sb.f_bsize != 0) {
		sb.f_blocks = MIN(sb.f_blocks, INT_MAX / sb.f_bsize);
		sb.f_bfree = MIN(sb.f_bfree, INT_MAX / sb.f_bsize);
		sb.f_bavail = MIN(sb.f_bavail, INT_MAX / sb.f_bsize);
	}
	u.u_error = copyout((caddr_t)&sb, (caddr_t)ubuf, sizeof (sb));
}

/*
 * Unmount system call.
 *
 * Note: unmount takes a path to the vnode mounted on as argument,
 * not special file (as before).
 */
unmount(uap)
	struct a {
		char	*pathp;
	} *uap;
{
	struct vnode *fsrootvp;
	register struct vfs *vfsp;

	if (!suser())
		return;
	/*
	 * lookup root of fs
	 */
	u.u_error = lookupname(uap->pathp, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &fsrootvp);
	if (u.u_error)
		return;
	/*
	 * make sure this is a root
	 */
	if ((fsrootvp->v_flag & VROOT) == 0) {
		u.u_error = EINVAL;
		VN_RELE(fsrootvp);
		return;
	}
	/*
	 * get vfs
	 */
	vfsp = fsrootvp->v_vfsp;
	VN_RELE(fsrootvp);
	/*
	 * Do the unmount.
	 */
	dounmount(vfsp);
}

/*
 * XXX - subroutine so the old 4.2/S5-style
 * "umount" call can use this code as well.
 */
dounmount(vfsp)
	register struct vfs *vfsp;
{
	register struct vnode *coveredvp;

	/*
	 * Get covered vnode.
	 */
	coveredvp = vfsp->vfs_vnodecovered;
	/*
	 * lock vnode to maintain fs status quo during unmount
	 */
	u.u_error = vfs_lock(vfsp);
	if (u.u_error)
		return;

	/*
	 * XXX - need to have to have something to remove all cached
	 * pages and segmap mappings to vnodes in this vfs.
	 */
	dnlc_purge();	/* remove dnlc entries for this file sys */
	VFS_SYNC(vfsp);

	u.u_error = VFS_UNMOUNT(vfsp);
	if (u.u_error) {
		vfs_unlock(vfsp);
	} else {
		vfs_remove(vfsp);
		VN_RELE(coveredvp);
#ifdef	VFSSTATS
		if (vfsp->vfs_stats) {
			kmem_free((caddr_t)vfsp->vfs_stats,
			    sizeof (struct vfsstats));
		}
#endif
		kmem_free((caddr_t)vfsp, sizeof (*vfsp));
	}
}

/*
 * External routines
 */

/*
 * vfs_mountroot is called by main (init_main.c) to
 * mount the root filesystem.
 */
void
vfs_mountroot()
{
	register int error;
	struct vfssw *vsw;
	extern char *strcpy();

	rootvfs = (struct vfs *)new_kmem_alloc(sizeof (struct vfs), KMEM_SLEEP);
retry:
	vsw = getfstype("root", rootfs.bo_fstype);
	if (vsw) {
		VFS_INIT(rootvfs, vsw->vsw_ops, (caddr_t)0);
		error = VFS_MOUNTROOT(rootvfs, &rootvp, rootfs.bo_name);
		if (error) {
			printf("Couldn't mount root from %s, error %d\n",
				vsw->vsw_name, error);
			goto retry;
		}
	} else {
		/*
		 * Step through the filesystem types til we find one that
		 * will mount a root filesystem.  If error panic.
		 */
		for (vsw = vfssw; vsw < vfsNVFS; vsw++) {
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
		panic("vfs_mountroot: cannot mount root");
	}
#ifdef	VFSSTATS
	{
		struct vfsstats *vfsstats;

		vfsstats = (struct vfsstats *)
		    new_kmem_zalloc(sizeof (struct vfsstats), KMEM_SLEEP);
		vfsstats->vs_time = time.tv_sec;
		rootvfs->vfs_stats = (caddr_t)vfsstats;
	}
#endif
	/*
	 * Get vnode for '/'.
	 * Setup rootdir, u.u_rdir and u.u_cdir to point to it.
	 * These are used by lookuppn so that it knows where
	 * to start from '/' or '.'.
	 */
	error = VFS_ROOT(rootvfs, &rootdir);
	if (error)
		panic("vfs_mountroot: no root vnode");
	u.u_cdir = rootdir;
	VN_HOLD(u.u_cdir);
	u.u_rdir = NULL;
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
 * covered vnodes subtree.  coveredvp is zero if this is the root.
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
	if (mflag & M_GRPID) {
		vfsp->vfs_flag |= VFS_GRPID;
	} else {
		vfsp->vfs_flag &= ~VFS_GRPID;
	}
	if (mflag & M_NOSUB) {
		vfsp->vfs_flag |= VFS_NOSUB;
	} else {
		vfsp->vfs_flag &= ~VFS_NOSUB;
	}
	if (mflag & M_MULTI) {
		vfsp->vfs_flag |= VFS_MULTI;
	} else {
		vfsp->vfs_flag &= ~VFS_MULTI;
	}
	return (0);
}

/*
 * Remove a vfs from the vfs list, and destory pointers to it.
 * Should be called by filesystem implementation after it determines
 * that an unmount is legal but before it destroys the vfs.
 */
void
vfs_remove(vfsp)
register struct vfs *vfsp;
{
	register struct vfs *tvfsp;
	register struct vnode *vp;

	/*
	 * can't unmount root. Should never happen, because fs will be busy.
	 */
	if (vfsp == rootvfs)
		panic("vfs_remove: unmounting root");
	for (tvfsp = rootvfs;
	    tvfsp != (struct vfs *)0; tvfsp = tvfsp->vfs_next) {
		if (tvfsp->vfs_next == vfsp) {
			/*
			 * remove vfs from list, unmount covered vp.
			 */
			tvfsp->vfs_next = vfsp->vfs_next;
			vp = vfsp->vfs_vnodecovered;
			vp->v_vfsmountedhere = (struct vfs *)0;
			/*
			 * release lock and wakeup anybody waiting
			 */
			vfs_unlock(vfsp);
			return;
		}
	}
	/*
	 * can't find vfs to remove
	 */
	panic("vfs_remove: vfs not found");
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

struct	vfs *
getvfs(fsid)
	fsid_t *fsid;
{
	register struct vfs *vfsp;

	for (vfsp = rootvfs; vfsp; vfsp = vfsp->vfs_next) {
		if (vfsp->vfs_fsid.val[0] == fsid->val[0] &&
		    vfsp->vfs_fsid.val[1] == fsid->val[1]) {
			break;
		}
	}
	return (vfsp);
}

/*
 * Take a file system ID of the sort that appears in a "struct vattr"
 * and find the VFS that has that file system ID.  This is done by
 * finding the root directory for each VFS, finding its attributes,
 * and comparing its file system ID with the given file system ID;
 * if we have a match, we've found the right VFS.  It's slow, but it
 * works, and it's only used for the System V "ustat" call which is
 * a crock anyway ("statfs" is better).
 */
int
vafsidtovfs(vafsid, vfspp)
	long vafsid;
	struct vfs **vfspp;
{
	register struct vfs *vfsp;
	struct vnode *rootvn;		/* pointer to root vnode of vfs */
	struct vattr vattr;
	register int error;

	for (vfsp = rootvfs; vfsp != (struct vfs *)0; vfsp = vfsp->vfs_next) {
		error = VFS_ROOT(vfsp, &rootvn);
		if (error)
			if (error == ESTALE)
				continue;
			else
				return (error);
		error = VOP_GETATTR(rootvn, &vattr, u.u_cred);
		VN_RELE(rootvn);
		if (error)
			if (error == ESTALE)
				continue;
			else
				return (error);
		if (vafsid == vattr.va_fsid) {
			*vfspp = vfsp;
			return (0);
		}
	}
	return (EINVAL);			/* not found */
}

/*
 * The policies and routines below guarantee that a unique device number
 * (as returned by the stat() system call) is associated with each mounted
 * filesystem and they must be adhered to by all filesystem types.
 * 	Local filesystems (i.e., those with associated Unix devices)
 * do not use the routines below.  Their device number is the device number
 * of the Unix device associated with the filesystem. The range
 * 0x0000 - 0x7fff is reserved for filesystems of this type.
 * 	Non-local filesystems use the range 0x8000-0xffff. For the major
 * device number, filesystem types which only require one major device number
 * for all mounts use their reserved number which is 0x80 + the index of
 * their filesystem type in the filesystem type table (vfs_conf.c). This
 * number may be obtained by calling the routine vfs_fixedmajor(). Filesystem
 * types requiring more than one major device number should obtain these
 * numbers via calls to vfs_getmajor() and release them via calls to
 * vfs_putmajor(). Minor device numbers are under the control of
 * individual filesystem types. Any filesystem types that wishes may
 * allocate and de-allocate minor device numbers using the routines
 * vfs_getnum() and vfs_putnum() and its own private minor device number map.
 */

#define	LOGBBY 3
#define	MAJOR_MIN 128
#define	MAJOR_MAX 255
#define	NUM_MAJOR (MAJOR_MAX - MAJOR_MIN + 1)
#define	MAJOR_MAPSIZE (NUM_MAJOR/NBBY)
static char devmap[MAJOR_MAPSIZE];    /* Bitmap storing used major device #s */

/*
 * Return the next available major device number from the range 128-255.
 * If a free number is available it is returned. Otherwise the reserved
 * number for the filesystem type is returned.
 */
int
vfs_getmajor(vfsp)
	struct vfs *vfsp;
{
	register int i;

	/* Load the filesystem-reserved numbers */
	for (i = 0; i < (vfsNVFS - vfssw); i++)
		devmap[i >> LOGBBY] |= (1 << (i - ((i >> LOGBBY) << LOGBBY)));

	/* Get the first avalaible number */
	i = vfs_getnum(devmap, MAJOR_MAPSIZE);

	/* If none are available, return the reserved # for this fs type */
	if (i == -1)
		i = vfs_fixedmajor(vfsp);
	else
		i += MAJOR_MIN;
	return (i);
}

/*
 * Return the reserved major device number for this filesystem type
 * defined as its position in the filesystem type table.
 */
int
vfs_fixedmajor(vfsp)
	struct vfs* vfsp;
{
	register struct vfssw *vs;

	for (vs = vfssw; vs < vfsNVFS; vs++) {
		if (vs->vsw_ops == vfsp->vfs_op)
			break;
	}
	return ((vs - vfssw) + MAJOR_MIN);
}

/*
 * Free the major number "num". "num" must have been allocated by a
 * call to vfs_getmajor().
 */
void
vfs_putmajor(vfsp, num)
	struct vfs *vfsp;
	register int num;
{

	num -= MAJOR_MIN;
	if (vfsp->vfs_op == vfssw[num].vsw_ops)
		return;
	vfs_putnum(devmap, num);
}

/*
 * Set and return the first free position from the bitmap "map".
 * Return -1 if no position found.
 */
int
vfs_getnum(map, mapsize)
	register char *map;
	int mapsize;
{
	register int i;
	register char *mp;

	for (mp = map; mp < &map[mapsize]; mp++) {
		if (*mp != (char)0xff) {
			for (i=0; i < NBBY; i++) {
				if (!((*mp >> i) & 0x1)) {
					*mp |= (1 << i);
					return ((mp - map) * NBBY  + i);
				}
			}
		}
	}
	return (-1);
}

/*
 * Clear the designated position "n" in bitmap "map".
 */
void
vfs_putnum(map, n)
	register char *map;
	int n;
{

	if (n >= 0)
		map[n >> LOGBBY] &= ~(1 << (n - ((n >> LOGBBY) << LOGBBY)));
}

/*
 * "sync" all file systems, and return only when all writes have been
 * completed.  For use by the reboot code; it's verbose.
 */
void
vfs_syncall()
{
	register struct buf *bp;
	int iter;
	int nppbusy, nbpbusy;
	struct page *pp;
#ifdef	UFS
	struct inode *ip;
	extern struct inode *ifreeh;
#endif

	printf("syncing file systems... ");
	/*
	 * XXX - do we need to have a way to have sync
	 * force the writeback in the case of keep'ed pages?
	 */
	sync();
#ifdef	UFS
	cleanup();
	/*
	 * A bug in vm/vm_pvn.c (1042607) could prevent us from
	 * syncing all the dirty pages to an inode when we release
	 * the inode to the free list. So here we try to capture these
	 * dirty pages and sync them out before the final shutdown.
	 */
	for (ip = ifreeh; ip != NULL; ip = ip->i_freef) {
		if (ip->i_fs == NULL || ITOV(ip)->v_pages == NULL ||
			ITOV(ip)->v_type == VCHR || ITOV(ip)->v_type == VSOCK)
			continue;
		(void) VOP_PUTPAGE(ITOV(ip), 0, 0, B_ASYNC, (struct ucred *)0);
	}
#endif
	for (iter = 0; iter < 20; iter++) {
		nbpbusy = 0;
		for (bp = bufchain; bp != (struct buf *)0;
		    bp = bp->b_chain) {
			if ((bp->b_flags & (B_BUSY|B_INVAL)) == B_BUSY)
				nbpbusy++;
		}
		nppbusy = 0;
		for (pp = epages; --pp >= pages; )
			if (((pp->p_intrans && !pp->p_pagein) ||
			    pp->p_mod) && !pp->p_free &&
			    pp->p_vnode != NULL &&
			    !IS_SWAPVP(pp->p_vnode)) {
				nppbusy++;
			}
		if (nbpbusy == 0 && nppbusy == 0)
			break;
		if (nbpbusy != 0)
			printf("[%d] ", nbpbusy);
		if (nppbusy != 0)
			printf("%d ", nppbusy);
		DELAY(36000 * iter);
		cleanup();
	}
	if (iter < 20)
		printf("done\n");
	else
		printf("give up!\n");
}
