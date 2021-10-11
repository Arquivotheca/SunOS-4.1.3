#ifndef lint
/*  @(#)tmp_vnodeops.c 1.1 92/07/30 SMI */
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/vfs_stat.h>
#include <sys/ucred.h>
#include <sys/dirent.h>
#include <sys/pathname.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpnode.h>
#include <tmpfs/tmpdir.h>
#include <sys/mman.h>
#include <vm/hat.h>
#include <vm/seg_vn.h>
#include <vm/seg_map.h>
#include <vm/seg.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/syslog.h>
#include <sys/debug.h>

#define	ISVDEV(t) ((t == VCHR) || (t == VBLK) || (t == VFIFO))

static  int tmp_open();
static  int tmp_close();
static  int tmp_rdwr();
static  int tmp_ioctl();
static  int tmp_select();
static  int tmp_getattr();
static  int tmp_setattr();
static  int tmp_access();
static  int tmp_lookup();
static  int tmp_create();
static  int tmp_remove();
static  int tmp_link();
static  int tmp_rename();
static  int tmp_mkdir();
static  int tmp_rmdir();
static  int tmp_readdir();
static  int tmp_symlink();
static  int tmp_readlink();
static  int tmp_fsync();
static  int tmp_inactive();
static  int tmp_lockctl();
static  int tmp_fid();
static  int tmp_getpage();
static  int tmp_putpage();
static  int tmp_map();
static  int tmp_dump();
static  int tmp_cmp();
static  int tmp_realvp();
static  int tmp_cntl();

struct vnodeops tmp_vnodeops = {
	tmp_open,
	tmp_close,
	tmp_rdwr,
	tmp_ioctl,
	tmp_select,
	tmp_getattr,
	tmp_setattr,
	tmp_access,
	tmp_lookup,
	tmp_create,
	tmp_remove,
	tmp_link,
	tmp_rename,
	tmp_mkdir,
	tmp_rmdir,
	tmp_readdir,
	tmp_symlink,
	tmp_readlink,
	tmp_fsync,
	tmp_inactive,
	tmp_lockctl,
	tmp_fid,
	tmp_getpage,
	tmp_putpage,
	tmp_map,
	tmp_dump,
	tmp_cmp,
	tmp_realvp,
	tmp_cntl,
};

#ifdef TMPFSDEBUG
int tmpfsdebug = 0;
int tmpdebugerrs = 0;
int tmplockdebug = 0;
int tmpdirdebug = 0;
int tmprwdebug = 0;
int tmpdebugalloc = 0;
#endif TMPFSDEBUG

extern struct timeval time;

/*
 * Table to convert vnode types to tmpnode formats
 */
int vttotf_tab[] = {
	0, TFREG, TFDIR, TFBLK, TFCHR, TFLNK, TFSOCK, TFMT, TFIFO
};

/*ARGSUSED*/
static int
tmp_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	VFS_RECORD((*vpp)->v_vfsp, VS_OPEN, VS_CALL);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_open: vpp %x\n", *vpp);
#endif TMPFSDEBUG
	/*
	 * swapon to tmpfs files are not supported
	 * so access is denied on open
	 */
	if ((*vpp)->v_flag & VISSWAP)
		return (EINVAL);
	return (0);
}

/*ARGSUSED*/
static int
tmp_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag, count;
	struct ucred *cred;
{
	VFS_RECORD(vp->v_vfsp, VS_CLOSE, VS_CALL);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_close: vp %x\n", vp);
#endif TMPFSDEBUG
	return (0);
}

/*ARGSUSED*/
static int
tmp_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);
	struct tmount *tm = VP_TO_TM(vp);
	int error;

	if (tp->tn_attr.va_type != VREG) {
		return (EISDIR);
	}
	if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
		/*
		 * In append mode start at end of file.
		 */
		uiop->uio_offset = tp->tn_attr.va_size;
	}
	error = rwtmp(tm, tp, uiop, rw);
	if (rw == UIO_WRITE)
		modified(tp);
	else
		accessed(tp);
	return (error);
}

