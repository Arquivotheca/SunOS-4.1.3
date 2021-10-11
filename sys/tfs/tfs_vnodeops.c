/*	@(#)tfs_vnodeops.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/pathname.h>
#include <netinet/in.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <tfs/tfs.h>
#include <tfs/tnode.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>
#include <vm/seg.h>

#define	GET_REALVP(vp, cred)	REALVP(vp) ? 0 : \
					tfs_translate(VTOT(vp), FALSE, cred)

/*
 * If the real vnode of a TFS vnode is an NFS vnode, and the tfsd is not
 * on the local machine, then we need to explicitly flush the local NFS
 * attr cache when an operation on the remote machine invalidates it.
 * Otherwise we won't be able to tell when a TFS directory changes.
 */
#define	NFSATTR_INVAL(vp) \
	if (REALVP(vp) && REALVP(vp)->v_op == &nfs_vnodeops) \
		vtor(REALVP(vp))->r_attrtime = time

/*
 * If the real vnode of a TFS vnode is an NFS vnode, and the tfsd is not
 * on the local machine, then we sometimes have to update the r_size field
 * of the NFS vnode, since it won't be done by the local NFS code.
 */
#define	NFS_SET_SIZE(vp, newsize) \
	if (REALVP(vp) && REALVP(vp)->v_op == &nfs_vnodeops) \
		vtor(REALVP(vp))->r_size = newsize

#define	SETDIROPARGS(args, nm, tp) \
	(args)->da_fhandle = (tp)->t_fh; \
	(args)->da_name = nm

#ifdef TFSDEBUG
int	tfsdebug = 0;
struct vfsstats tfsstats;
#endif
extern int tnode_sleeps;
extern struct vnodeops nfs_vnodeops;

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the translucent file system.  All routines which modify/read directories
 * rely on the user-level tfsd process to do the work, while most of the
 * operations which operate on files just take their parameters, figure out
 * the real vnode to operate on, and then call the appropriate real vnode
 * routine(s) to do the work.  Note that we need to hold the real vnode
 * before calling the real vnode routine, to prevent it from being
 * inactivated if another process clears the real vnode.
 *
 * We are very careful not to use much kernel stack space, especially
 * in routines which call maketfsnode(), which in turn calls lookupname()
 * and may result in a very deep call stack.  Consequently, routines such
 * as do_lookup() exist so that large local variables (e.g. a struct
 * nfsdiropargs) stay on the stack only as long as they are needed.
 */

static int tfs_open();
static int tfs_close();
static int tfs_rdwr();
static int tfs_ioctl();
static int tfs_select();
int tfs_getattr();
static int tfs_setattr();
static int tfs_access();
int tfs_lookup();
static int tfs_create();
static int tfs_remove();
static int tfs_link();
static int tfs_rename();
static int do_rename();
static int tfs_mkdir();
static int tfs_rmdir();
static int tfs_readdir();
static int tfs_symlink();
static int tfs_readlink();
static int tfs_fsync();
static int tfs_inactive();
static int tfs_lockctl();
static int tfs_getpage();
static int tfs_putpage();
static int tfs_map();
static int tfs_cmp();
static int tfs_realvp();
int tfs_badop();
int tfs_noop();

static int do_setattr();
static int getattrs_of();
static int do_lookup();
static int do_create();
static int do_remove();

extern struct vnode *dnlc_lookup();
extern struct tfsdiropres *tfsdiropres_get();

/*
 * TFS operations
 */
struct vnodeops tfs_vnodeops = {
	tfs_open,
	tfs_close,
	tfs_rdwr,
	tfs_ioctl,
	tfs_select,
	tfs_getattr,
	tfs_setattr,
	tfs_access,
	tfs_lookup,
	tfs_create,
	tfs_remove,
	tfs_link,
	tfs_rename,
	tfs_mkdir,
	tfs_rmdir,
	tfs_readdir,
	tfs_symlink,
	tfs_readlink,
	tfs_fsync,
	tfs_inactive,
	tfs_lockctl,
	tfs_noop,	/* fid */
	tfs_getpage,
	tfs_putpage,
	tfs_map,
	tfs_badop,	/* dump */
	tfs_cmp,
	tfs_realvp,
	tfs_noop,
};

