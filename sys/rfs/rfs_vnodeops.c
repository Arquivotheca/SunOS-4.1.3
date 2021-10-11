#ident	"@(#)rfs_vnodeops.c 1.1 92/07/30 SMI"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/dirent.h>
#include <sys/pathname.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <sys/stream.h>
#include <sys/fcntl.h>
#include <sys/mount.h>
#include <rfs/rfs_misc.h>
#include <rfs/nserve.h>
#include <rfs/rfs_node.h>
#include <rfs/rfs_mnt.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <sys/debug.h>
#include <rfs/rdebug.h>
#include <rfs/cirmgr.h>
#include <rfs/hetero.h>
#include <rfs/message.h>


static int rf_open();
static int rf_close();
static int rf_rdwr();
static int rf_ioctl();
static int rf_select();
static int rf_getattr();
static int rf_setattr();
static int rf_access();
static int rf_lookup();
static int rf_create();
static int rf_remove();
static int rf_link();
static int rf_rename();
static int rf_mkdir();
static int rf_rmdir();
static int rf_readdir();
static int rf_symlink();
static int rf_readlink();
static int rf_fsync();
static int rf_inactive();
static int rf_lockctl();
static int rf_fid();
static int rf_getpage();
static int rf_putpage();
static int rf_map();
static int rf_cmp();
static int rf_realvp();
static int rf_badop();
static int rf_cntl();


struct vnodeops rfs_vnodeops = {
	rf_open,
	rf_close,
	rf_rdwr,
	rf_ioctl,
	rf_select,
	rf_getattr,
	rf_setattr,
	rf_access,
	rf_lookup,
	rf_create,
	rf_remove,
	rf_link,
	rf_rename,
	rf_mkdir,
	rf_rmdir,
	rf_readdir,
	rf_symlink,
	rf_readlink,
	rf_fsync,
	rf_inactive,
	rf_lockctl,
	rf_fid,
	rf_getpage,
	rf_putpage,
	rf_map,
	rf_badop,
	rf_cmp,
	rf_realvp,
	rf_cntl,
};


static int
rf_open(vpp, flag, cred)
register struct vnode **vpp;
int flag;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(*vpp);
	sndd_t sdp = (sndd_t) NULL;
	struct nodeinfo ninfo;
	int error = 0;

	DUPRINT3(DB_VNODE, "rfopen: vp %x, flag %x\n", *vpp, flag);

	/*
	 * KLUDGE: don't do the open call if it was already opened by a
	 * VOP_CREAT().
	 */
	if (rfp->rfs_flags & RFSOPEN) {
		rfp->rfs_flags &= ~RFSOPEN;
		goto out;
	}
	/* Do the open */
	if (error = du_open(rfp->rfs_name, rfp->rfs_psdp, cred,
				(u_short) OMODETORFS(flag), 0, &sdp, &ninfo))
		goto out;

	/* Get client cache status on file */
	rfp->rfs_sdp->sd_stat &= ~SDCACHE_MASK;
	rfp->rfs_sdp->sd_stat |= sdp->sd_stat & SDCACHE_MASK;
	rfp->rfs_vcode = sdp->sd_vcode;

	/* Check the remote reference returned */
	(void) transform_rfsnode(vpp, sdp, &ninfo);

	/*
	 * Invalidate the cache if mandatory locking or write was done
	 * since close (version # check) or cache disabled
	 */
	/* rfcache_check(sd, vp) */
out:
	DUPRINT2(DB_VNODE, "rfopen: result %d\n", error);
	return (error);
}

static int
rf_close(vp, flag, count, cred)
struct vnode *vp;
int flag;
int count;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	int error;

	DUPRINT3(DB_VNODE, "rf_close: vp %x, flag %x\n", vp, flag);
	ASSERT(rfp->rfs_sdp);

	error = du_close(rfp->rfs_sdp, FFLAGTORFS(flag), count, cred);

	/* Get client cache status on file */
	rfp->rfs_vcode = rfp->rfs_sdp->sd_vcode;

	DUPRINT2(DB_VNODE, "rf_close: result %d\n", error);
	return (error);
}

/*
 * Read/write peculiarities:
 * ioflag options
 *	a) IO_UNIT -- nothing special is done for this, i.e., it's not
 *	   really supported.
 *	b) IO_SYNC -- S5R3 has a mode, FSYNC, which supports this. This
 *	can be passed with RFS read/write system call in the rq_mode field.
 */