/*ARGSUSED*/
static int
rwtmp(tm, tp, uiop, rw)
	struct tmount *tm;
	struct tmpnode *tp;
	struct uio *uiop;
	enum uio_rw rw;
{
	extern void swap_xlate();
	register char *base = NULL;	/* base of segmap */
	register u_int offset;		/* offset in tmpfs file (uio_offset) */
	register u_int pageoffset;	/* byte offset from page boundary */
	register u_int segmap_offset;	/* pagesize byte offset into segmap */
	register u_int newpageno;
	register u_int oldpageno = -1;
	register int bytes;		/* bytes to uiomove */
	register int nmoved;		/* bytes actually moved */
	int pagecreate = 0;		/* == 1 if we allocated a page */
	int error = 0;
	enum vtype type = tp->tn_attr.va_type;
	struct vnode *swapvp;
	u_int swapoff;
	int adjust_resid = 0;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmprwdebug)
		printf("rwtmp: tp %x offset %d resid %d rw %d\n", tp,
		    uiop->uio_offset, uiop->uio_resid, rw);
#endif TMPFSDEBUG

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwtmp");
	if (type != VREG && type != VDIR && type != VLNK)
		panic("rwtmp type");

	if (uiop->uio_offset < 0 || (uiop->uio_offset + uiop->uio_resid) < 0)
		return (EINVAL);

	if (rw == UIO_WRITE) {
		if (type == VREG && uiop->uio_offset + uiop->uio_resid >
		    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
			if (uiop->uio_offset >=
			    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
				psignal(u.u_procp, SIGXFSZ);
				error = EFBIG;
				goto out;
			} else {
				adjust_resid = uiop->uio_resid;
				uiop->uio_resid =
				    u.u_rlimit[RLIMIT_FSIZE].rlim_cur -
				    uiop->uio_offset;
				adjust_resid -= uiop->uio_resid;
			}
		}
	}

	tmpnode_lock(tp);
	while (uiop->uio_resid > 0 && !error) {
		offset = uiop->uio_offset;
		pageoffset = offset & PAGEOFFSET;
		bytes = MIN(PAGESIZE - pageoffset, uiop->uio_resid);
		newpageno = btop(offset);

		/*
		 * if we have a segmap and on new page in file,
		 * release segmap
		 */
		if (base != NULL && newpageno != oldpageno) {
			(void) segmap_release(segkmap, (addr_t)base, 0);
			base = NULL;
		}
		if (rw == UIO_READ) {
			int diff = tp->tn_attr.va_size - offset;

			VFS_RECORD(tm->tm_vfsp, VS_READ, VS_CALL);
			if (diff <= 0) {
				error = 0;
				break;
			}
			if (diff < bytes)
				bytes = diff;
		} else {
			VFS_RECORD(tm->tm_vfsp, VS_WRITE, VS_CALL);
			modified(tp);
		}

		/*
		 * For tmpfs, the underlying filesystem blocks are actually
		 * anonymous pages whose vp & offset correspond to the swap
		 * device rather than the tmpfs file. Addresses and offsets
		 * regarding the uiomove are set relative to swapvp rather
		 * than what is in the uio structure.
		 */
		if (base == NULL) {
			/*
			 * if we need a segmap page, first allocate the
			 * anon slot for the tmpnode
			 */
			if (rw == UIO_WRITE ||
			    tp->tn_amapp->anon[newpageno] == NULL) {
				pagecreate = tmpnode_findpage(tm, tp, offset);
				if (pagecreate < 0) {
					log(LOG_ERR,
"%s: file system full, anon allocation exceeded\n",
					    tm->tm_mntpath);
					uprintf(
"\n%s: write failed, file system is full\n",
					    tm->tm_mntpath);
					error = ENOSPC;
					break;
				}
			}
			swap_xlate(tp->tn_amapp->anon[newpageno],
			    &swapvp, &swapoff);
			/*
			 * The base for the uiomove is set relative to
			 * offset on the swap device.
			 */
			base = segmap_getmap(segkmap, swapvp,
			    (swapoff & MAXBMASK));
			segmap_offset = swapoff & MAXBOFFSET;
		}

		if (rw == UIO_WRITE) {
			int delta = offset + bytes - tp->tn_attr.va_size;
			/*
			 * Extend the file length.  Just update the size
			 * in tmpnode since anon slot is already allocated
			 */
			if (delta > 0) {
				if (tmp_resv(tm, tp, (u_int)delta,
				    pagecreate)) {
					log(LOG_ERR,
"%s: file system full, anon reservation exceeded\n",
					    tm->tm_mntpath);
					uprintf(
"\n%s: write failed, file system is full\n",
					    tm->tm_mntpath);
					error = ENOSPC;
					break;
				}
#ifdef TMPFSDEBUG
				if (tmprwdebug)
					printf("rwtmp: extend file %d bytes\n",
					    delta);

#endif TMPFSDEBUG
				tp->tn_attr.va_size += delta;
			}
		}

#ifdef TMPFSDEBUG
		if (tmprwdebug)
			printf("rwtmp: %s %x %d\n",
			    (rw == UIO_READ ? "reading" : "writing"),
			    base + pageoffset + segmap_offset, bytes);
#endif TMPFSDEBUG
		if (pagecreate > 0) {
			segmap_pagecreate(segkmap, base + segmap_offset,
			    (u_int)bytes, 0);
			/*
			 * if the file has a hole, we need to zero the
			 * page now rather than let a page of stuff get
			 * to the user.
			 */
			if (rw == UIO_READ)
				bzero(base + segmap_offset, (u_int) PAGESIZE);
		}
		error = uiomove(base + segmap_offset + pageoffset, bytes,
		    rw, uiop);
		nmoved = uiop->uio_offset - offset;
		ASSERT((nmoved + pageoffset) <= PAGESIZE);
#ifdef TMPFSDEBUG
		if (tmprwdebug) {
			printf("rwtmp: uio_offset %x actually moved %d\n",
			    uiop->uio_offset, nmoved);
		}
#endif TMPFSDEBUG

		/*
		 * if we wrote a partial page, its remainder must be zeroed
		 */
		if (rw == UIO_WRITE && pagecreate > 0 && !error) {
			int zoffset;	/* offset into page to zero from */

			/*
			 * Clear from the beginning of the page to the starting
			 * offset of the data.
			 */
			if (pageoffset > 0)
				kzero(base + segmap_offset, pageoffset);

			/*
			 * Zero from the end of data in the page to the
			 * end of the page.
			 */
			if ((zoffset = pageoffset + nmoved) < PAGESIZE)
				kzero(base + segmap_offset + zoffset, 
					(u_int) (PAGESIZE - zoffset));
		}
		if (error == 0 && rw == UIO_WRITE) {
			if (u.u_ruid != 0 && (tp->tn_attr.va_mode & (TEXEC |
			    (TEXEC >> 3) | (TEXEC >> 6))) != 0)
				tp->tn_attr.va_mode &= ~(S_ISUID|S_ISGID);
		}
		oldpageno = newpageno;
	}
	/*
	 * XXX if there was an error in the uiomove, we'll need to
	 * delete appropriate anon slots allocated for the tmpnode
	 */
	if (base != NULL)
		(void) segmap_release(segkmap, (addr_t)base, 0);
	tmpnode_unlock(tp);
