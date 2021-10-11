#ident  "@(#)ufs_vfsops.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/vaccess.h>
#include <sys/lockfs.h>
#include <ufs/fs.h>
#include <ufs/mount.h>
#include <ufs/inode.h>
#include <ufs/lockfs.h>
#undef NFS
#include <sys/mount.h>
#include <sys/bootconf.h>
#include <sys/reboot.h>
#include <vm/swap.h>

/*
 * ufs vfs operations.
 */
static int ufs_mount();
static int ufs_unmount();
static int ufs_root();
static int ufs_statfs();
static int ufs_sync();
static int ufs_vget();
static int ufs_mountroot();
static int ufs_badop();

struct vfsops ufs_vfsops = {
	ufs_mount,
	ufs_unmount,
	ufs_root,
	ufs_statfs,
	ufs_sync,
	ufs_vget,
	ufs_mountroot,
	ufs_badop,		/* XXX - swapvp */
};

/*
 * Mount table.
 */
struct mount	*mounttab = NULL;
int	mounttab_flags = 0;
int	mounttab_rlock = 0;

/*
 * ufs_mount system call
 */
static int
ufs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	dev_t dev;
	struct vnode *devvp;
	struct ufs_args args;

	/*
	 * Get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (struct ufs_args));
	if (error) {
		return (error);
	}
	if ((error = getmdev(args.fspec, &dev)) != 0)
		return (error);
	/*
	 * make a special (device) vnode for the filesystem
	 */
	devvp = bdevvp(dev);

	/*
	 * If the device is a tape, mount it read only
	 */
	if ((bdevsw[major(dev)].d_flags & B_TAPE) == B_TAPE)
		vfsp->vfs_flag |= VFS_RDONLY;

	/*
	 * Mount the filesystem.
	 */
	error = mountfs(&devvp, path, vfsp);
	if (error) {
		VN_RELE(devvp);
	}
	return (error);
}

/*
 * Called by vfs_mountroot when ufs is going to be mounted as root
 */
static int
ufs_mountroot(vfsp, vpp, name)
	struct vfs *vfsp;
	struct vnode **vpp;
	char *name;
{
	register struct fs *fsp;
	register int error;
	static int ufsrootdone = 0;
	extern dev_t getblockdev();

	if (ufsrootdone) {
		return (EBUSY);
	}
	rootdev = getblockdev("root", name);
	if (rootdev == (dev_t)0) {
		return (ENODEV);
	}
	if ((boothowto & RB_WRITABLE) == 0) {
		/*
		 * We mount a ufs root file system read-only to
		 * avoid problems during fsck.   After fsck runs,
		 * we remount it read-write.
		 */
		vfsp->vfs_flag |= VFS_RDONLY;
	}
	*vpp = bdevvp(rootdev);
	error = mountfs(vpp, "/", vfsp);
	if (error) {
		VN_RELE(*vpp);
		*vpp = (struct vnode *)0;
		return (error);
	}
	error = vfs_add((struct vnode *)0, vfsp,
			(vfsp->vfs_flag & VFS_RDONLY) ? M_RDONLY : 0);
	if (error) {
		(void) unmount1(vfsp);
		VN_RELE(*vpp);
		*vpp = (struct vnode *)0;
		return (error);
	}
	ufsrootdone++;		/* Lock out further mounts */
	vfs_unlock(vfsp);
	fsp = ((struct mount *)(vfsp->vfs_data))->m_bufp->b_un.b_fs;
	inittodr(fsp->fs_time);
	return (0);
}

static int
mountfs(devvpp, path, vfsp)
	struct vnode **devvpp;
	char *path;
	struct vfs *vfsp;
{
	register struct fs *fsp;
	register struct mount *mp = 0;
	register struct buf *bp = 0;
	struct buf *tp = 0;
	int error;
	u_int len;
	static int initdone = 0;
	int mounttablocked = 0;
	extern maxphys;

	if (!initdone) {
		ihinit();
		initdone = 1;
	}

	/*
	 * Open block device mounted on.
	 * When bio is fixed for vnodes this can all be vnode operations
	 */
	error = VOP_OPEN(devvpp,
	    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, u.u_cred);
	if (error) {
		return (error);
	}

	/*
	 * Refuse to go any further if this
	 * device is being used for swapping.
	 */
	if (IS_SWAPVP(*devvpp)) {
		error = EBUSY;
		goto out;
	}