static int
tfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct vnode *realvp;
	bool_t retried = FALSE;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_open vp %x flag %d\n", *vpp, flag);
	VFS_RECORD((*vpp)->v_vfsp, VS_OPEN, VS_CALL);
#endif
again:
	if (error = GET_REALVP(*vpp, cred)) {
		return (error);
	}
	realvp = REALVP(*vpp);
	VN_HOLD(realvp);
	error = VOP_OPEN(&realvp, flag, cred);
	VN_RELE(realvp);
	if (error == ESTALE && !retried) {
		if (REALVP(*vpp)) {
			realvp = REALVP(*vpp);
			REALVP(*vpp) = NULL;
			VN_RELE(realvp);
		}
		retried = TRUE;
		goto again;
	}
	return (error);
}

static int
tfs_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_close vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_CLOSE, VS_CALL);
#endif
	if (VTOT(vp)->t_unlinkp) {
		/*
		 * Closing a file that was unlinked while it was open.
		 * We want this vnode to be freed as soon as possible.
		 */
		dnlc_purge_vp(vp);
	}
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_CLOSE(realvp, flag, count, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_rdwr vp %x  rw %s  offset %x\n",
		vp, rw == UIO_READ ? "READ" : "WRITE", uiop->uio_offset);
	VFS_RECORD(vp->v_vfsp, rw == UIO_READ ? VS_READ : VS_WRITE, VS_CALL);
#endif
	error = GET_REALVP(vp, cred);
	if (!VTOT(vp)->t_writeable && !error) {
		if (rw == UIO_READ) {
			struct vattr va;

			/*
			 * Check to see if the file has been copy-on-writed
			 * from another machine.
			 */
			error = tfs_getattr(vp, &va, cred);
		} else {
			/*
			 * Copy-on-write the file.
			 */
			error = tfs_translate(VTOT(vp), TRUE, cred);
		}
	}
	if (error) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_RDWR(realvp, uiop, rw, ioflag, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	VFS_RECORD(vp->v_vfsp, VS_IOCTL, VS_CALL);
#endif TFSDEBUG
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_IOCTL(realvp, com, data, flag, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	VFS_RECORD(vp->v_vfsp, VS_SELECT, VS_CALL);
#endif TFSDEBUG
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_SELECT(realvp, which, cred);
	VN_RELE(realvp);
	return (error);
}

int
tfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;
	struct nfssattr *sa;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_getattr vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_GETATTR, VS_CALL);
#endif
again:
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_GETATTR(realvp, vap, cred);
	VN_RELE(realvp);
	if (error) {
		if (error == ESTALE) {
			if (REALVP(vp)) {
				realvp = REALVP(vp);
				REALVP(vp) = NULL;
				VN_RELE(realvp);
			}
			goto again;
		}
	} else {
		if (VTOT(vp)->t_ctime.tv_sec &&
		    (vap->va_ctime.tv_sec != VTOT(vp)->t_ctime.tv_sec ||
		    vap->va_ctime.tv_usec != VTOT(vp)->t_ctime.tv_usec)) {
			if (!VTOT(vp)->t_writeable) {
				/*
				 * File has been copy-on-writed by someone
				 * else.
				 */
				if (REALVP(vp)) {
					realvp = REALVP(vp);
					REALVP(vp) = NULL;
					VN_RELE(realvp);
				}
				goto again;
			} else {
				/*
				 * Directory has changed -- purge dnlc
				 */
				dnlc_purge_vp(vp);
				VTOT(vp)->t_ctime = vap->va_ctime;
			}
		}
		vap->va_nodeid = VTOT(vp)->t_nodeid;
		vap->va_fsid = 0x0ffff &
				(long)makedev(vfs_fixedmajor(vp->v_vfsp),
					0xff & vtomi(vp)->mi_mntno);
		sa = &VTOT(vp)->t_sattrs;
		if (sa->sa_mode != (long) -1) {
			vap->va_mode = sa->sa_mode;
		}
		if (sa->sa_uid != (long) -1) {
			vap->va_uid = sa->sa_uid;
		}
		if (sa->sa_gid != (long) -1) {
			vap->va_gid = sa->sa_gid;
		}
		if (sa->sa_size != (long) -1) {
			vap->va_size = sa->sa_size;
		}
		if (sa->sa_atime.tv_sec != (long) -1) {
			vap->va_atime.tv_sec = sa->sa_atime.tv_sec;
			vap->va_atime.tv_usec = sa->sa_atime.tv_usec;
		}
		if (sa->sa_mtime.tv_sec != (long) -1) {
			vap->va_mtime.tv_sec = sa->sa_mtime.tv_sec;
			vap->va_mtime.tv_usec = sa->sa_mtime.tv_usec;
		}
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_getattr: returning %d\n", error);
#endif
	return (error);
}