out:
	if (!error && adjust_resid) {
		uiop->uio_resid = adjust_resid;
		psignal(u.u_procp, SIGXFSZ);
	}
	return (error);
}

/*ARGSUSED*/
static int
tmp_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	VFS_RECORD(vp->v_vfsp, VS_IOCTL, VS_CALL);
	return (EINVAL);
}

/*ARGSUSED*/
static int
tmp_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	VFS_RECORD(vp->v_vfsp, VS_SELECT, VS_CALL);
	return (EINVAL);
}

/*ARGSUSED*/
static int
tmp_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);

	VFS_RECORD(vp->v_vfsp, VS_GETATTR, VS_CALL);

	*vap = tp->tn_attr;
	vap->va_blocks = btodb(ptob(btopr(vap->va_size)));
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_getattr: tp %x vap:%x\n", tp, vap);
#endif TMPFSDEBUG
	return (0);
}

#define	OWNER(cred, va) \
	((((cred)->cr_uid == 0) || ((va)->va_uid == (cred)->cr_uid))? 0: EPERM)

static int
tmp_setattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	struct tmount *tm = VP_TO_TM(vp);
	struct tmpnode *tp = VP_TO_TN(vp);
	int error = 0;
	struct vattr *get = &tp->tn_attr;

	VFS_RECORD(vp->v_vfsp, VS_SETATTR, VS_CALL);