	/*
	 * Flush back any dirty pages on the block device to
	 * try and keep the buffer cache in sync with the page
	 * cache if someone is trying to use block devices when
	 * they really should be using the raw device.
	 */
	(void) VOP_PUTPAGE(*devvpp, 0, 0, B_INVAL, u.u_cred);
	bsinval(*devvpp);

	/*
	 * Must acquire mounttab lock *before* acquiring superblock buf lock
	 */
	MWLOCK();
	mounttablocked = 1;

	/*
	 * read superblock from disk
	 */
	tp = bread(*devvpp, SBLOCK, SBSIZE);
	if (tp->b_flags & B_ERROR) {
		goto out;
	}
	/*
	 * check for dev already mounted on
	 */
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
		if (mp->m_bufp != 0 && (*devvpp)->v_rdev == mp->m_dev) {
			if (vfsp->vfs_flag & VFS_REMOUNT) {
				bp = mp->m_bufp;
				goto modify_now;
			} else {
				mp = 0;
				error = EBUSY;
				goto out;
			}
		}
	}
	/*
	 * If this is a remount request, and we didn't spot the mounted
	 * device, then we're in error here....
	 */
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		vfsp->vfs_flag &= ~VFS_REMOUNT;
		printf("mountfs: illegal remount request\n");
		error = EINVAL;
		goto out;
	}
	/*
	 * get new mount table entry and link after the first entry
	 */
	mp = (struct mount *)new_kmem_zalloc(
		sizeof (struct mount), KMEM_SLEEP);
	mp->m_ul = (struct ulockfs *)new_kmem_zalloc(
		sizeof (struct ulockfs), KMEM_SLEEP);
	if (mounttab) {
		mp->m_nxt = mounttab->m_nxt;
		mounttab->m_nxt = mp;
	} else {
		mp->m_nxt = mounttab;
		mounttab = mp;
	}

	vfsp->vfs_data = (caddr_t)mp;
	mp->m_vfsp = vfsp;
	mp->m_bufp = tp;	/* just to reserve this slot */
	mp->m_dev = NODEV;
	mp->m_devvp = *devvpp;
	fsp = tp->b_un.b_fs;
	if (fsp->fs_bsize * fsp->fs_maxcontig > maxphys) {
		error = EIO;	/* drivers can't handle this */
		goto out;
	}
	if (fsp->fs_magic != FS_MAGIC || fsp->fs_bsize > MAXBSIZE ||
	    fsp->fs_bsize < sizeof (struct fs) || fsp->fs_bsize < PAGESIZE/2) {
		error = EINVAL;	/* also needs translation */
		goto out;
	}
	/*
	 * Copy the super block into a buffer in it's native size.
	 * Use getpermeblk to allocate the buffer on a semi-permanent
	 * basis.
	 */
	bp = getpermeblk((int)fsp->fs_sbsize);
	mp->m_bufp = bp;
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
			(u_int)fsp->fs_sbsize);
	brelse(tp);
	tp = 0;
modify_now:
	fsp = bp->b_un.b_fs;
	/*
	 * Curently we only allow a remount to change from
	 * read-only to read-write.
	 */
	if (vfsp->vfs_flag & VFS_RDONLY) {
		if (vfsp->vfs_flag & VFS_REMOUNT) {
			printf ("mountfs: can't remount ro\n");
			error = EINVAL;
			goto out;
		}
		fsp->fs_ronly = 1;
	} else {
		if (vfsp->vfs_flag & VFS_REMOUNT) {
			/*
			 * set appropriate fs_clean bits and release superblock
			 */
			ufs_checkremount(mp, tp);

			vfsp->vfs_flag &= ~VFS_REMOUNT;
			MUNLOCK();
			(void) VOP_CLOSE(*devvpp, FREAD|FWRITE, 1, u.u_cred);
			return (0);
		}
		fsp->fs_fmod = 0;
		fsp->fs_ronly = 0;
	}
	vfsp->vfs_bsize = fsp->fs_bsize;
	/*
	 * Read in cyl group info
	 */
	error = ufs_getsummaryinfo(mp, fsp);
	if (error)
		goto out;
	mp->m_dev = mp->m_devvp->v_rdev;
	vfsp->vfs_fsid.val[0] = (long)mp->m_dev;
	vfsp->vfs_fsid.val[1] = MOUNT_UFS;
	(void) copystr(path, fsp->fs_fsmnt, sizeof (fsp->fs_fsmnt) - 1, &len);
	bzero(fsp->fs_fsmnt + len, sizeof (fsp->fs_fsmnt) - len);

	/* Sanity checks for old file systems.			   XXX */
	fsp->fs_npsect = MAX(fsp->fs_npsect, fsp->fs_nsect);    /* XXX */
	fsp->fs_interleave = MAX(fsp->fs_interleave, 1);	/* XXX */
	if (fsp->fs_postblformat == FS_42POSTBLFMT)		/* XXX */
		fsp->fs_nrpos = 8;				/* XXX */
	/*
	 * set appropriate fs_clean bits
	 */
	ufs_checkmount(mp);

	MUNLOCK();

	return (0);