static int
rf_rdwr(vp, uiop, rw, ioflag, cred)
struct vnode *vp;
struct uio *uiop;
enum uio_rw rw;
int ioflag;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	register sndd_t sdp = rfp->rfs_sdp;
	int fflag = 0;
	int error = 0;
	int rwerror = 0;

	DUPRINT6(DB_VNODE, "rfrdwr: vp %x, %s, base %x, len %d, ioflag %x\n",
		vp, rw == UIO_READ ? "READ" : "WRITE", uiop->uio_iov->iov_base,
		uiop->uio_iov->iov_len, ioflag);

	ASSERT(rfp->rfs_sdp);

	/* Translate the io options to RFSese */
	fflag = FFLAGTORFS(uiop->uio_fmode);
	if (ioflag & IO_SYNC)
		fflag |= RFS_FSYNC;
	if (ioflag & IO_APPEND)
		fflag |= RFS_FAPPEND;

	/* If the server has turned caching off, make sure cache is off */
	if ((sdp->sd_stat & SDMNDLCK) || !(sdp->sd_stat & SDCACHE))
	/* 	rfinval(sd, -1, 0) */;

	/* Do the read/write */
	if (rw == UIO_READ)
		rwerror = du_read(sdp, uiop, fflag, cred);
	else
		rwerror = du_write(sdp, uiop, fflag, cred);

	DUPRINT2(DB_VNODE, "rfrdwr: results %d\n", error);
	return (rwerror ? rwerror : error);
}

static int
rf_ioctl(vp, com, data, flag, cred)
struct vnode *vp;
int com;
caddr_t data;
int flag;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	int error;

	DUPRINT5(DB_VNODE, "rfioctl: vp %x, com %x, data %x, flag %x\n",
			vp, com, data, flag);

	if (!rfp->rfs_sdp) {
		error = EBADF;
		goto out;
	}

	/*
	 * Can only do ioctl between homogeneous machines, since you
	 * don't know what data elements are in the ioctl args
	 */
	if (GDP(rfp->rfs_sdp->sd_queue)->hetero != NO_CONV) {
		error = EOPNOTSUPP;
		goto out;
	}

	error = du_ioctl(rfp->rfs_sdp, com, data, flag, cred);
out:
	DUPRINT2(DB_VNODE, "rfioctl: result %d\n", error);
	return (error);
}

/*ARGSUSED*/
static int
rf_select(vp, which, cred)
struct vnode *vp;
int which;
struct ucred *cred;
{
	return (EOPNOTSUPP);
}

static int
rf_getattr(vp, vap, cred)
struct vnode *vp;
struct vattr *vap;
struct ucred *cred;
{
	register struct rfsnode *rfp = vtorfs(vp);
	struct rfsmnt *rm =  (struct rfsmnt *) vp->v_vfsp->vfs_data;
	struct rfs_stat sb;
	int error;

	DUPRINT2(DB_VNODE, "rf_getattr: vp %x\n", vp);

	/* Do FSTAT if true ref, else do STAT with name and parent ref */
	if (rfp->rfs_sdp)
		error = du_fstat(rfp->rfs_sdp, cred, &sb);
	else
		error = du_stat(rfp->rfs_name, rfp->rfs_psdp, cred, &sb);

	/* Put the results in the attribute structure */
	if (!error) {
		vap->va_type = RFSTOVT(sb.st_mode);
		vap->va_mode = sb.st_mode;  /* Same bits for S5 and sun */
		vap->va_uid = sb.st_uid;
		vap->va_gid = sb.st_gid;
		vap->va_fsid =
			0x0ffff & (long) makedev(rm->rm_mntno, 0x00ff & sb.st_dev);
		vap->va_nodeid = sb.st_ino;
		vap->va_nlink = sb.st_nlink;
		vap->va_size = (unsigned) sb.st_size;
		vap->va_rdev = sb.st_rdev;
		vap->va_atime.tv_sec = sb.st_atime;
		vap->va_atime.tv_usec = 0;
		vap->va_mtime.tv_sec = sb.st_mtime;
		vap->va_mtime.tv_usec = 0;
		vap->va_ctime.tv_sec = sb.st_ctime;
		vap->va_ctime.tv_usec = 0;
		/* These values derive from statfs done at mount time */
		vap->va_blocksize = rm->rm_bsize;
		vap->va_blocks = sb.st_size / rm->rm_bsize +
		    (sb.st_size % rm->rm_bsize != 0);
	}

	DUPRINT6(DB_VNODE, "rf_getattr: rslt %d, type %d, mode %o, nodeid %d, size %d\n", error, vap->va_type, vap->va_mode, vap->va_nodeid, vap->va_size);

