#ident	"@(#)vfs_syscalls.c 1.1 92/07/30 SMI"

#include <sys/param.h>
#include <sys/label.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/vnode.h>
#include <sys/dirent.h>
#include <sys/audit.h>
#include <sys/kmem_alloc.h>
#include <ufs/inode.h>

extern	struct fileops vnodefops;

/*
 * System call routines for operations on files other
 * than read, write and ioctl.  These calls manipulate
 * the per-process file table which references the
 * networkable version of normal UNIX inodes, called vnodes.
 *
 * Many operations take a pathname, which is read
 * into a kernel buffer by pn_get (see vfs_pathname.c).
 * After preparing arguments for an operation, a simple
 * operation proceeds:
 *
 *	error = lookupname(pname, seg, followlink, &dvp, &vp, &vattr)
 *
 * where pname is the pathname operated on, seg is the segment that the
 * pathname is in (UIO_USERSPACE or UIO_SYSSPACE), followlink specifies
 * whether to follow symbolic links, dvp is a pointer to the vnode that
 * represents the parent directory of vp, the pointer to the vnode
 * referenced by the pathname. vattr is a vattr structure which hold the
 * attributes of the final component. The lookupname routine fetches the
 * pathname string into an internal buffer using pn_get (vfs_pathname.c),
 * and iteratively running down each component of the path until the
 * the final vnode and/or it's parent are found. If either of the addresses
 * for dvp or vp are NULL, then it assumes that the caller is not interested
 * in that vnode. If the pointer to the vattr structure is NULL then attributes
 * are not returned. Once the vnode or its parent is found, then a vnode
 * operation (e.g. VOP_OPEN) may be applied to it.
 *
 * One important point is that the operations on vnode's are atomic, so that
 * vnode's are never locked at this level.  Vnode locking occurs
 * at lower levels either on this or a remote machine. Also permission
 * checking is generally done by the specific filesystem. The only
 * checks done by the vnode layer is checks involving file types
 * (e.g. VREG, VDIR etc.), since this is static over the life of the vnode.
 *
 */

/*
 * Change current working directory (".").
 */