out:
	if (error == 0)
		error = EIO;
	if (mp)
		mp->m_bufp = 0;
	if (bp)
		freepermeblk(bp);
	if (tp)
		brelse(tp);
	(void) VOP_CLOSE(*devvpp, (vfsp->vfs_flag & VFS_RDONLY) ?
		    FREAD : FREAD|FWRITE, 1, u.u_cred);
	/*
	 * When the device is closed, spec_close will binval all the buffers
	 */
	if (mounttablocked)
		MUNLOCK();
	return (error);
}

/*
 * The System V Interface Definition requires a "umount" operation
 * which takes a device pathname as an argument.  This requires this
 * to be a system call.
 */
umount(uap)
	struct a {
		char	*fspec;
	} *uap;
{
	register struct mount *mp;
	dev_t dev;

	if (!suser())
		return;

	if ((u.u_error = getmdev(uap->fspec, &dev)) != 0)
		return;

	if ((mp = getmp(dev)) == NULL) {
		u.u_error = EINVAL;
		return;
	}

	dounmount(mp->m_vfsp);
}

/*
 * vfs operations
 */
static int
ufs_unmount(vfsp)
	struct vfs *vfsp;
{

	return (unmount1(vfsp));
}

static int
unmount1(vfsp)
	register struct vfs *vfsp;
{
	dev_t dev;
	register struct mount *mp;
	register struct mount *mmp;
	register struct fs *fs;
	int flag;
	int forced;
	struct ulockfs *ul;
	struct lockfs *lf;
	int error;

	mp = (struct mount *)vfsp->vfs_data;
	dev = mp->m_dev;
	/*
	 * forced unmount if fs is hlock'ed
	 */
	ul = mp->m_ul;
	lf = UTOL(ul);
	if (forced = (LOCKFS_IS_HLOCK(lf) && !LOCKFS_IS_BUSY(lf)))
		if (error = ufs_lockfs_fumount(ul))
			return (error);
	if (iflush(dev, forced, mp->m_qinod))
		return (EBUSY);
#ifdef QUOTA
	(void) closedq(mp);
	/*
	 * Here we have to iflush again to get rid of the quota inode.
	 * A drag, but it would be ugly to cheat, & this doesn't happen often
	 */
	(void) iflush(dev, forced, (struct inode *)NULL);
#endif
	MWLOCK();
	fs = mp->m_bufp->b_un.b_fs;
	if ((fs->fs_ronly == 0) && (fs->fs_fmod)) {
		fs->fs_fmod = 0;
		sbupdate(mp);
	}
	bsinval(mp->m_devvp);

	/*
	 * set appropriate fs_clean bits
	 */
	ufs_checkunmount(mp);
	bsinval(mp->m_devvp);

	kmem_free((caddr_t)fs->fs_csp[0], (u_int)fs->fs_cssize);
	flag = !fs->fs_ronly;
	freepermeblk(mp->m_bufp);
	mp->m_bufp = 0;
	mp->m_dev = 0;

	(void) VOP_CLOSE(mp->m_devvp, flag, 1, u.u_cred);
	bsinval(mp->m_devvp);
	VN_RELE(mp->m_devvp);
	mp->m_devvp = (struct vnode *)0;
	if (mp != mounttab) {
		for (mmp = mounttab; mmp != NULL; mmp = mmp->m_nxt) {
			if (mmp->m_nxt == mp) {
				mmp->m_nxt = mp->m_nxt;
			}
		}
		kmem_free((caddr_t)mp, sizeof (struct mount));
	}
	kmem_free(lf->lf_comment, (u_int)lf->lf_comlen);
	kmem_free((caddr_t)ul, sizeof (struct ulockfs));
	MUNLOCK();
	return (0);
}

/*
 * find root of ufs
 */
static int
ufs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct mount *mp;
	struct inode *ip;
	int err;

	VFS_RECORD(vfsp, VS_ROOT, VS_CALL);

	if (err = ufs_lockfs_hold(vfsp))
		return (err);

	mp = (struct mount *)vfsp->vfs_data;
	if (err = iget(mp->m_dev, mp->m_bufp->b_un.b_fs, (ino_t)ROOTINO, &ip))
		goto out;
	IUNLOCK(ip);
	*vpp = ITOV(ip);