	return (error);
}

/*
 * rf_setattr --
 *	1)Depending on how many attributes you want to set, this routine
 *	may require several RFS system calls. This means that in case
 *	of an error return some of the file attributes may be modified
 *	to the requested values and some may not be.  This is only a problem
 *	if you attempt to change multiple attrbutes at once, something not
 *	currently done by any usages of VOP_SETATTR in Sun's kernel.
 *	2)Note the restrictions on what can be set:
 *	a) va_size can only be set to 0.
 *	b) If setting va_atime or va_mtime both must be set.
 */


static int
rf_setattr(vp, vap, cred)
register struct vnode *vp;
register struct vattr *vap;
struct ucred *cred;
{

	struct rfsnode *rfp = vtorfs(vp);
	sndd_t gift = (sndd_t) NULL;
	struct nodeinfo ninfo;
	long atime, mtime;
	uid_t uid;
	gid_t gid;
	int size;
	int error;
	struct rfs_stat sb;

	DUPRINT6(DB_VNODE,
	    "rfsetattr: vp %x, sz %d, mode %x, uid %d, atim %d\n",
	    vp, vap->va_size, vap->va_mode, vap->va_uid, vap->va_atime.tv_sec);

	/* Set size (only support 0 size) */
	size = vap->va_size;
	if (size > 0) {
		error = EOPNOTSUPP;
		goto out;
	} else if (size == 0) {
		error = du_open(rfp->rfs_name, rfp->rfs_psdp, cred,
					(u_short) (RFS_O_WRONLY|RFS_O_TRUNC), 0,
					&gift, &ninfo);
		if (error) goto out;
		error = du_close(gift, 0, 1, cred);

		/* Get client cache status on file */
		rfp->rfs_sdp->sd_stat &= ~SDCACHE_MASK;
		rfp->rfs_sdp->sd_stat |= gift->sd_stat & SDCACHE_MASK;
		rfp->rfs_vcode = gift->sd_vcode;

		error = del_sndd(gift);	/* Free remote reference from open() */
		if (error) goto out;

		/* Invalidate the cache rfs_cache_check(vp, sdp); */
	}

	/* Set mode */
	if (vap->va_mode != (u_short) -1) {
		error = du_chmod(rfp->rfs_name, rfp->rfs_psdp, cred,
				vap->va_mode);
		if (error)
			goto out;
	}

	/* Set uid, gid */
	uid = vap->va_uid;  gid = vap->va_gid;
	if (uid != (uid_t) -1 || gid != (gid_t) -1) {
		/* Use current values if requestor doesn't want to set both */
		if (uid == (uid_t) -1 || gid == (gid_t) -1) {
			error = du_fstat(rfp->rfs_sdp, cred, &sb);
			if (error) goto out;
			if (uid == (uid_t) -1)
				uid = sb.st_uid;
			if (gid == (gid_t) -1)
				gid = sb.st_gid;
		}
		error = du_chown(rfp->rfs_name, rfp->rfs_psdp, cred,
					uid, gid);
		if (error) goto out;
	}

	/* Set atime, mtime (only support setting both) */
	atime = vap->va_atime.tv_sec;  mtime = vap->va_mtime.tv_sec;
	if ((atime != -1 && mtime == -1) || (atime == -1 && mtime != -1)) {
		if (!vap->va_mtime.tv_sec && vap->va_mtime.tv_usec == -1) {
			vap->va_atime.tv_sec = -1;
			vap->va_mtime.tv_sec = -1;
			error = du_utime(rfp->rfs_name, rfp->rfs_psdp, cred,
				&vap->va_atime, &vap->va_mtime);
				if (error) goto out;
		} else {
			error = EOPNOTSUPP;
			goto out;
		}
	} else if (atime != -1 && mtime != -1) {
		error = du_utime(rfp->rfs_name, rfp->rfs_psdp, cred,
			&vap->va_atime, &vap->va_mtime);
		if (error) goto out;
	}

out:
	DUPRINT2(DB_VNODE, "rfsetattr: result %d\n", error);
	return (error);
}

static int
rf_access(vp, mode, cred)
struct vnode *vp;
int mode;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	int error;

	DUPRINT3(DB_VNODE, "rf_access: vp %x, mode %x\n", vp, mode);

	error = du_access(rfp->rfs_name, rfp->rfs_psdp, cred,
				(u_short)VACCTORFS(mode));

	DUPRINT2(DB_VNODE, "rf_access: result %d\n", error);
	return (error);
}