chdir(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{

	u.u_error = chdirec(uap->dirnamep, -1, &u.u_cdir, (au_path_t *)0);
}

/*
 * Change current directory to open directory
 */
fchdir(uap)
	register struct a {
		int	fd;
	} *uap;
{

	u.u_error = chdirec((char *)NULL, uap->fd, &u.u_cdir, (au_path_t *)0);
}

/*
 * Change notion of root ("/") directory.
 */
chroot(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{

	u.u_error = chdirec(uap->dirnamep, -1, &u.u_rdir, (au_path_t *)0);
}

/*
 * Change notion of root to open directory
 * This lets you get back out of a chroot!
 */
fchroot(uap)
	register struct a {
		int	fd;
	} *uap;
{

	u.u_error = chdirec((char *)NULL, uap->fd, &u.u_rdir, (au_path_t *)0);
}

/*
 * Common code for chdir and chroot.
 * Translate the pathname or file descriptor and insist that it
 * is a directory to which we have execute access.
 * If it is replace u.u_[cr]dir with new vnode.
 */
int
chdirec(dirnamep, fd, vpp, au_path)
	char *dirnamep;
	int fd;
	struct vnode **vpp;
	au_path_t *au_path;
{
	struct vnode *vp;		/* new directory vnode */
	register int error;

	if (dirnamep) {
		error = au_lookupname(dirnamep, UIO_USERSPACE, FOLLOW_LINK,
			(struct vnode **)0, &vp, au_path);
		if (error)
			return (error);
	} else {
		struct file *fp;

		error = getvnodefp(fd, &fp);
		if (error)
			return (error);
		vp = (struct vnode *)fp->f_data;
		VN_HOLD(vp);
	}
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, u.u_cred);
	if (error) {
		VN_RELE(vp);
		return (error);
	}
	/*
	 * Special chroot semantics:
	 * chroot is allowed if root or if the target is really
	 * a loopback mount of the root as determined by comparing
	 * dev and inode numbers
	 */
	if (vpp == &u.u_rdir) {
		struct vattr tattr;
		struct vattr rattr;

		if (error = VOP_GETATTR(vp, &tattr, u.u_cred)) {
			VN_RELE(vp);
			return (error);
		}
		if (error = VOP_GETATTR(rootdir, &rattr, u.u_cred)) {
			VN_RELE(vp);
			return (error);
		}
		if ((tattr.va_fsid != rattr.va_fsid ||
		    tattr.va_nodeid != rattr.va_nodeid) && !suser()) {
			error = u.u_error;
			VN_RELE(vp);
			return (error);
		}
	}
	if (*vpp != (struct vnode *)0)
		VN_RELE(*vpp);
	*vpp = vp;
	return (0);
}

/*
 * Open system call.
 */
open(uap)
	register struct a {
		char *fnamep;
		int fmode;
		int cmode;
	} *uap;
{

	u.u_error = copen(uap->fnamep, uap->fmode - FOPEN, uap->cmode);
}

/*
 * Creat system call.
 */
creat(uap)
	register struct a {
		char *fnamep;
		int cmode;
	} *uap;
{

	u.u_error = copen(uap->fnamep, FWRITE|FCREAT|FTRUNC, uap->cmode);
}

/*
 * Common code for open, creat.
 */
static int
copen(pnamep, filemode, createmode)
	char *pnamep;
	int filemode;
	int createmode;
{
	register struct file *fp;
	struct vnode *vp;
	register int error;
	register int indx;

	/*
	 * allocate a user file descriptor and file table entry.
	 */
	fp = falloc();
	if (fp == NULL)
		return (u.u_error);
	indx = u.u_r.r_val1;		/* this is bullshit */
	/*
	 * open the vnode.
	 */
	error = vn_open(pnamep, UIO_USERSPACE, filemode,
	    ((createmode & 07777) & ~u.u_cmask), &vp);

	/*
	 * If there was an error, deallocate the file descriptor.
	 * Otherwise fill in the file table entry to point to the vnode.
	 */
	if (error) {
		u.u_ofile[indx] = NULL;
		crfree(fp->f_cred);
		fp->f_count = 0;
	} else {
		fp->f_flag = filemode & FMASK;
		fp->f_type = DTYPE_VNODE;
		fp->f_data = (caddr_t)vp;
		fp->f_ops = &vnodefops;

		/*
		 * For named pipes, the FNDELAY flag must propagate to the
		 * rdwr layer, for backward compatibility.
		 */
		if (vp->v_type == VFIFO)
			fp->f_flag |= (filemode & FNDELAY);
	}
	return (error);
}

/*
 * Create a special (or regular) file.
 */
mknod(uap)
	register struct a {
		char		*pnamep;
		int		fmode;
		int		dev;
	} *uap;
{
	struct vnode	*vp;
	struct vattr	vattr;
	int		error = 0;

	/* map 0 type into regular file, as other versions of UNIX do */
	if ((uap->fmode & IFMT) == 0)
		uap->fmode |= IFREG;

	/* Must be super-user unless making a FIFO node */
	if (((uap->fmode & IFMT) != IFIFO) && !suser()) {
#ifdef	SYSAUDIT
		au_sysaudit(AUR_MKNOD, AU_MINPRIV, u.u_error, u.u_rval1, 4,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    AUP_USER, (caddr_t)uap->pnamep,
		    sizeof (uap->fmode), (caddr_t)&(uap->fmode));
#endif	SYSAUDIT
		return;
	}

	/*
	 * Setup desired attributes and vn_create the file.
	 */
	vattr_null(&vattr);
	vattr.va_type = MFTOVT(uap->fmode);
	vattr.va_mode = (uap->fmode & 07777) & ~u.u_cmask;

	switch (vattr.va_type) {
	case VDIR:
		error = EISDIR;	/* Can't mknod directories: use mkdir */
		break;

	case VBAD:
	case VCHR:
	case VBLK:
		vattr.va_rdev = uap->dev;
		break;

	case VNON:
		error = EINVAL;
		break;

	default:
		break;
	}

	if (error == 0) {
		error = vn_create(uap->pnamep, UIO_USERSPACE,
			&vattr, EXCL, 0, &vp);
		if (error == 0)
			VN_RELE(vp);
	}
	u.u_error = error;
#ifdef	SYSAUDIT
	au_sysaudit(AUR_MKNOD, AU_DCREATE, error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->pnamep,
	    sizeof (uap->fmode), (caddr_t)&(uap->fmode));
#endif	SYSAUDIT
}

/*
 * Make a directory.
 */
mkdir(uap)
	struct a {
		char	*dirnamep;
		int	dmode;
	} *uap;
{
	struct vnode *vp;
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & 0777) & ~u.u_cmask;

	u.u_error =
	    vn_create(uap->dirnamep, UIO_USERSPACE, &vattr, EXCL, 0, &vp);
	if (u.u_error == 0)
		VN_RELE(vp);
}