static int
tfs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	struct tfsdiropres *dr;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_setattr vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_SETATTR, VS_CALL);
#endif
	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	dr = tfsdiropres_get();
	error = do_setattr(vp, vap, dr, cred);
	NFSATTR_INVAL(vp);	/* some attrs have changed */
	if (!error) {
		error = geterrno(dr->dr_status);
	}
	if (!error) {
		/*
		 * TFS attribute mask may have changed, so re-init the tnode.
		 */
		TLOCK(VTOT(vp));
		error = init_tnode(VTOT(vp), dr, cred);
		TUNLOCK(VTOT(vp));
		if (vap->va_size != -1) {
			NFS_SET_SIZE(vp, vap->va_size);
		}
	}
	tfsdiropres_put(dr);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_setattr: returning %d\n", error);
#endif
	return (error);
}

static int
tfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct vattr va;
	int *gp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_access vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_ACCESS, VS_CALL);
#endif
	error = tfs_getattr(vp, &va, cred);
	if (error) {
		return (error);
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (cred->cr_uid == 0)
		return (0);
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (cred->cr_uid != va.va_uid) {
		mode >>= 3;
		if (cred->cr_gid == va.va_gid)
			goto found;
		gp = cred->cr_groups;
		for (; gp < &cred->cr_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (va.va_gid == *gp)
				goto found;
		mode >>= 3;
	}
found:
	if ((va.va_mode & mode) == mode) {
		return (0);
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_access: returning EACCES\n");
#endif
	return (EACCES);
}

static int
tfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_readlink vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_READLINK, VS_CALL);
#endif
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_READLINK(realvp, uiop, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_fsync vp %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_FSYNC, VS_CALL);
#endif
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_FSYNC(realvp, cred);
	VN_RELE(realvp);
	return (error);
}

/*
 * If the file was removed while it was open it got renamed (by tfs_remove)
 * instead.  Here we remove the renamed file.
 */
/* ARGSUSED */
static int
tfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_inactive %x\n", vp);
	VFS_RECORD(vp->v_vfsp, VS_INACTIVE, VS_CALL);
#endif
	tfs_tunsave(VTOT(vp));
	if (VTOT(vp)->t_unlinkp) {
		enum nfsstat status;
		struct tnode_unl *tup = VTOT(vp)->t_unlinkp;

		(void) do_remove(tup->tu_dvp, tup->tu_name, &status,
					tup->tu_cred);
		VN_RELE(tup->tu_dvp);
		kmem_free(tup->tu_name, (u_int)NFS_MAXNAMLEN);
		crfree(tup->tu_cred);
		kmem_free((caddr_t)tup, sizeof (*tup));
		VTOT(vp)->t_unlinkp = NULL;
	}
	tfs_tfree(VTOT(vp));
	vtomi(vp)->mi_refct--;
	return (0);
}