/*
 * rf_lookup - lookup an RFS file and return a vnode for the file.
 * If multi-component lookup is specified on this filesystem, the
 * operation tries to eat as many components of name/pnp as it can.
 * In case of 1) backing over a server mount point, 2) the parent
 * directory vnode is desired, or 3) real error, it eats less and
 * returns the unused path.
 */
/*ARGSUSED*/
static int
rf_lookup(dvp, name, vpp, cred, pnp, flags)
struct vnode *dvp;
char *name;
struct vnode **vpp;
struct ucred *cred;
struct pathname *pnp;
int flags;
{

	register struct rfsnode *drfp;
	sndd_t psdp;
	sndd_t sdp = (sndd_t) NULL; 	/* Returned sd for name */
	struct nodeinfo ninfo;
	extern int rfs_magic_cred;
	struct ucred lcred;
	int error = 0;
	char *lastcomp = NULL;
	struct pathname pn;
	static caddr_t kmp;
	int multi = 0;


	DUPRINT3(DB_VNODE, "rf_lookup: dvp %x, name %s\n", dvp, name);
	drfp = vtorfs(dvp);
	psdp = drfp->rfs_sdp;
	*vpp = (struct vnode *) NULL;
	(void) pn_alloc(&pn);
	(void) pn_set(&pn, name);

	/*
	 * If specified for this filesystem, do multi-component lookup
	 * XXX u_rdir test is a hack to prevent backing over a chroot()
	 * in this filesystem
	 */
	multi = ((dvp->v_vfsp->vfs_flag & VFS_MULTI) &&
		pnp && pn_pathleft(pnp) &&
		(!u.u_rdir || (u.u_rdir->v_vfsp != dvp->v_vfsp)));
	if (multi) {
		pnp->pn_path[pnp->pn_pathlen] = '\0';
		error = pn_append(&pn, pn_getpath(pnp));   /* Remake path */
		ASSERT(!error);
		/* If parent wanted, remove last component */
		if (flags & LOOKUP_DIR) {
			lastcomp = new_kmem_fast_alloc(
					&kmp, MAXNAMLEN, 1, KMEM_SLEEP);
			error = pn_getlast(&pn, lastcomp);
			ASSERT(!error);
		}
	}

	rfslock(drfp);

	/* Parent dir vnode must have remote reference, or can't look in it. */
	if (!psdp) {
		error = EACCES;
		goto out;
	}

	/*
	 * If the magic id is set, use it.  It provides super-user access on
	 * the server and guarantees that LINK will acquire a remote reference
	 * if the file exists. Normally this will not be set and we assume we're
	 * dealing with a server who does only lookup-type permissions checking.
	 * and not link() related checking.  The magic id is left in to deal
	 * with servers that aren't that nice.
	 */
	if (rfs_magic_cred != -1) {
		lcred.cr_uid = rfs_magic_cred;
		lcred.cr_gid = rfs_magic_cred;
	} else {
		lcred.cr_uid = cred->cr_uid;
		lcred.cr_gid = cred->cr_gid;
	}

	/* Get the remote reference using the link call */
	error = du_link(&pn, psdp, &lcred, &sdp, &ninfo);

	if (error == EDOTDOT) {  /* ..'ed server mnt point, return root vnode */
		ASSERT(multi);
		(void) VFS_ROOT(dvp->v_vfsp, vpp);
		error = 0;
	} else if (error) {	/* Real error, just return */
		goto out;
	} else {		/* Normal completion, make new vnode */
		error = get_rfsnode(dvp->v_vfsp, psdp,
					pn_getpath(&pn), sdp, &ninfo, vpp);
		(void) pn_set(&pn, "");	/* No path left */
	}

	/* If did multi-component, return remaining path */
	if (multi) {
		error = pn_copy(&pn, pnp);
		ASSERT(!error);
		if (lastcomp) {
			error = pn_append(pnp, lastcomp);
			ASSERT(!error);
		}
	}
out:
	DUPRINT6(DB_VNODE,
	    "rf_lookup: rslt %d, vp %x, type %d  sd %x, pn %s\n",
	    error, *vpp, *vpp ? (unsigned) (*vpp)->v_type : (unsigned) 0,
	    sdp, pnp ? pn_getpath(pnp) : "");
	rfsunlock(drfp);
	pn_free(&pn);
	if (lastcomp)
		kmem_fast_free(&kmp, lastcomp);
	return (error);
}