/*
 * make a hard link
 */
link(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	u.u_error = vn_link(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * rename or move an existing file
 */
rename(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	u.u_error = vn_rename(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Create a symbolic link.
 * Similar to link or rename except target
 * name is passed as string argument, not
 * converted to vnode reference.
 */
symlink(uap)
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
{
	struct vnode *dvp;
	struct vattr vattr;
	struct pathname tpn;
	struct pathname lpn;

	u.u_error = pn_get(uap->linkname, UIO_USERSPACE, &lpn);
	if (u.u_error)
		return;
	u.u_error = lookuppn(&lpn, NO_FOLLOW, &dvp, (struct vnode **)0);
	if (u.u_error) {
		pn_free(&lpn);
		return;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
		goto out;
	}
	u.u_error = pn_get(uap->target, UIO_USERSPACE, &tpn);
	vattr_null(&vattr);
	vattr.va_mode = 0777;
	if (u.u_error == 0) {
		u.u_error = VOP_SYMLINK(dvp, lpn.pn_path, &vattr,
		    tpn.pn_path, u.u_cred);
		pn_free(&tpn);
	}
out:
	pn_free(&lpn);
	VN_RELE(dvp);
}

/*
 * Unlink (i.e. delete) a file.
 */
unlink(uap)
	struct a {
		char	*pnamep;
	} *uap;
{

	u.u_error = vn_remove(uap->pnamep, UIO_USERSPACE, FILE);
}

/*
 * Remove a directory.
 */
rmdir(uap)
	struct a {
		char	*dnamep;
	} *uap;
{

	u.u_error = vn_remove(uap->dnamep, UIO_USERSPACE, DIRECTORY);
}

/*
 * get directory entries in a file system independent format
 * This is the old system call.  It remains in 4.0 for binary compatibility.
 * It will disappear in 5-?.0. It returns directory entries in the
 * old file-system independent directory entry format. This structure was
 * formerly defined in /usr/include/sys/dir.h. It is now no longer available to
 * user source files. It is defined only by struct direct below.
 *	This system call is superseded by the new getdents() call, which
 * returns directory entries in the new filesystem-independent format
 * given in /usr/include/sys/dirent.h and /usr/include/sys/dir.h. The
 * vn_readdir interface has also been modified to return entries in this
 * format.
 */

getdirentries(uap)
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
		long	*basep;
	} *uap;
{

	struct direct {
		u_long d_fileno;
		u_short d_reclen;
		u_short d_namlen;
		char d_name[MAXNAMLEN + 1];
	};
#define	DIRECTSIZ(dp)  \
	(((sizeof (*(dp)) - (MAXNAMLEN+1) + ((dp)->d_namlen+1)) + 3) & ~3)


	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	register caddr_t ibuf, obuf, ibufend, obufend;
	register struct dirent *idp;
	register struct direct *odp;
	int outcount;
	off_t offset;
#ifdef UFS
	extern struct vnodeops ufs_vnodeops;
#endif
	extern char *strcpy();

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}

	if (uap->count < sizeof (struct direct)) {
		u.u_error = EINVAL;
		return;
	}

#ifdef UFS
	/* Performance hack, use old dir. stuff for local ufs filesystems */
	if (((struct vnode *)fp->f_data)->v_op == &ufs_vnodeops) {
		old_getdirentries(uap, fp);
		return;
	}
#endif

	/* Allocate temporary space for format conversion */
	ibuf = new_kmem_alloc(uap->count, KMEM_SLEEP);
	obuf = new_kmem_alloc(uap->count + sizeof (struct direct), KMEM_SLEEP);
	aiov.iov_base = ibuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_resid = uap->count;

	u.u_error = VOP_READDIR((struct vnode *)fp->f_data, &auio, fp->f_cred);
	if (u.u_error)
		goto out;
	offset = auio.uio_offset;

	/* Convert entries from new back to old format */

	for (idp = (struct dirent *)ibuf, odp = (struct direct *)obuf,
	    ibufend = ibuf + (uap->count - auio.uio_resid),
	    obufend = obuf + uap->count;
	    (caddr_t)idp < ibufend;
	    idp = (struct dirent *)((caddr_t)idp + idp->d_reclen),
	    odp = (struct direct *)((caddr_t)odp + odp->d_reclen)) {
		odp->d_fileno = idp->d_fileno;
		odp->d_namlen = idp->d_namlen;
		(void) strcpy(odp->d_name, idp->d_name);
		odp->d_reclen = DIRECTSIZ(odp);
		if ((caddr_t)odp + odp->d_reclen > obufend)
			break;
	}

	outcount = (caddr_t)odp - obuf;
	aiov.iov_base = uap->buf;
	aiov.iov_len = outcount;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = outcount;

	if (u.u_error = uiomove(obuf, outcount, UIO_READ, &auio))
		goto out;

	u.u_error =
	    copyout((caddr_t)&fp->f_offset, (caddr_t)uap->basep, sizeof (long));
	u.u_r.r_val1 = outcount - auio.uio_resid;
	fp->f_offset = offset;
out:
	kmem_free(ibuf, uap->count);
	kmem_free(obuf, uap->count + sizeof (struct direct));
}

#ifdef UFS
/*
 * old form of the getdirentries system call. This is called by
 * getdirentries() when it discovers it is dealing with a ufs filesystem.
 * This routine in turn calls the old ufs_readdir routine directly. This
 * ugly hack is to avoid a big performance penalty that occurs with local ufs
 * filesystems because the directory entries have to be converted to new
 * format by ufs_readdir and then back to the old format by getdirentries.
 * By using this calling method no conversion takes place and no penalties
 * are incurred. Unsightly, but it will go away soon, we hope!
 */
old_getdirentries(uap, fp)
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
		long	*basep;
	} *uap;
	struct file *fp;
{
	struct uio auio;
	struct iovec aiov;
	extern int old_ufs_readdir();

	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	u.u_error =
	    old_ufs_readdir((struct vnode *)fp->f_data, &auio, fp->f_cred);
	if (u.u_error)
		return;
	u.u_error =
	    copyout((caddr_t)&fp->f_offset, (caddr_t)uap->basep, sizeof (long));
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	fp->f_offset = auio.uio_offset;
}
#endif

/*
 * get directory entries in a file system independent format.
 * This call returns directory entries in the new format specified
 * in sys/dirent.h which is also the format returned by the vn_readdir
 * interface. This call supersedes getdirentries(), which will disappear
 * in 5-?.0.
 */
getdents(uap)
	register struct a {
		int	fd;
		char	*buf;
		unsigned count;
	} *uap;
{
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct vnode *vp;

	if (uap->count < sizeof (struct dirent)) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	vp = (struct vnode *)fp->f_data;
	if ((fp->f_flag & FREAD) == 0) {
		u.u_error = EBADF;
		return;
	}
	if (vp->v_type != VDIR) {
		u.u_error = ENOTDIR;
		return;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READDIR(vp, &auio, fp->f_cred);
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->count - auio.uio_resid;
	fp->f_offset = auio.uio_offset;
}

/*
 * Seek on file.  Only hard operation is seek relative to
 * end which must apply to vnode for current file size.
 *
 * Note: lseek(0, 0, L_XTND) costs much more than it did before.
 */

lseek(uap)
	register struct a {
		int	fd;
		off_t	off;
		int	sbase;
	} *uap;
{
	register struct file *fp;
	register struct vnode *vp;

	GETF(fp, uap->fd);
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = ESPIPE;
		return;
	}

	vp = (struct vnode *)fp->f_data;
	if (vp->v_type == VFIFO) {
		u.u_error = ESPIPE;
		return;
	}
	u.u_error = lseek1(fp, uap->off, uap->sbase);
	u.u_r.r_off = fp->f_offset;
}

/*
 * Common code called from lseek and arw
 */

#define	TWO_GB_BLKOFFSET \
		(1 << sizeof (off_t) * NBBY - DEV_BSHIFT - 1)

int
lseek1(fp, delta, whence)
	struct file *fp;
	off_t delta;
	int whence;
{
	off_t offset0 = fp->f_offset;
	int blockmode = ((fp->f_flag & FSETBLK) != 0);
	struct vnode *vp = (struct vnode *)fp->f_data;
	struct vattr vattr;
	int error = 0;

	/*
	 * Block-mode seeks are limited to character special files
	 */
	if (whence == LB_SET || whence == LB_INCR || whence == LB_XTND) {
		if (vp->v_type != VCHR) 
			return (EINVAL);
		fp->f_flag |= FSETBLK;
	} else {
		fp->f_flag &= ~FSETBLK;
	}

	switch (whence) {
	case L_INCR:
		if (!blockmode) {
			fp->f_offset += delta;
		} else if ((u_long)offset0 >= TWO_GB_BLKOFFSET && delta >= 0) {
			error = EFBIG;
			break;
		} else {
			off_t dstblk = offset0 +
			    delta / DEV_BSIZE - (delta % DEV_BSIZE < 0);
			if ((u_long)dstblk >= TWO_GB_BLKOFFSET) {
				error = EFBIG;
				break;
			}
			fp->f_offset = dbtob(dstblk) + delta % DEV_BSIZE;
			if (delta % DEV_BSIZE < 0) 
				fp->f_offset += DEV_BSIZE;
		}
		break;

	case L_XTND: 
		error = VOP_GETATTR(vp, &vattr, u.u_cred);
		if (error)
			break;
		fp->f_offset = delta + vattr.va_size;
		break;

	case L_SET:
	case LB_SET:
		fp->f_offset = delta;
		break;

	case LB_INCR:
		/*
		 * LB_INCR succeeds only if the file is already in block
		 * mode or the current byte offset is on a block boundary
		 */
		if (!blockmode) {
			if (offset0 % DEV_BSIZE != 0) {
				error = EINVAL;
				break;
			}
			fp->f_offset = btodb(offset0);
		}
		fp->f_offset += delta;
		break;

	case LB_XTND: 
		error = VOP_GETATTR(vp, &vattr, u.u_cred);
		if (error)
			break;
		if (vattr.va_size % DEV_BSIZE != 0) {
			error = EINVAL;
			break;
		}
		fp->f_offset = delta + btodb(vattr.va_size);
		break;

	default:
		error = EINVAL;
	}
	/*
	 * only char special devices are allowed negative offsets
	 */
	if (fp->f_offset < 0 && vp->v_type != VCHR)
		error = EINVAL;

	if (error) {
		fp->f_offset = offset0;
		if (blockmode)
			fp->f_flag |= FSETBLK;
		else
			fp->f_flag &= ~FSETBLK;
	}
	return (error);
}

/*
 * Determine accessibility of file, by
 * reading its attributes and then checking
 * against our protection policy.
 */
access(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vnode *vp;
	u_short mode;
	struct ucred *tmpcred;
	struct ucred *savecred;

	/*
	 * posix check for invalid mode, weird because F_OK == 0
	 */
	if (uap->fmode != F_OK && (uap->fmode & ~(R_OK|W_OK|X_OK))) {
	    u.u_error = EINVAL;
	    return;
	}

	/*
	 * Use the real uid and gid and check access
	 */
	if (u.u_ruid == u.u_uid && u.u_rgid == u.u_gid) {
		tmpcred = u.u_cred;
		savecred = NULL;
	} else {
		savecred = u.u_cred;
		u.u_cred = tmpcred = crdup(savecred);
		u.u_uid = u.u_ruid;
		u.u_gid = u.u_rgid;
	}

	/*
	 * Lookup file
	 */
	u.u_error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (u.u_error) {
		if (savecred != NULL) {
			u.u_cred = savecred;
			crfree(tmpcred);
		}
		return;
	}

	mode = 0;
	/*
	 * fmode == 0 means only check for exist
	 */
	if (uap->fmode) {
		if (uap->fmode & R_OK)
			mode |= VREAD;
		if (uap->fmode & W_OK) {
			if (isrofile(vp)) {
				u.u_error = EROFS;
				goto out;
			}
			mode |= VWRITE;
		}
		if (uap->fmode & X_OK)
			mode |= VEXEC;
		u.u_error = VOP_ACCESS(vp, mode, tmpcred);
	}

	/*
	 * release the vnode and free the temporary credential
	 */
out:
	VN_RELE(vp);
	if (savecred != NULL) {
		u.u_cred = savecred;
		crfree(tmpcred);
	}
}

/*
 * Get attributes from file or file descriptor.
 * Argument says whether to follow links, and is
 * passed through in flags.
 */
stat(uap)
	caddr_t uap;
{

	u.u_error = stat1(uap, FOLLOW_LINK);
}

lstat(uap)
	caddr_t uap;
{

	u.u_error = stat1(uap, NO_FOLLOW);
}

static int
stat1(uap0, follow)
	caddr_t uap0;
	enum symfollow follow;
{
	struct vnode *vp;
	struct stat sb;
	register int error;
	register struct a {
		char	*fname;
		struct	stat *ub;
	} *uap = (struct a *)uap0;

	error =
	    lookupname(uap->fname, UIO_USERSPACE, follow,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	error = vno_stat(vp, &sb);
	VN_RELE(vp);
	if (error)
		return (error);
	return (copyout((caddr_t)&sb, (caddr_t)uap->ub, sizeof (sb)));
}

/*
 * Read contents of symbolic link.
 */
readlink(uap)
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;
{
	struct vnode *vp;
	struct iovec aiov;
	struct uio auio;

	u.u_error = lookupname(uap->name, UIO_USERSPACE, NO_FOLLOW,
	    (struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VLNK) {
		u.u_error = EINVAL;
		goto out;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	u.u_error = VOP_READLINK(vp, &auio, u.u_cred);
out:
	VN_RELE(vp);
	u.u_r.r_val1 = uap->count - auio.uio_resid;
}

/*
 * Change mode of file given path name.
 */
chmod(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 0);
}

/*
 * Change mode of file given file descriptor.
 */
fchmod(uap)
	register struct a {
		int	fd;
		int	fmode;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_mode = uap->fmode & 07777;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Change ownership of file given file name.
 */
chown(uap)
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = namesetattr(uap->fname, NO_FOLLOW, &vattr, 0);
}

/*
 * Change ownership of file given file descriptor.
 */
fchown(uap)
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;
{
	struct vattr vattr;

	vattr_null(&vattr);
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	u.u_error = fdsetattr(uap->fd, &vattr);
}

/*
 * Set access/modify times on named file.
 */
utimes(uap)
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap;
{
	struct timeval tv[2];
	struct vattr vattr;
	int flags = 0;

	vattr_null(&vattr);
	if (uap->tptr != NULL) {
		u.u_error =
		    copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
		if (u.u_error)
			return;
		if (tv[0].tv_usec < 0 || tv[0].tv_usec >= 1000000 ||
		    tv[1].tv_usec < 0 || tv[1].tv_usec >= 1000000) {
			u.u_error = EINVAL;
			return;
		}
		vattr.va_atime = tv[0];
		vattr.va_mtime = tv[1];
		flags |= ATTR_UTIME;
	} else {
		/*
		 * If the timeval ptr is NULL, then this is the
		 * SystemV-compatible call to set the access
		 * and modified times to the present.
		 *
		 * XXX - For now, flag this with -1 in vattr.va_mtime.tv_usec.
		 */
		vattr.va_mtime.tv_sec = 0;
		vattr.va_mtime.tv_usec = -1;
	}
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, flags);
}

/*
 * Truncate a file given its path name.
 */
truncate(uap)
	register struct a {
		char	*fname;
		off_t	length;
	} *uap;
{
	struct vattr vattr;

	if (uap->length < 0) {
		u.u_error = EINVAL;
		return;
	}
	vattr_null(&vattr);
	vattr.va_size = (u_long)uap->length;
	u.u_error = namesetattr(uap->fname, FOLLOW_LINK, &vattr, 0);
}

/*
 * Truncate a file given a file descriptor.
 */
ftruncate(uap)
	register struct a {
		int	fd;
		off_t	length;
	} *uap;
{
	register struct vnode *vp;
	struct file *fp;

	if (uap->length < 0) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;
	vp = (struct vnode *)fp->f_data;
	if ((fp->f_flag & FWRITE) == 0) {
		u.u_error = EINVAL;
	} else if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		u.u_error = EROFS;
	} else {
		struct vattr vattr;

		vattr_null(&vattr);
		vattr.va_size = (u_long)uap->length;
		u.u_error = VOP_SETATTR(vp, &vattr, fp->f_cred);
	}
}

/*
 * Common routine for modifying attributes
 * of named files.
 */
int
namesetattr(fnamep, followlink, vap, flags)
	char *fnamep;
	enum symfollow followlink;
	struct vattr *vap;
	int flags;
{
	struct vnode *vp;
	register int error;

	error = lookupname(fnamep, UIO_USERSPACE, followlink,
		(struct vnode **)0, &vp);
	if (error)
		return (error);
	/*
	 * XXX - Allow either owner or root to set access and
	 * modification times if flags is ATTR_UTIME.
	 */
	if (flags & ATTR_UTIME) {
		struct vattr vtmp;

		error = VOP_GETATTR(vp, &vtmp, u.u_cred);
		if (error)
			goto out;
		if (u.u_cred->cr_uid != vtmp.va_uid && u.u_cred->cr_uid != 0) {
			error = EPERM;
			goto out;
		}
	}
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/* If truncating, check for write permission */
	if (vap->va_size != (u_long)-1) {
		error = VOP_ACCESS(vp, VWRITE, u.u_cred);
		if (error)
			goto out;
	}
	error = VOP_SETATTR(vp, vap, u.u_cred);
out:
	VN_RELE(vp);
	return (error);
}

/*
 * Common routine for modifying attributes
 * of file referenced by descriptor.
 */
static int
fdsetattr(fd, vap)
	int fd;
	struct vattr *vap;
{
	struct file *fp;
	register struct vnode *vp;
	register int error;

	error = getvnodefp(fd, &fp);
	if (error == 0) {
		vp = (struct vnode *)fp->f_data;
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return (EROFS);
		/* If truncating, check for write permission */
		if (vap->va_size != (u_long)-1) {
			if ((fp->f_flag & FWRITE) == 0)
				return (EINVAL);
		}
		error = VOP_SETATTR(vp, vap, u.u_cred);
	}
	return (error);
}

/*
 * Flush output pending for file.
 */
fsync(uap)
	struct a {
		int	fd;
	} *uap;
{
	struct file *fp;

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error == 0)
		u.u_error = VOP_FSYNC((struct vnode *)fp->f_data, fp->f_cred);
}