#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_setattr: tp %x vap %x\n", tp, vap);
#endif TMPFSDEBUG

	/*
	 * Cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}
	tmpnode_lock(tp);
	if (vap->va_mode != (u_short)-1) {
		if (error = OWNER(cred, get)) {
			goto out;
		}
		get->va_mode &= S_IFMT;
		get->va_mode |= vap->va_mode & ~S_IFMT;
		if (cred->cr_uid != 0) {
			if ((get->va_mode & S_IFMT) != S_IFDIR)
				get->va_mode &= ~S_ISVTX;
			if (!groupmember((int)get->va_gid))
				get->va_mode &= ~S_ISGID;
		}
	}
	if (vap->va_uid != (uid_t)-1) {
		if (cred->cr_uid == 0 || (cred->cr_uid == get->va_uid &&
		    get->va_uid == vap->va_uid)) {
			get->va_uid = vap->va_uid;
			if (cred->cr_uid != 0)
				get->va_mode &= ~(S_ISUID|S_ISGID);
		} else {
			error = EPERM;
			goto out;
		}
	}
	if (vap->va_gid != (gid_t)-1) {
		if (cred->cr_uid == 0 || ingroup(vap->va_gid, cred)) {
			get->va_gid = vap->va_gid;
			if (cred->cr_uid != 0)
				get->va_mode &= ~(S_ISUID|S_ISGID);
		} else {
			error =  EPERM;
			goto out;
		}
	}
	if (vap->va_atime.tv_sec != -1) {
		if (error = OWNER(cred, get)) {
			goto out;
		}
		get->va_atime = vap->va_atime;
		GET_TIME(&get->va_ctime);
	}
	if (vap->va_mtime.tv_sec != -1) {
		if (error = OWNER(cred, get)) {
			goto out;
		}
		/*
		 * Allow SystemV compatible option to set access and
		 * modified times to the current time.
		 *
		 * XXX - va_mtime.tv_usec == -1 flags this.
		 */
		if (vap->va_mtime.tv_usec == -1) {
			GET_TIME(&get->va_atime);
			GET_TIME(&get->va_mtime);
		} else {
			get->va_mtime = vap->va_mtime;
		}
		GET_TIME(&get->va_ctime);
	}
	if (vap->va_size != (u_long)-1) {
		if (tp->tn_attr.va_type == VDIR) {
			error =  EISDIR;
			goto out;
		}
		/* write access was checked above the vnode layer */
		if (error = tmpnode_trunc(tm, tp, vap->va_size)) {
			goto out;
		}
	}
out:
	tmpnode_unlock(tp);
	return (error);
}

static int
tmp_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);

	VFS_RECORD(vp->v_vfsp, VS_ACCESS, VS_CALL);

	return (taccess(tp, cred, mode));
}

/*ARGSUSED*/
static int
tmp_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	struct tmpnode *tp = VP_TO_TN(dvp);
	struct tmpnode *ntp;
	int error;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_lookup: tp %x nm %s\n", tp, nm);