static int
rf_create(dvp, nm, vap, exclusive, mode, vpp, cred)
struct vnode *dvp;
char *nm;
struct vattr *vap;
enum vcexcl exclusive;
int mode;
struct vnode **vpp;
struct ucred *cred;
{
	struct rfsnode *drfp = vtorfs(dvp);
	sndd_t sdp = (sndd_t) NULL;
	struct nodeinfo ninfo;
	int error;
	u_short nmode;
	u_short openflag;
	int vcode, stat;
	struct rfsnode *rfp;

	DUPRINT6(DB_VNODE, "rf_create: dvp %x, nm %s, excl %x, va_mode %o, mode %x\n",
			dvp, nm, exclusive, vap->va_mode, mode);

	if (!drfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}

	switch (vap->va_type) {
	case VREG:
		/*
		 * Create file by opening it O_CREAT. KLUDGE:  Flag it open
		 * so that the caller won't try to open it again later in
		 * VOP_OPEN. This is not per-process so it only works if there's
		 * no context switch between the VOP_CREATE() and the VOP_OPEN()
		 */
		openflag = RFS_O_CREAT |
		    (vap->va_size == 0 ? RFS_O_TRUNC : 0) |
		    ((mode & (VREAD|VWRITE)) == VREAD ? RFS_O_RDONLY : 0) |
		    ((mode & (VREAD|VWRITE)) == VWRITE ? RFS_O_WRONLY : 0) |
		    ((mode & (VREAD|VWRITE)) == (VREAD|VWRITE) ? RFS_O_RDWR : 0)|
		    (exclusive == EXCL ? RFS_O_EXCL : 0);
		if (error = du_open(nm, drfp->rfs_sdp, cred, openflag,
			vap->va_mode, &sdp, &ninfo))
			break;
		stat = sdp->sd_stat;
		vcode = sdp->sd_vcode;
		error = get_rfsnode(
			    dvp->v_vfsp, drfp->rfs_sdp, nm, sdp, &ninfo, vpp);
		if (error)
			break;
		rfp = vtorfs(*vpp);
		rfp->rfs_flags |= RFSOPEN;	/* KLUDGE: flag file open */

		/* Get client cache status on file */
		rfp->rfs_sdp->sd_stat &= ~SDCACHE_MASK;
		rfp->rfs_sdp->sd_stat |= stat & SDCACHE_MASK;
		rfp->rfs_vcode = vcode;
		/* Check the cache and invalidate if out of date */
		/* XXX sd has already been freed by get_rfsnode() */
		/* rfs_cache_check(vp, sd) */
		break;
	case VDIR:
		/* can't create directories in rf_create. use rf_mkdir */
		if (vap->va_type == VDIR)
			error = EISDIR;
		break;
	case VBLK:
	case VCHR:
	case VFIFO:
		nmode = vap->va_mode | VTTORFS(vap->va_type);
		/* Device file: mknod */
		if (error=du_mknod(nm, drfp->rfs_sdp, cred, nmode, vap->va_rdev))
			break;
		/* Lookup result to get remote reference and make the vnode */
		error = rf_lookup(dvp, nm, vpp, cred, (struct pathname *)NULL, 0);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

out:
	DUPRINT2(DB_VNODE, "rf_create: result %d\n", error);
	return (error);
}

static int
rf_remove(dvp, nm, cred)
struct vnode *dvp;
char *nm;
struct ucred *cred;
{
	register struct rfsnode *drfp = vtorfs(dvp);
	int error;

	DUPRINT3(DB_VNODE, "rfremove: dvp %x, name %s\n", dvp, nm);

	if (!drfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}

	error = du_unlink(nm, drfp->rfs_sdp, cred);

out:
	DUPRINT2(DB_VNODE, "rfremove: result %d\n", error);
	return (error);
}

static int
rf_link(vp, tdvp, tnm, cred)
struct vnode *vp;
struct vnode *tdvp;
char *tnm;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	struct rfsnode *drfp = vtorfs(tdvp);
	sndd_t sdp = (sndd_t) NULL;
	struct nodeinfo ninfo;
	struct pathname pn;
	int error;

	DUPRINT4(DB_VNODE, "rflink: vp %x, tdvp %x, name %s\n", vp, tdvp, tnm);
	pn_alloc(&pn);
	(void) pn_set(&pn, rfp->rfs_name);

	if (!drfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}
	/* First half of call: get remote ref to source */
	if (error = du_link(&pn, rfp->rfs_psdp, cred, &sdp, &ninfo))
		goto out;

	/* Check the remote reference returned */
	error = transform_rfsnode(&vp, sdp, &ninfo);

