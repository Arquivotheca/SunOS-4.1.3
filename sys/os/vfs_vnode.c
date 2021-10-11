/*	@(#)vfs_vnode.c 1.1 92/07/30 SMI 	*/

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/file.h>
#include <sys/pathname.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/stat.h>

/*
 * read or write a vnode
 */
int
vn_rdwr(rw, vp, base, len, offset, seg, ioflag, aresid)
	enum uio_rw rw;
	struct vnode *vp;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int ioflag;
	int *aresid;
{

	struct uio auio;
	struct iovec aiov;
	int error;

	if ((rw == UIO_WRITE) && isrofile(vp)) {
		return (EROFS);
	}

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = seg;
	auio.uio_resid = len;
	error = VOP_RDWR(vp, &auio, rw, ioflag, u.u_cred);
	if (aresid)
		*aresid = auio.uio_resid;
	else
		if (auio.uio_resid)
			error = EIO;
	return (error);
}

/*
 * Release a vnode.  Decrements reference count
 * and calls VOP_INACTIVE on last.
 */
void
vn_rele(vp)
	register struct vnode *vp;
{

	if (vp->v_count == 0)			/* sanity check */
		panic("vn_rele");
	if (--vp->v_count == 0)
		VOP_INACTIVE(vp, u.u_cred);
}

/*
 * Open/create a vnode.
 * This may be callable by the kernel, the only known side effect being that
 * the current user uid and gid are used for permissions.
 */
int
vn_open(pnamep, seg, filemode, createmode, vpp)
	char *pnamep;
	register int filemode;
	int createmode;
	struct vnode **vpp;
{
	struct vnode *vp;		/* ptr to file vnode */
	register int mode;
	register int error;

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE | FTRUNC))
		mode |= VWRITE;

	if (filemode & FCREAT) {
		struct vattr vattr;
		enum vcexcl excl;

		/*
		 * Wish to create a file.
		 */
		vattr_null(&vattr);
		vattr.va_type = VREG;
		vattr.va_mode = createmode;
		if (filemode & FTRUNC)
			vattr.va_size = 0;
		if (filemode & FEXCL)
			excl = EXCL;
		else
			excl = NONEXCL;
		filemode &= ~(FCREAT | FTRUNC | FEXCL);

		error = vn_create(pnamep, seg, &vattr, excl, mode, &vp);
		if (error)
			return (error);
	} else {
		/*
		 * Wish to open a file.
		 * Just look it up.
		 */
		error = lookupname(pnamep, seg, FOLLOW_LINK,
		    (struct vnode **)0, &vp);
		if (error)
			return (error);
		/*
		 * cannnot write directories, active texts or
		 * read only filesystems
		 */
		if (filemode & (FWRITE | FTRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (isrofile(vp)) {
				error = EROFS;
				goto out;
			}
		}
		/*
		 * check permissions
		 */
		error = VOP_ACCESS(vp, mode, u.u_cred);
		if (error)
			goto out;
		/*
		 * Sockets in filesystem name space are not supported (yet?)
		 */
		if (vp->v_type == VSOCK) {
			error = EOPNOTSUPP;
			goto out;
		}
	}
	/*
	 * do opening protocol.
	 */
	error = VOP_OPEN(&vp, filemode, u.u_cred);
	/*
	 * truncate if required
	 */
	if ((error == 0) && (filemode & FTRUNC)) {
		struct vattr vattr;

		filemode &= ~FTRUNC;
		vattr_null(&vattr);
		vattr.va_size = 0;
		error = VOP_SETATTR(vp, &vattr, u.u_cred);
	}
out:
	if (error) {
		VN_RELE(vp);
	} else {
		*vpp = vp;
	}
	return (error);
}

/*
 * create a vnode (makenode)
 */
int
vn_create(pnamep, seg, vap, excl, mode, vpp)
	char *pnamep;
	int seg;
	struct vattr *vap;
	enum vcexcl excl;
	int mode;
	struct vnode **vpp;
{
	struct vnode *dvp;	/* ptr to parent dir vnode */
	struct pathname pn;
	register int error;