#endif TMPFSDEBUG

	VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_CALL);

	error = tdirlookup(tp, nm, &ntp, cred);
	if (error == 0) {
		*vpp = TP_TO_VP(ntp);
		accessed(tp);
		tmpnode_unlock(ntp);
		/*
		 * If vnode is a device return special vnode instead
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct vnode *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type);
			VN_RELE(*vpp);
			*vpp = newvp;
		}
	}
	return (error);
}

/*ARGSUSED*/
static int
tmp_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct tmpnode *parent = VP_TO_TN(dvp);
	struct tmount *tm = VP_TO_TM(dvp);
	struct tmpnode *self;
	int error;
	struct tmpnode *oldtp;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_create: parent %x nm %s\n", parent, nm);
#endif TMPFSDEBUG
	VFS_RECORD(dvp->v_vfsp, VS_CREATE, VS_CALL);

	switch ((int) vap->va_type) {
		/* only super-user can create non-FIFO special devices */
		case (int) VBLK:
		case (int) VCHR:
			if (cred->cr_uid != 0)
				return (EPERM);
			else
				break;

		/* tmp_mkdir is used to create directories */
		case (int) VDIR:
			return (EISDIR);
	}

	error = tdirlookup(parent, nm, &oldtp, cred);
	if (error == 0) {	/* name found */
		if (exclusive == EXCL) {
			tmpnode_put(oldtp);
			return (EEXIST);
		}
		if (oldtp->tn_attr.va_type == VDIR) {
			tmpnode_put(oldtp);
			return (EISDIR);
		}
		if (error = taccess(oldtp, cred, mode)) {
			tmpnode_put(oldtp);
			return (error);
		}
		if (vap->va_size == 0) {
			(void) tmpnode_trunc(tm, oldtp, (u_long)0);
		}
		*vpp = TP_TO_VP(oldtp);
		if (vap != NULL)
			*vap = oldtp->tn_attr;
		tmpnode_unlock(oldtp);
		/*
		 * If vnode is a device return special vnode instead
		 */
		if (ISVDEV((*vpp)->v_type)) {
			struct vnode *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type);
			VN_RELE(*vpp);
			*vpp = newvp;
		}
		return (0);
	}
	if (error != ENOENT) {
		return (error);
	}
	self = tmpnode_alloc(tm, vap->va_type);
	if (self == NULL) {
		log(LOG_ERR, "%s: file system full, kmem_alloc failure\n",
		    tm->tm_mntpath);
		uprintf("\n%s: create failed, out of kernel memory\n",
		    tm->tm_mntpath);
		return (ENOSPC);
	}
#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_create: newnode type %x mode %x\n", vap->va_type,
			vttotf_tab[(int)vap->va_type] | (vap->va_mode & 0777));
#endif TMPFSDEBUG
	newnode(self, vttotf_tab[(int)vap->va_type] | (vap->va_mode & 0777),
	    cred->cr_uid, cred->cr_gid);
	error = tdirenter(tm, parent, nm, self, cred);
	if (error) {
		tmpnode_free(tm, self);
		return (error);
	}
	tmpnode_unlock(self);
	modified(parent);
	*vpp = TP_TO_VP(self);
	if (ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		if ((*vpp)->v_type == VBLK || (*vpp)->v_type == VCHR) {
			self->tn_attr.va_rdev = (*vpp)->v_rdev =
			    vap->va_rdev;
		}
		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type);
		VN_RELE(*vpp);
		*vpp = newvp;
	}

	if (vap != NULL)
		*vap = self->tn_attr;
	return (0);
}

static int
tmp_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	struct tmpnode *parent = VP_TO_TN(dvp);
	struct tmount *tm = VP_TO_TM(dvp);
	int error;
	struct tmpnode *tp;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_remove: parent %x nm %s\n", parent, nm);
#endif TMPFSDEBUG

	VFS_RECORD(dvp->v_vfsp, VS_REMOVE, VS_CALL);

	error = tdirlookup(parent, nm, &tp, cred);
	if (error) {
		return (error);
	}
	if (tp->tn_attr.va_type == VDIR) {
		tmpnode_put(tp);
		return (EISDIR);
	}
	error = tdirdelete(tm, parent, tp, nm, cred);
	if (error) {
		tmpnode_put(tp);
		return (error);
	}
	tmpnode_put(tp);
	modified(parent);
	return (0);
}

static int
tmp_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	struct tmpnode *from = VP_TO_TN(vp);
	struct tmpnode *parent = VP_TO_TN(tdvp);
	struct tmount *tm = VP_TO_TM(vp);
	int error;
	struct tmpnode *found;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_link: from %x parent %x tnm %s\n", from,
		    parent, tnm);
#endif TMPFSDEBUG
	VFS_RECORD(vp->v_vfsp, VS_LINK, VS_CALL);
	if (from->tn_attr.va_type == VDIR && cred->cr_uid != 0)
		return (EPERM);
	error = tdirlookup(parent, tnm, &found, cred);
	if (error == 0) {
		tmpnode_put(found);
		return (EEXIST);
	}
	if (error != ENOENT)
		return (error);
	error = tdirenter(tm, parent, tnm, from, cred);
	if (error) {
		return (error);
	}
	modified(parent);
	return (0);
}