	/* Second half of call: link it */
	error = du_link1(sdp, tnm, drfp->rfs_sdp, cred);
out:
	pn_free(&pn);
	DUPRINT2(DB_VNODE, "rflink: result %d\n", error);
	return (error);
}

/* Rename
 *	First link the new name then remove the old one.
 *	This insures that you can't lose the node in a system
 *	crash. But you can end up with a node with both the old
 *	and new names linked to it. RFS doesn't really provide
 *	you with the capability to get around this.
 */

static int
rf_rename(odvp, onm, ndvp, nnm, cred)
struct vnode *odvp;
char *onm;
struct vnode *ndvp;
char *nnm;
struct ucred *cred;
{
	struct rfsnode *odrfp = vtorfs(odvp);
	struct rfsnode *ndrfp = vtorfs(ndvp);
	sndd_t sdp = (sndd_t) NULL;
	struct nodeinfo ninfo;
	struct pathname pn;
	int error;

	DUPRINT5(DB_VNODE, "rfrename: odvp %x, onm %s, ndvp %x, nnm %s\n",
			odvp, onm, ndvp, nnm);
	pn_alloc(&pn);
	(void) pn_set(&pn, onm);

	if (!odrfp->rfs_sdp || !ndrfp->rfs_sdp)
		return (EACCES);

	/* Link the new name */

	/* First half of call: get remote ref to source */
	if (error = du_link(&pn, odrfp->rfs_sdp, cred, &sdp, &ninfo))
		goto out;

	/* Second half of call: link it */
	if (error = du_link1(sdp, nnm, ndrfp->rfs_sdp, cred)) {
		if (error != EEXIST)
			goto out;
		/* Destination exists try to unlink it, then try linking again */
		if (error = du_unlink(nnm, ndrfp->rfs_sdp, cred))
			goto out;
		if (error = du_link1(sdp, nnm, ndrfp->rfs_sdp, cred))
			goto out;
	}

	/* Unlink the old name */
	if (error = du_unlink(onm, odrfp->rfs_sdp, cred)) {
		/* Failed: try to unlink the new name */
		error = du_unlink(nnm, ndrfp->rfs_sdp, cred);
	}
out:
	pn_free(&pn);
	if (sdp)
		error = del_sndd(sdp);
	DUPRINT2(DB_VNODE, "rfrename: result %d\n", error);
	return (error);
}

/*
 * Make a remote directory. Note that the vnode mkdir op is expected to
 * return a vnode for the directory, but the RFS call, DUMKDIR, does not
 * hold open the inode for the directory on completion. Thus you have
 * to follow the DUMKDIR with an RFS lookup to hold open the remote inode.
 * This is potentially racy (someone could remove the directory between
 * the time you create it and the time you look it up), but it can't be helped.
 */
static int
rf_mkdir(dvp, nm, va, vpp, cred)
struct vnode *dvp;
char *nm;
register struct vattr *va;
struct vnode **vpp;
struct ucred *cred;
{
	struct rfsnode *drfp = vtorfs(dvp);
	int error;

	DUPRINT3(DB_VNODE, "rfmkdir: dvp %x, nm %s\n", dvp, nm);

	if (!drfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}

	/* Make the directory */
	if (error = du_mkdir(nm, drfp->rfs_sdp, cred, va->va_mode))
		goto out;

	/* Lookup result to get remote reference and make the vnode */
	error = rf_lookup(dvp, nm, vpp, cred, (struct pathname *) NULL, 0);

out:
	DUPRINT2(DB_VNODE, "rfmkdir: result %d\n", error);
	return (error);

}

static int
rf_rmdir(dvp, nm, cred)
struct vnode *dvp;
char *nm;
struct ucred *cred;
{
	struct rfsnode *drfp = vtorfs(dvp);
	int error;
	struct vnode *vp;

	DUPRINT3(DB_VNODE, "rfrmdir: dvp %x, nm %s\n", dvp, nm);

	if (!drfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}

	/* Lookup result to get remote reference */
	error = rf_lookup(dvp, nm, &vp, cred, (struct pathname *) NULL, 0);
	if (error) goto out;
	if (VN_CMP(vp, u.u_cdir)) {
		error = EINVAL;
		VN_RELE(vp);
		goto out;
	}
	VN_RELE(vp);

	error = du_rmdir(nm, drfp->rfs_sdp, cred);
out:
	DUPRINT2(DB_VNODE, "rfrmdir: result %d\n", error);
	return (error);
}

/*
 * rf_readdir
 * Have to copy the S5 directory entries into an in-kernel buffer,
 * convert them to Sun format, and copy them out to user land.
 */