/*
 * Set file creation mask.
 */
umask(uap)
	register struct a {
		int mask;
	} *uap;
{

	u.u_r.r_val1 = u.u_cmask;
	u.u_cmask = uap->mask & 07777;
}

/*
 * Revoke access the current tty by all processes.
 * Used only by the super-user in init
 * to give ``clean'' terminals at login.
 */
vhangup()
{

	if (!suser())
		return;
	if (u.u_procp->p_sessp->s_ttyp == NULL)
		return;
	forceclose(u.u_procp->p_sessp->s_ttyd);
	gsignal(*u.u_procp->p_sessp->s_ttyp, SIGHUP);
}

forceclose(dev)
	dev_t dev;
{
	register struct file *fp;
	register struct vnode *vp;

	for (fp = file; fp < fileNFILE; fp++) {
		if (fp->f_count == 0)
			continue;
		if (fp->f_type != DTYPE_VNODE)
			continue;
		vp = (struct vnode *)fp->f_data;
		if (vp == 0)
			continue;
		if (vp->v_type != VCHR)
			continue;
		if (vp->v_rdev != dev)
			continue;
		/*
		 * Note that while this prohibits further I/O on the
		 * descriptor, it does not prohibit closing the
		 * descriptor.
		 */
		fp->f_flag &= ~(FREAD|FWRITE);
	}
}

/*
 * Get the file structure entry for the file descriptor, but make sure
 * it's a vnode.
 */
int
getvnodefp(fd, fpp)
	int fd;
	struct file **fpp;
{
	register struct file *fp;

	if (fd < 0 || fd > u.u_lastfile || (fp = u.u_ofile[fd]) == NULL)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);
	*fpp = fp;
	return (0);
}