static int
tmp_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	struct tmpnode *fromparent = VP_TO_TN(odvp);
	struct tmpnode *toparent = VP_TO_TN(ndvp);
	struct tmpnode *fromtp = NULL;
	struct tmpnode *totp = NULL;
	struct tmpnode *tmptp = NULL;
	struct tmount *tm = VP_TO_TM(odvp);
	int error, doingdirectory;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_rename: fromparent %x onm %s toparent %x nnm %s\n",
		    fromparent, onm, toparent, nnm);
#endif TMPFSDEBUG
	VFS_RECORD(odvp->v_vfsp, VS_RENAME, VS_CALL);

	/*
	 * Make sure we can delete the old (source) entry.
	 */
	error = taccess(fromparent, cred, VWRITE);
	if (error) {
		return (error);
	}

	/*
	 * Check for renaming '.' or '..'
	 */
	if ((strcmp(onm, ".") == 0) || (strcmp(onm, "..") == 0))
		return (EINVAL);

	/*
	 * Lookup source
	 */
	error = tdirlookup(fromparent, onm, &fromtp, cred);
	if (error) {
		return (error);
	}

	if ((fromparent->tn_attr.va_mode & TSVTX) && cred->cr_uid != 0 &&
	    cred->cr_uid != fromparent->tn_attr.va_uid &&
	    fromtp->tn_attr.va_uid != cred->cr_uid) {
		tmpnode_put(fromtp);
		return (EPERM);
	}

	/*
	 * Check that we don't move a parent directory down to one of its
	 * children, or we'd end up removing this subtree from the directory
	 * hierarchy.
	 */
	if (isparent(fromtp, toparent)) {
		tmpnode_put(fromtp);
		return (EINVAL);
	}

	doingdirectory = (fromtp->tn_attr.va_type == VDIR);

	/*
	 * Check out the target
	 */
	error = tdirlookup(toparent, nnm, &totp, cred);
	if (error == 0) {	/* entry of name already exists */
		if (totp->tn_attr.va_type == VDIR) {
			if (!doingdirectory)
				error = ENOTDIR;
			if (totp->tn_attr.va_size > TEMPTYDIRSIZE ||
			    totp->tn_attr.va_nlink > 2)
				error = ENOTEMPTY;
			goto done;
		} else if (doingdirectory) {
			error = ENOTDIR;
			goto done;
		}

		if (totp == fromtp) {	/* same file */
			goto done;
		}
		/*
		 * Unlink old target
		 * XXX truncate and remove if nlink < 2 ??
		 */
		error = tdirdelete(tm, toparent, totp, nnm, cred);
		if (error) {
			goto done;
		}
	} else if (error != ENOENT) {
		tmpnode_put(fromtp);
		return (error);
	}

	/*
	 * Link source to new target
	 */
	error = tdirenter(tm, toparent, nnm, fromtp, cred);
	if (error) {
		/*
		 * Re-enter old target into directory if necessary
		 */
		if (totp)
			error = tdirenter(tm, toparent, nnm, totp, cred);
		goto done;
	}

	/*
	 * if renaming a directory moves it into a different parent directory,
	 * rename the .. entry to point at new parent
	 * always account for link to parent if renaming directories
	 */
	if (doingdirectory) {
		if (fromparent != toparent) {
			if (tdirlookup(fromtp, "..", &tmptp, cred) == 0) {
				(void) tdirdelete(tm, fromtp, tmptp,
				    "..", cred);
				tmpnode_put(tmptp);
				/*
				 * tdirdelete decrements the count on the
				 * tmpnode we remove (i.e. parent of fromtp) and
				 * the directory we remove it from (i.e. fromtp)
				 * We need to bump up the link count on these
				 * because they'll be decremented again below
				 * when the source is unlinked.
				 */
				fromtp->tn_attr.va_nlink++;
				tmptp->tn_attr.va_nlink++;
			}
			if (tdirenter(tm, fromtp, "..", toparent, cred) != 0)
				panic("tmp_rename");
		} else
			toparent->tn_attr.va_nlink++;
	}

	/*
	 * Unlink source
	 */
	error = tdirdelete(tm, fromparent, fromtp, onm, cred);
	if (error) {
	    goto done;
	}

	modified(fromparent);
	modified(toparent);