#define	DCHUNK 512
static int
rf_readdir(vp, uiop, cred)
struct vnode *vp;
register struct uio *uiop;
struct ucred *cred;
{
	struct rfsnode *rfp = vtorfs(vp);
	int error;
	caddr_t ibuf = (caddr_t) NULL;
	caddr_t obuf = (caddr_t) NULL;
	int inbufcnt, outbufcnt;
	struct uio tuio;
	struct iovec tiov;
	struct dirent *dp;
	struct rfs_dirent *rdp;
	int in_reclen = 0;
	register char *inp, *onp;
	int bufsize;
	int reqsize;
	long last_offset;

	DUPRINT4(DB_VNODE, "rfreaddir: vp %x, uio offset %x, count %d\n",
		vp, uiop->uio_offset, uiop->uio_resid);

	/* Must have already opened directory and gotten remote reference */
	if (!rfp->rfs_sdp) {
		error = EACCES;
		goto out;
	}

	/*
	 * Regardless of how much was asked for request no more than DCHUNK
	 * data.  This hack is because of a bug which makes 3b server barf
	 * on large chunks if it has to XDRize. Making a small request works,
	 * because semantic of VOP_READDIR() says that you may get less than
	 * you asked for.
	 */
	reqsize = (uiop->uio_resid < DCHUNK ? uiop->uio_resid : DCHUNK);

	/* Setup to copy results to temp buffer */
	bcopy((caddr_t) uiop, (caddr_t) &tuio, sizeof (struct uio));
	tuio.uio_iov = &tiov;
	bufsize = 2 * reqsize;	/* Leave server room to return more than req'd */
	ibuf = new_kmem_alloc((u_int)bufsize, KMEM_SLEEP);
	tuio.uio_iov->iov_base = ibuf;
	tuio.uio_iov->iov_len = bufsize;
	tuio.uio_resid = bufsize;
	tuio.uio_seg = UIOSEG_KERNEL;
	obuf = new_kmem_alloc((u_int)bufsize, KMEM_SLEEP);

	/*
	 * Make the getdents call -- on return dirents are in buf and
	 * uio_offset contains remote directory offset.
	 */
	if (error = du_getdents(rfp->rfs_sdp, &tuio, reqsize, cred))
		goto out;

	/* Transform the entries (in place) to local format */
	for (inbufcnt = bufsize - tuio.uio_resid, outbufcnt = 0,
		rdp=(struct rfs_dirent *)ibuf, dp = (struct dirent *)obuf;
		(caddr_t) rdp  < (ibuf + inbufcnt);
		outbufcnt += dp->d_reclen,
		rdp = (struct rfs_dirent *) ((char *) rdp + in_reclen),
		dp = (struct dirent *) ((char *) dp + dp->d_reclen)) {
		in_reclen = rdp->d_reclen;
		dp->d_off = rdp->d_off;
		dp->d_fileno = rdp->d_ino;
		inp = rdp->d_name;
		onp = dp->d_name;
		while (*onp++ = *inp++);
		dp->d_namlen = inp - rdp->d_name;
		dp->d_reclen = DIRSIZ(dp);
		/* Got more than requested, back up one */
		if (outbufcnt + dp->d_reclen > reqsize)
			break;
		last_offset = rdp->d_off;
		DUPRINT5(DB_VNODE,
		    "rfreaddir: f# %d, reclen %d, namelen %d, name %s\n",
		    dp->d_fileno, dp->d_reclen, dp->d_namlen, dp->d_name);
	}

	/* Copy out the result to user space */
	error = uiomove(obuf, outbufcnt, UIO_READ, uiop);

	/* Return remote directory offset as side effect */
	uiop->uio_offset = last_offset;
out:
	if (ibuf)
		kmem_free(ibuf, (u_int) bufsize);
	if (obuf)
		kmem_free(obuf, (u_int) bufsize);
	DUPRINT4(DB_VNODE, "rfreaddir: result %d, ocnt %d, resid %d\n",
			error, outbufcnt, uiop->uio_resid);
	return (error);
}

/*ARGSUSED*/
static int
rf_symlink(dvp, lnm, tva, tnm, cred)
struct vnode *dvp;
char *lnm;
struct vattr *tva;
char *tnm;
struct ucred *cred;
{
	DUPRINT1(DB_VNODE, "rf_symlink: not supported\n");
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
static int
rf_readlink(vp, uiop, cred)
struct vnode *vp;
struct uio *uiop;
struct ucred *cred;
{
	DUPRINT1(DB_VNODE, "rf_readlink: not supported\n");
	return (EOPNOTSUPP);
}

/*
 * fsync op -- There is currently no equivalent of this in RFS, so instead
 * do a sync()
 */
/*ARGSUSED*/
static int
rf_fsync(vp, cred)
struct vnode *vp;
struct ucred *cred;
{
	struct vnode *rf_rootvp;
	struct rfsnode *rfp;
	int error = 0;