/*ARGSUSED*/
int
tfs_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	struct tfsdiropres *dr;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_lookup %x '%s'\n", dvp, nm);
	VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_CALL);
#endif
	/*
	 * Before checking dnlc, call getattr to be
	 * sure directory hasn't changed.  getattr
	 * will purge dnlc if a change has occurred.
	 */
	if (error = getattrs_of(dvp, cred)) {
		return (error);
	}
	TLOCK(VTOT(dvp));
	*vpp = dnlc_lookup(dvp, nm, cred);
	if (*vpp) {
		VN_HOLD(*vpp);
	} else {
#ifdef TFSDEBUG
		tfs_dprint(tfsdebug, 4, "  tfs_lookup: calling tfsd\n");
		VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_MISS);
#endif
		dr = tfsdiropres_get();
		error = do_lookup(dvp, nm, dr, cred);
		if (!error) {
			error = geterrno(dr->dr_status);
		}
		if (!error) {
			error = maketfsnode(dr, dvp->v_vfsp, vpp, cred);
			if (!error) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}
		} else {
			*vpp = NULL;
		}
		tfsdiropres_put(dr);
		/*
		 * If the directory wasn't in a writeable layer before
		 * the lookup, then it is now, since the tfsd always
		 * copy-on-writes a directory when reading it for the
		 * first time.
		 */
		if (!VTOT(dvp)->t_writeable && REALVP(dvp)) {
			struct vnode	*realvp = REALVP(dvp);

			REALVP(dvp) = NULL;
			VN_RELE(realvp);
		}
	}
	TUNLOCK(VTOT(dvp));
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_lookup: returning %d  vp %x\n", error,
			*vpp);
#endif
	return (error);
}

/*ARGSUSED*/
static int
tfs_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct tfsdiropres *dr;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_create dvp %x '%s'\n", dvp, nm);
	VFS_RECORD(dvp->v_vfsp, VS_CREATE, VS_CALL);
#endif
	if (exclusive == EXCL) {
		/*
		 * We should send the exclusive flag over the wire.
		 */
		error = tfs_lookup(dvp, nm, vpp, cred,
				(struct pathname *)0, 0);
		if (!error) {
			VN_RELE(*vpp);
			*vpp = NULL;
			error = EEXIST;
			goto bad;
		}
	}
	*vpp = NULL;
	TLOCK(VTOT(dvp));
	dr = tfsdiropres_get();
	error = do_create(dvp, nm, va, TFS_CREATE, dr, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
	}
	if (!error) {
		error = maketfsnode(dr, dvp->v_vfsp, vpp, cred);
		if (!error) {
			if (va->va_size == 0) {
				NFS_SET_SIZE(*vpp, 0);
			}
			error = tfs_getattr(*vpp, va, cred);
		}
		if (!error) {
			dnlc_enter(dvp, nm, *vpp, cred);
		} else {
			*vpp = NULL;
		}
	}
	TUNLOCK(VTOT(dvp));
	tfsdiropres_put(dr);
bad:
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_create: returning %d  vp %x\n", error,
			*vpp);
#endif
	return (error);
}

static int
tfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	enum nfsstat status;
	struct vnode *vp;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_remove dvp %x '%s'\n", dvp, nm);
	VFS_RECORD(dvp->v_vfsp, VS_REMOVE, VS_CALL);