done:
	tmpnode_put(fromtp);
	if (totp)
		tmpnode_put(totp);
	return (error);
}

static int
tmp_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct tmpnode *parent = VP_TO_TN(dvp);
	struct tmpnode *self;
	struct tmount *tm = VP_TO_TM(dvp);
	int error;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_mkdir: parent %x nm %s\n", parent, nm);
#endif TMPFSDEBUG

	VFS_RECORD(dvp->v_vfsp, VS_MKDIR, VS_CALL);
	/*
	 * Make sure we have write permission on the parent directory
	 * before we allocate anything
	 */
	if (error = taccess(parent, cred, VWRITE)) {
		tdirclose(parent);
		return (error);
	}
	error = tdirlookup(parent, nm, &self, cred);
	if (error == 0) {
		tmpnode_put(self);
		return (EEXIST);
	}
	if (error != ENOENT) {
		return (error);
	}
	self = tmpnode_alloc(tm, VDIR);
	if (self == NULL) {
		return (ENOSPC);
	}
	newnode(self, S_IFDIR | (va->va_mode & 0777), cred->cr_uid,
	    cred->cr_gid);
	*va = self->tn_attr;
	if ((error = tdirenter(tm, self, ".", self, cred)) ||
	    (error = tdirenter(tm, self, "..", parent, cred)) ||
	    (error = tdirenter(tm, parent, nm, self, cred))) {
		tmpnode_free(tm, self);
		return (error);
	}
	tmpnode_unlock(self);
	modified(parent);
	*vpp = TP_TO_VP(self);
	return (0);
}

static int
tmp_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	struct tmpnode *parent = VP_TO_TN(dvp);
	struct tmpnode *self;
	struct tmount *tm = VP_TO_TM(dvp);
	int error = 0;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_rmdir: parent %x nm %s\n", parent, nm);
#endif TMPFSDEBUG

	VFS_RECORD(dvp->v_vfsp, VS_RMDIR, VS_CALL);

	/*
	 * return error when removing . and ..
	 */
	if (strcmp(nm, ".") == 0)
		return (EINVAL);
	if (strcmp(nm, "..") == 0)
		return (ENOTEMPTY);
	error = tdirlookup(parent, nm, &self, cred);
	if (error) {
		return (error);
	}
	if (self->tn_attr.va_type != VDIR) {
		tmpnode_put(self);
		return (ENOTDIR);
	}
	if ((self->tn_attr.va_nlink > 2) ||
	    (self->tn_attr.va_size > TEMPTYDIRSIZE)) {
		tmpnode_put(self);
		return (ENOTEMPTY);
	}
	error = tdirdelete(tm, parent, self, nm, cred);
	if (error) {
		tmpnode_put(self);
	} else {
		self->tn_attr.va_nlink--;
		(void) tmpnode_trunc(tm, self, (u_long)0);
		tmpnode_put(self);
		modified(parent);
	}
	return (error);
}

static int
tmp_readdir(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);
	struct dirent d;
	struct tdirent *tdp;
	char *strcpy();
	int error, offset;

	VFS_RECORD(vp->v_vfsp, VS_READDIR, VS_CALL);

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_readdir: tp %x offset %d count %d\n", tp,
		    uiop->uio_offset, uiop->uio_iov->iov_len);
#endif TMPFSDEBUG
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	error = tdiropen(tp, cred);
	if (error) {
		return (error);
	}
	if (uiop->uio_offset >= tp->tn_attr.va_size)
		return (0);
	if ((tdp = tp->tn_dir) == NULL) {
		panic("empty directory");
	}

	for (offset = 0; tdp; tdp = tdp->td_next) {
		if (offset >= uiop->uio_offset) {
			d.d_namlen = tdp->td_namelen;
			d.d_reclen = DIRSIZ(&d);
			if (d.d_reclen > uiop->uio_iov->iov_len) {
				break;
			}
			d.d_off = offset+tdp->td_reclen;
			d.d_fileno = (u_long)tdp->td_tmpnode->tn_attr.va_nodeid;
			(void) strcpy(d.d_name, tdp->td_name);
			if (error = uiomove((char *)&d, (int)d.d_reclen,
			    UIO_READ, uiop))
				break;
		}
		offset += tdp->td_reclen;
	}
	accessed(tp);
	tdirclose(tp);
	return (error);
}