out:
	ufs_lockfs_rele(vfsp);
	return (err);
}

/*
 * Get file system statistics.
 */
static int
ufs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	register struct fs *fsp;
	int err;

	VFS_RECORD(vfsp, VS_STATFS, VS_CALL);

	if (err = ufs_lockfs_hold(vfsp))
		return (err);

	fsp = ((struct mount *)vfsp->vfs_data)->m_bufp->b_un.b_fs;
	if (fsp->fs_magic != FS_MAGIC)
		panic("ufs_statfs");
	sbp->f_bsize = fsp->fs_fsize;
	sbp->f_blocks = fsp->fs_dsize;
	sbp->f_bfree = fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
	    fsp->fs_cstotal.cs_nffree;
	/*
	 * avail = MAX(max_avail - used, 0)
	 */
	sbp->f_bavail =
	    fsp->fs_dsize - (fsp->fs_dsize / 100) * fsp->fs_minfree -
	    (fsp->fs_dsize % 100) * fsp->fs_minfree / 100 -
	    (fsp->fs_dsize - sbp->f_bfree);
	/*
	 * inodes
	 */
	sbp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	sbp->f_ffree = fsp->fs_cstotal.cs_nifree;
	sbp->f_fsid = vfsp->vfs_fsid;
	ufs_lockfs_rele(vfsp);
	return (0);
}

/*
 * Flush any pending I/O to file system vfsp.
 * The update() routine will only flush *all* ufs files.
 */
/*ARGSUSED*/
static int
ufs_sync(vfsp)
	struct vfs *vfsp;
{

	update();
	return (0);
}

sbupdate(mp)
	struct mount *mp;
{
	register struct fs *fs = mp->m_bufp->b_un.b_fs;
	register struct buf *bp;
	int blks;
	caddr_t space;
	int i, size;

	/*
	 * for ulockfs processing, limit the superblock writes
	 */
	if ((mp->m_ul->ul_sbowner) && (u.u_procp != mp->m_ul->ul_sbowner))
		return;
	LOCKFS_SET_MOD(UTOL(mp->m_ul));

	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
	ufs_sbwrite(mp, fs, bp);
	blks = howmany(fs->fs_cssize, fs->fs_fsize);
	space = (caddr_t)fs->fs_csp[0];
	for (i = 0; i < blks; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > blks)
			size = (blks - i) * fs->fs_fsize;
		bp = getblk(mp->m_devvp, (daddr_t)fsbtodb(fs, fs->fs_csaddr+i),
		    size);
		bcopy(space, bp->b_un.b_addr, (u_int)size);
		space += size;
		bwrite(bp);
	}
}

/*
 * Common code for mount and umount.
 * Check that the user's argument is a reasonable
 * thing on which to mount, and return the device number if so.
 */
static int
getmdev(fspec, pdev)
	char *fspec;
	dev_t *pdev;
{
	register int error;
	struct vnode *vp;

	/*
	 * Get the device to be mounted
	 */
	error = lookupname(fspec, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (error) {
		if (u.u_error == ENOENT)
			return (ENODEV);	/* needs translation */
		return (error);
	}
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	*pdev = vp->v_rdev;
	VN_RELE(vp);
	if (major(*pdev) >= nblkdev)
		return (ENXIO);
	return (0);
}

static int
ufs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	register struct ufid *ufid;
	register struct mount *mp;
	struct inode *ip;

	*vpp = NULL;

	if (ufs_lockfs_hold(vfsp))
		return (0);

	mp = (struct mount *)vfsp->vfs_data;
	ufid = (struct ufid *)fidp;
	if (iget(mp->m_dev, mp->m_bufp->b_un.b_fs, ufid->ufid_ino, &ip))
		goto out;
	if (ip->i_gen != ufid->ufid_gen) {
		idrop(ip);
		goto out;
	}
	IUNLOCK(ip);
	*vpp = ITOV(ip);
	if ((ip->i_mode & ISVTX) && !(ip->i_mode & (IEXEC | IFDIR))) {
		(*vpp)->v_flag |= VISSWAP;
	} else {
		(*vpp)->v_flag &= ~VISSWAP;
	}
out:
	ufs_lockfs_rele(vfsp);
	return (0);
}

static int
ufs_badop()
{

	return (EINVAL);
}