#endif
	error = tfs_lookup(dvp, nm, &vp, cred, (struct pathname *)0, 0);
	if (error) {
		goto bad;
	}
	/*
	 * We need to check the real reference count on the vnode, to
	 * determine whether or not we can really remove it.
	 */
	dnlc_purge_vp(vp);
	if (vp->v_count > 1 && VTOT(vp)->t_unlinkp == NULL) {
		char *tmpname;
		extern char *newname();

		/*
		 * Unlinking an open file.
		 */
		tmpname = newname();
		error = do_rename(dvp, nm, dvp, tmpname, cred);
		if (error) {
			kmem_free(tmpname, (u_int)NFS_MAXNAMLEN);
		} else {
			struct tnode_unl *tup;

			tup = (struct tnode_unl *)new_kmem_alloc(
					(u_int)sizeof (*tup), KMEM_SLEEP);
			VN_HOLD(dvp);
			tup->tu_dvp = dvp;
			tup->tu_name = tmpname;
			crhold(cred);
			tup->tu_cred = cred;
			VTOT(vp)->t_unlinkp = tup;
		}
		/*
		 * Have to do this in case the real vnode isn't in the
		 * frontmost layer, and the tfsd is on a remote machine.
		 * We don't want the renamed real vnode to be found in the
		 * dnlc under its old name.
		 */
		if (REALVP(vp) && REALVP(vp)->v_count > 1) {
			dnlc_purge_vp(REALVP(vp));
		}
	} else {
		error = do_remove(dvp, nm, &status, cred);
		NFSATTR_INVAL(dvp);	/* mod time changed */
		NFSATTR_INVAL(vp);	/* link count changed */
		if (!error) {
			error = geterrno(status);
		}
	}
	VN_RELE(vp);
bad:
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_remove: returning %d\n", error);
#endif
	return (error);
}

static int
tfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	struct nfslinkargs args;
	enum nfsstat status;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_link vp %x to dvp %x '%s'\n",
			vp, tdvp, tnm);
	VFS_RECORD(vp->v_vfsp, VS_LINK, VS_CALL);
#endif
	args.la_from = VTOT(vp)->t_fh;
	SETDIROPARGS(&args.la_to, tnm, VTOT(tdvp));
	error = tfscall(vtomi(vp), TFS_LINK, xdr_linkargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(tdvp);	/* mod time changed */
	NFSATTR_INVAL(vp);	/* link count changed */
	if (!error) {
		error = geterrno(status);
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_link: returning %d\n", error);
#endif
	return (error);
}

static int
tfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_rename from dvp %x '%s' to dvp %x '%s'\n",
			odvp, onm, ndvp, nnm);
	VFS_RECORD(odvp->v_vfsp, VS_RENAME, VS_CALL);
#endif
	error = do_rename(odvp, onm, ndvp, nnm, cred);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_rename: returning %d\n", error);
#endif
	return (error);
}

static int
do_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	struct nfsrnmargs args;
	enum nfsstat status;
	int error;

	SETDIROPARGS(&args.rna_from, onm, VTOT(odvp));
	SETDIROPARGS(&args.rna_to, nnm, VTOT(ndvp));
	error = tfscall(vtomi(odvp), TFS_RENAME, xdr_rnmargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(odvp);	/* mod time changed */
	NFSATTR_INVAL(ndvp);	/* mod time changed */
	if (!error) {
		error = geterrno(status);
	}
	return (error);
}

static int
tfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct tfsdiropres *dr;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_mkdir dvp %x '%s'\n", dvp, nm);
	VFS_RECORD(dvp->v_vfsp, VS_MKDIR, VS_CALL);
#endif
	TLOCK(VTOT(dvp));
	dr = tfsdiropres_get();
	error = do_create(dvp, nm, va, TFS_MKDIR, dr, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
	}
	if (!error) {
		error = maketfsnode(dr, dvp->v_vfsp, vpp, cred);
		if (!error) {
			dnlc_enter(dvp, nm, *vpp, cred);
		}
	} else {
		*vpp = NULL;
	}
	TUNLOCK(VTOT(dvp));
	tfsdiropres_put(dr);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_mkdir: returning %d  vp %x\n", error,
			*vpp);
#endif
	return (error);
}

static int
tfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	struct nfsdiropargs da;
	enum nfsstat status;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_rmdir dvp %x '%s'\n", dvp, nm);
	VFS_RECORD(dvp->v_vfsp, VS_RMDIR, VS_CALL);