	/*
	 * Lookup directory.
	 * If new object is a file, call lower level to create it.
	 * Note that it is up to the lower level to enforce exclusive
	 * creation, if the file is already there.
	 * This allows the lower level to do whatever
	 * locking or protocol that is needed to prevent races.
	 * If the new object is directory call lower level to make
	 * the new directory, with "." and "..".
	 */
	dvp = (struct vnode *)0;
	*vpp = (struct vnode *)0;
	error = pn_get(pnamep, seg, &pn);
	if (error)
		return (error);
	/*
	 * lookup will find the parent directory for the vnode.
	 * When it is done the pn hold the name of the entry
	 * in the directory.
	 * If this is a non-exclusive create we also find the node itself.
	 */
	if (excl == EXCL) {
		error = lookuppn(&pn, NO_FOLLOW, &dvp, (struct vnode **)0);
	} else {
		error = lookuppn(&pn, FOLLOW_LINK, &dvp, vpp);
		/*
		 * EEXIST is returned for open(".", ...)
		 * Return EISDIR instead.
		 */
		if (error == EEXIST)
			error = EISDIR;
	}
	if (error) {
		pn_free(&pn);
		return (error);
	}
	/*
	 * Make sure not a socket.
	 */
	if (*vpp != (struct vnode *)0 && (*vpp)->v_type == VSOCK) {
		pn_free(&pn);
		VN_RELE(*vpp);
		return (EOPNOTSUPP);
	}
	/*
	 * Make sure filesystem is writeable.
	 */
	if ((*vpp != NULL && isrofile(*vpp)) ||
	    (*vpp == NULL && dvp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		if (*vpp) {
			VN_RELE(*vpp);
		}
		error = EROFS;
	} else if (excl == NONEXCL && *vpp != (struct vnode *)0) {
		/*
		 * We throw the vnode away to let VOP_CREATE
		 * truncate the file in a non-racy manner.
		 */
		VN_RELE(*vpp);
	}
	if (error == 0) {
		/*
		 * Call mkdir if directory or create if other.
		 */
		if (vap->va_type == VDIR) {
			error = VOP_MKDIR(dvp, pn.pn_path, vap, vpp, u.u_cred);
		} else {
			error = VOP_CREATE(
			    dvp, pn.pn_path, vap, excl, mode, vpp, u.u_cred);
		}
	}
	pn_free(&pn);
	VN_RELE(dvp);
	return (error);
}

/*
 * close a vnode
 */
int
vn_close(vp, flag, count)
register struct vnode *vp;
int flag;
int count;
{

	return (VOP_CLOSE(vp, flag, count, u.u_cred));
}

/*
 * Link.
 */
int
vn_link(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct pathname pn;
	register int error;
	struct vattr vattr;
	long fsid;

	fvp = tdvp = (struct vnode *)0;
	error = pn_get(to_p, seg, &pn);
	if (error)
		return (error);
	error = lookupname(from_p, seg, FOLLOW_LINK, (struct vnode **)0, &fvp);
	if (error)
		goto out;
	error = lookuppn(&pn, FOLLOW_LINK, &tdvp, (struct vnode **)0);
	if (error)
		goto out;
	/*
	 * Make sure both source vnode and target directory vnode are
	 * in the same vfs and that it is writeable.
	 * We check fsid's, not vfs pointers, so loopback fs works
	 */
	error = VOP_GETATTR(fvp, &vattr, u.u_cred);
	if (error)
		goto out;
	fsid = vattr.va_fsid;
	error = VOP_GETATTR(tdvp, &vattr, u.u_cred);
	if (error)
		goto out;
	if (fsid != vattr.va_fsid) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * do the link
	 */
	error = VOP_LINK(fvp, tdvp, pn.pn_path, u.u_cred);
out:
	pn_free(&pn);
	if (fvp)
		VN_RELE(fvp);
	if (tdvp)
		VN_RELE(tdvp);
	return (error);
}

/*
 * Rename.
 */
int
vn_rename(from_p, to_p, seg)
	char *from_p;
	char *to_p;
	int seg;
{
	struct vnode *fdvp;		/* from directory vnode ptr */
	struct vnode *fvp;		/* from vnode ptr */
	struct vnode *tdvp;		/* to directory vnode ptr */
	struct vnode *tvp;		/* to vnode ptr */
	struct pathname fpn;		/* from pathname */
	struct pathname tpn;		/* to pathname */
	register int error;
	struct vattr vattr;
	long fsid;
	long mode;

	fdvp = tdvp = fvp = tvp = (struct vnode *)0;
	/*
	 * get to and from pathnames
	 */
	error = pn_get(from_p, seg, &fpn);
	if (error)
		return (error);

	error = pn_get(to_p, seg, &tpn);
	if (error) {
		pn_free(&fpn);
		return (error);
	}

	/*
	 * lookup to and from directories
	 */
	error = lookuppn(&fpn, NO_FOLLOW, &fdvp, &fvp);
	if (error)
		goto out;
	/*
	 * make sure there is an entry
	 */
	if (fvp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}

	error = lookuppn(&tpn, NO_FOLLOW, &tdvp, &tvp);
	if (error)
		goto out;
	/*
	 * Make sure both the from vnode and the to directory are
	 * in the same vfs and that it is writeable.
	 * We check fsid's, not vfs pointers, so loopback fs works
	 */
	error = VOP_GETATTR(fvp, &vattr, u.u_cred);
	if (error)
		goto out;
	fsid = vattr.va_fsid;
	mode = vattr.va_mode;
	error = VOP_GETATTR(tdvp, &vattr, u.u_cred);
	if (error)
		goto out;
	if (fsid != vattr.va_fsid) {
		error = EXDEV;
		goto out;
	}
	if (tdvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * posix - rename(!dir, dir)  should return EISDIR
	 */
	if (!S_ISDIR(mode) && tvp) {
		error = VOP_GETATTR(tvp, &vattr, u.u_cred);
		if (error)
			goto out;
		if (S_ISDIR(vattr.va_mode)) {
			error = EISDIR;
			goto out;
		}
	}

	/*
	 * do the rename
	 */
	error = VOP_RENAME(fdvp, fpn.pn_path, tdvp, tpn.pn_path, u.u_cred);
out:
	pn_free(&fpn);
	pn_free(&tpn);
	if (fvp)
		VN_RELE(fvp);
	if (fdvp)
		VN_RELE(fdvp);
	if (tvp)
		VN_RELE(tvp);
	if (tdvp)
		VN_RELE(tdvp);
	/*
	 * EEXIST is returned for link(from, ".")
	 * Return EINVAL instead.
	 */
	return (error == EEXIST ? EINVAL : error);
}

/*
 * remove a file or directory.
 */
int
vn_remove(fnamep, seg, dirflag)
	char *fnamep;
	int seg;
	enum rm dirflag;
{
	struct vnode *vp;		/* entry vnode */
	struct vnode *dvp;		/* ptr to parent dir vnode */
	struct pathname pn;		/* name of entry */
	enum vtype vtype;
	register int error;

	error = pn_get(fnamep, seg, &pn);
	if (error)
		return (error);
	vp = (struct vnode *)0;
	error = lookuppn(&pn, NO_FOLLOW, &dvp, &vp);
	if (error) {
		pn_free(&pn);
		/*
		 * EEXIST is returned for unlink(".")
		 * Return EINVAL instead.
		 */
		return (error == EEXIST ? EINVAL : error);
	}
	/*
	 * make sure there is an entry
	 */
	if (vp == (struct vnode *)0) {
		error = ENOENT;
		goto out;
	}
	/*
	 * make sure filesystem is writeable
	 */
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto out;
	}
	/*
	 * don't unlink the root of a mounted filesystem.
	 */
	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}
	/*
	 * release vnode before removing
	 */
	vtype = vp->v_type;
	VN_RELE(vp);
	vp = (struct vnode *)0;
	if (vtype == VDIR) {
		/*
		 * if caller thought it was removing a directory, go ahead
		 */
		if (dirflag == DIRECTORY)
			error = VOP_RMDIR(dvp, pn.pn_path, u.u_cred);
		else if (suser())
			error = VOP_REMOVE(dvp, pn.pn_path, u.u_cred);
		else
			error = EPERM;
	} else {
		/*
		 * if caller thought it was removing a directory, barf.
		 */
		if (dirflag == FILE)
			error = VOP_REMOVE(dvp, pn.pn_path, u.u_cred);
		else
			error = ENOTDIR;
	}
out:
	pn_free(&pn);
	if (vp != (struct vnode *)0)
		VN_RELE(vp);
	VN_RELE(dvp);
	return (error);
}