/*ARGSUSED*/
static int
tmp_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	struct tmpnode *parent = VP_TO_TN(dvp);
	struct tmpnode *self;
	struct tmount *tm = VP_TO_TM(dvp);
	char *cp, *strncpy();
	int error, len;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmp_symlink: parent %x lnm %s tnm %s\n", parent,
		    lnm, tnm);
#endif TMPFSDEBUG

	error = tdirlookup(parent, lnm, &self, cred);
	if (error == 0) {
		tmpnode_put(self);
		return (EEXIST);
	}
	if (error != ENOENT) {
		return (error);
	}
	self = tmpnode_alloc(tm, VLNK);
	if (self == NULL) {
		return (ENOSPC);
	}
	newnode(self, S_IFLNK | 0777, cred->cr_uid, cred->cr_gid);
	len = strlen(tnm);
	if ((cp = tmp_memalloc(tm, (u_int)len)) == NULL) {
		tmpnode_free(tm, self);
		return (ENOSPC);
	}
	(void) strncpy(cp, tnm, len);
	self->tn_symlink = cp;
	self->tn_attr.va_size = len;
	error = tdirenter(tm, parent, lnm, self, cred);
	if (error) {
		tmpnode_free(tm, self);
		tmp_memfree(tm, cp, (u_int)len);
		return (error);
	}
	tmpnode_put(self);
	modified(parent);
	return (0);
}

/*ARGSUSED*/
static int
tmp_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);
	int error;

	VFS_RECORD(vp->v_vfsp, VS_READLINK, VS_CALL);

	if (tp->tn_attr.va_type != VLNK) {
		return (EINVAL);
	}
	error = uiomove(tp->tn_symlink, (int)tp->tn_attr.va_size,
	    UIO_READ, uiop);
	accessed(tp);
	if (error) {
		return (error);
	}
	return (0);
}

/*ARGSUSED*/
static int
tmp_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	VFS_RECORD(vp->v_vfsp, VS_FSYNC, VS_CALL);
	return (0);
}

/*ARGSUSED*/
static int
tmp_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct tmpnode *tp = VP_TO_TN(vp);
	struct tmount *tm = VFSP_TO_TM(vp->v_vfsp);

	VFS_RECORD(vp->v_vfsp, VS_INACTIVE, VS_CALL);

	tmpnode_inactive(tm, tp);
	return (0);
}

static int
tmp_lockctl()
{
	return (EINVAL);
}

/*
 * nfs mounts of tmpfs file systems are not supported
 */
/*ARGSUSED*/
static int
tmp_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
tmp_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	return (0);
}

/*ARGSUSED*/
static int
tmp_putpage(vp, off, len, flags, cred)
	struct vnode *vp;
	u_int off, len;
	int flags;
	struct ucred *cred;
{
	return (0);
}

/*ARGSUSED*/
static int
tmp_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot;
	u_int maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segvn_crargs vn_a;
	struct tmpnode *tp = VP_TO_TN(vp);

	VFS_RECORD(vp->v_vfsp, VS_MAP, VS_CALL);

#ifdef TMPFSDEBUG
	if (tmpfsdebug)
		printf("tmp_map: tp %x off %x len %x flags %x as %x\n",
		    tp, off, len, flags, as);
#endif TMPFSDEBUG
	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_type != VREG)
		return (ENODEV);

	if (off + len > roundup(tp->tn_attr.va_size, PAGESIZE))
		return (EINVAL);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 1);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}

	vn_a.vp = NULL;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cred;
	vn_a.amp = tp->tn_amapp;
	return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
}

static int
tmp_dump()
{
	return (EINVAL);
}

static int
tmp_cmp(vp1, vp2)
	struct vnode *vp1;
	struct vnode *vp2;
{
	return (vp1 == vp2);
}

/*ARGSUSED*/
static int
tmp_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
tmp_cntl(vp, cmd, idata, odata, iflg, oflg)
	struct vnode *vp;
	caddr_t idata, odata;
{
	return (ENOSYS);
}