#endif
	SETDIROPARGS(&da, nm, VTOT(dvp));
	error = tfscall(vtomi(dvp), TFS_RMDIR, xdr_diropargs, (caddr_t)&da,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(status);
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_rmdir: returning %d\n", error);
#endif
	return (error);
}

static int
tfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	struct nfsslargs args;
	enum nfsstat status;
	int error;

#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_symlink dvp %x '%s' to '%s'\n",
			dvp, lnm, tnm);
	VFS_RECORD(dvp->v_vfsp, VS_SYMLINK, VS_CALL);
#endif
	SETDIROPARGS(&args.sla_from, lnm, VTOT(dvp));
	vattr_to_sattr(tva, &args.sla_sa);
	args.sla_tnm = tnm;
	error = tfscall(vtomi(dvp), TFS_SYMLINK, xdr_slargs, (caddr_t)&args,
			xdr_enum, (caddr_t)&status, cred);
	NFSATTR_INVAL(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(status);
	}
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_symlink: returning %d\n", error);
#endif
	return (error);
}

/*
 * Read directory entries.
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.
 */
static int
tfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	struct nfsrddirargs rda;
	struct nfsrddirres rd;
	struct tnode *tp;
	int count;
	int error;

	count = uiop->uio_iov->iov_len;
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 4, "tfs_readdir vp %x count %d offset %d\n",
			vp, count, uiop->uio_offset);
	VFS_RECORD(vp->v_vfsp, VS_READDIR, VS_CALL);
#endif
	tp = VTOT(vp);
	if (tp->t_eof && tp->t_size == (long) uiop->uio_offset) {
		return (0);
	}
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	rda.rda_offset = uiop->uio_offset;
	rda.rda_fh = VTOT(vp)->t_fh;
	rd.rd_entries = (struct dirent *)
				new_kmem_alloc((u_int)count, KMEM_SLEEP);
	do {
		count = MIN(count, vtomi(vp)->mi_curread);
		rda.rda_count = count;
		rd.rd_size = count;

		error = tfscall(vtomi(vp), TFS_READDIR, xdr_rddirargs,
				(caddr_t)&rda, xdr_getrddirres, (caddr_t)&rd,
				cred);
	} while (error == ENFS_TRYAGAIN);
	if (!error) {
		error = geterrno(rd.rd_status);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (rd.rd_size) {
			error = uiomove((caddr_t)rd.rd_entries,
			    (int)rd.rd_size, UIO_READ, uiop);
			uiop->uio_offset = rd.rd_offset;
		}
		tp->t_eof = rd.rd_eof;
		if (tp->t_eof) {
			tp->t_size = uiop->uio_offset;
		}
	}
	kmem_free((caddr_t)rd.rd_entries, (u_int)count);
#ifdef TFSDEBUG
	tfs_dprint(tfsdebug, 5, "tfs_readdir: returning %d\n", error);
#endif
	return (error);
}

/* ARGSUSED */
static int
tfs_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

#ifdef TFSDEBUG
	VFS_RECORD(vp->v_vfsp, VS_LOCKCTL, VS_CALL);