/*
 * Set vattr structure to a null value.
 * Boy is this machine dependent!
 */
void
vattr_null(vap)
struct vattr *vap;
{
	register int n;
	register char *cp;

	n = sizeof (struct vattr);
	cp = (char *)vap;
	while (n--) {
		*cp++ = -1;
	}
}

/*
 * Determine if this vnode is a file that is read-only
 */
isrofile(vp)
	struct vnode *vp;
{
	return (vp->v_type != VCHR &&
		vp->v_type != VBLK &&
		vp->v_type != VFIFO &&
		(vp->v_vfsp->vfs_flag & VFS_RDONLY));
}


#ifdef DEBUG
prvnode(vp)
	register struct vnode *vp;
{

	printf("vnode vp=0x%x ", vp);
	printf("flag=0x%x, count=%d, shlcnt=%d, exclcnt=%d\n",
		vp->v_flag, vp->v_count, vp->v_shlockc, vp->v_exlockc);
	printf("	vfsmnt=0x%x, vfsp=0x%x, type=%d, dev=0x%x\n",
		vp->v_vfsmountedhere, vp->v_vfsp, vp->v_type, vp->v_rdev);
}

prvattr(vap)
	register struct vattr *vap;
{

	printf("vattr: vap=0x%x ", vap);
	printf("type=%d, mode=0%o, uid=%d, gid=%d\n",
		vap->va_type, vap->va_mode, vap->va_uid, vap->va_gid);
	printf("fsid=%d, nodeid=%d, nlink=%d, size=%d, bsize=%d\n",
		vap->va_fsid, vap->va_nodeid, vap->va_nlink,
		vap->va_size, vap->va_blocksize);
	printf("atime=(%d,%d), mtime=(%d,%d), ctime=(%d,%d)\n",
		vap->va_atime.tv_sec, vap->va_atime.tv_usec,
		vap->va_mtime.tv_sec, vap->va_mtime.tv_usec,
		vap->va_ctime.tv_sec, vap->va_ctime.tv_usec);
	printf("rdev=0x%x, blocks=%d\n", vap->va_rdev, vap->va_blocks);
}
#endif