	DUPRINT2(DB_VNODE, "rf_fsync: vp %x\n", vp);

	/* Get the root node for the file system */
	if (error = VFS_ROOT(vp->v_vfsp, &rf_rootvp))
		goto out;

	rfp = vtorfs(rf_rootvp);

	/* Do the update system call */
	error = du_update(rfp->rfs_sdp);

	VN_RELE(rf_rootvp);
out:
	DUPRINT2(DB_VNODE, "rf_fsync: result %d\n", error);
	return (error);
}


/*
 *  Release an rf_vnode.
 *  Uncache the node.
 *  Free send descriptor, parent send descriptor and memory.
 */
static int
rf_inactive(vp)
	struct vnode *vp;
{
	register struct rfsnode *rfp = vtorfs(vp);
	int error = 0;

	ASSERT(rfp->rfs_sdp && rfp->rfs_psdp);

	DUPRINT5(DB_VNODE,
	    "rfinactive: freeing vp %x, sd %x, psd %x, name %s\n",
	    vp, rfp->rfs_sdp, rfp->rfs_psdp, rfp->rfs_name);

	rfs_cache_lock();
	rf_unsave(rfp, vp->v_vfsp);
	rfs_cache_unlock();
	error = del_sndd(rfp->rfs_psdp);	/* XXX -- what if it fails? */
	error = del_sndd(rfp->rfs_sdp);

	/* Invalidate the cache pages associated with the node */
	/* rfinval(); */

	((struct rfsmnt *)(vp->v_vfsp->vfs_data))->rm_refcnt--;
	kmem_free((caddr_t)rfp, (u_int) rfp->rfs_alloc);
	return (error);
}

/*ARGSUSED*/
static int
rf_lockctl(vp, ld, com, cred, clid)
struct vnode *vp;
struct flock *ld;
int com;
struct ucred *cred;
int clid;
{
	struct rfsnode *rfp = vtorfs(vp);
	int error;

	DUPRINT4(DB_VNODE, "rflockctl: vp %x, ld %x, com %x\n",
			vp, ld, com);

	if (!rfp->rfs_sdp) {
		error = EBADF;
		goto out;
	}

	error = du_fcntl(rfp->rfs_sdp, com, ld, cred);
out:
	DUPRINT2(DB_VNODE, "rflockctl: result %d\n", error);
	return (error);
}

/*ARGSUSED*/
static int
rf_fid(vp, fidpp)
struct vnode *vp;
struct fid **fidpp;
{
	DUPRINT1(DB_VNODE, "rf_fid: not supported\n");
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
static int
rf_getpage()
{
	DUPRINT1(DB_VNODE, "rf_putpage: not supported\n");
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
static int
rf_putpage()
{
	DUPRINT1(DB_VNODE, "rf_putpage: not supported\n");
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
static int
rf_map(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	DUPRINT1(DB_VNODE, "rf_map: not supported\n");
	return (EOPNOTSUPP);
}

static int
rf_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{
	register struct sndd *sdp1, *sdp2;
	/* Both are the same */
	if (vp1 == vp2)
		return (1);
	/* One is NULL (unfortunately some people pass in NULL vnodes) */
	if (vp1 == (struct vnode *) NULL || vp2 == (struct vnode *) NULL)
		return (0);
	/* One is not an rfs vnode */
	if (vp1->v_op != &rfs_vnodeops || vp2->v_op != &rfs_vnodeops)
		return (0);
	/* Two distinct rfs vnodes -- do they reference same remote inode? */
	sdp1 = vtorfs(vp1)->rfs_sdp;
	sdp2 = vtorfs(vp2)->rfs_sdp;
	return (sdp1->sd_queue == sdp2->sd_queue &&
		sdp1->sd_sindex == sdp2->sd_sindex);
}

static int
rf_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	*vpp = vp;
	return (0);
}

static int
rf_badop()
{
	panic("rf_badop");
}

/*ARGSUSED*/
static int
rf_cntl(vp, cmd, idata, odata, iflg, oflg)
	struct vnode *vp;
	caddr_t idata, odata;
{
	return (ENOSYS);
}