#endif TFSDEBUG
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_LOCKCTL(realvp, ld, cmd, cred, clid);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_GETPAGE(realvp, off, len, protp, pl, plsz, seg, addr,
			    rw, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_putpage(vp, off, len, flags, cred)
	struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_CALL);
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_PUTPAGE(realvp, off, len, flags, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_map(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct vnode *realvp;
	int error;

	VFS_RECORD(vp->v_vfsp, VS_MAP, VS_CALL);
	if (error = GET_REALVP(vp, cred)) {
		return (error);
	}
	realvp = REALVP(vp);
	VN_HOLD(realvp);
	error = VOP_MAP(realvp, off, as, addr, len, prot, maxprot,
			flags, cred);
	VN_RELE(realvp);
	return (error);
}

static int
tfs_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{
	int error;

	VFS_RECORD(vp1->v_vfsp, VS_CMP, VS_CALL);
	if (vp1->v_op == &tfs_vnodeops) {
		if (error = GET_REALVP(vp1, u.u_cred)) {
			return (error);
		}
		vp1 = REALVP(vp1);
	}
	if (vp2->v_op == &tfs_vnodeops) {
		if (error = GET_REALVP(vp2, u.u_cred)) {
			return (error);
		}
		vp2 = REALVP(vp2);
	}
	return (VOP_CMP(vp1, vp2));
}

static int
tfs_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	struct vnode *rvp;
	int error;

	VFS_RECORD(vp->v_vfsp, VS_REALVP, VS_CALL);
	if (vp->v_op == &tfs_vnodeops) {
		if (error = GET_REALVP(vp, u.u_cred)) {
			return (error);
		}
		vp = REALVP(vp);
	}
	if (VOP_REALVP(vp, &rvp) == 0) {
		vp = rvp;
	}
	*vpp = vp;
	return (0);
}

int
tfs_badop()
{
	panic("tfs_badop");
}

int
tfs_noop()
{
	return (EREMOTE);
}

/*
 * The following routines exist to save kernel stack space.
 */
static int
do_setattr(vp, vap, dr, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct tfsdiropres *dr;
	struct ucred *cred;
{
	struct nfssaargs args;

	vattr_to_sattr(vap, &args.saa_sa);
	args.saa_fh = VTOT(vp)->t_fh;
	return (tfscall(vtomi(vp), TFS_SETATTR, xdr_saargs, (caddr_t)&args,
			xdr_tfsdiropres, (caddr_t)dr, cred));
}

static int
getattrs_of(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct vattr va;

	return (tfs_getattr(vp, &va, cred));
}

static int
do_lookup(dvp, nm, dr, cred)
	struct vnode *dvp;
	char *nm;
	struct tfsdiropres *dr;
	struct ucred *cred;
{
	struct nfsdiropargs da;

	SETDIROPARGS(&da, nm, VTOT(dvp));
	return (tfscall(vtomi(dvp), TFS_LOOKUP, xdr_diropargs, (caddr_t)&da,
			xdr_tfsdiropres, (caddr_t)dr, cred));
}

/*
 * XXX use the sa_uid field of CREATE and MKDIR args as a transaction
 * ID, so that the tfsd can use this ID to check for duplicate requests.
 * The correct way to do this would be to add a transaction ID to the TFS
 * protocol for all requests that should only happen once, not just CREATE
 * and MKDIR.  (REMOVE, RMDIR, RENAME, LINK, & SYMLINK should also use
 * a transaction ID.)
 */
static int	tfs_xid = 1000000;

static int
do_create(dvp, nm, va, procnum, dr, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	int procnum;
	struct tfsdiropres *dr;
	struct ucred *cred;
{
	struct nfscreatargs args;

	SETDIROPARGS(&args.ca_da, nm, VTOT(dvp));
	vattr_to_sattr(va, &args.ca_sa);
	if (args.ca_sa.sa_uid == -1) {
		/*
		 * Use the uid field as a transaction ID, making sure that
		 * the transaction ID is an impossible user ID.
		 */
		args.ca_sa.sa_uid = tfs_xid++;
		if (tfs_xid == -1) {
			tfs_xid = 1000000;
		}
	}
	return (tfscall(vtomi(dvp), procnum, xdr_creatargs, (caddr_t)&args,
			xdr_tfsdiropres, (caddr_t)dr, cred));
}

static int
do_remove(dvp, nm, statusp, cred)
	struct vnode *dvp;
	char *nm;
	enum nfsstat *statusp;
	struct ucred *cred;
{
	struct nfsdiropargs da;

	SETDIROPARGS(&da, nm, VTOT(dvp));
	return (tfscall(vtomi(dvp), TFS_REMOVE, xdr_diropargs, (caddr_t)&da,
			xdr_enum, (caddr_t)statusp, cred));
}
