/*	@(#)lo_vnodeops.c 1.1 92/07/30 SMI 	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

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
#include <sys/pathname.h>
#include <lofs/lnode.h>
#include <lofs/loinfo.h>

struct vnode *makelonode();
#ifdef	LODEBUG
int lodebug;
#endif	LODEBUG

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the looped-back file system.  These routines just take their parameters,
 * and then calling the appropriate real vnode routine(s) to do the work.
 */

static int lo_open();
static int lo_close();
static int lo_getattr();
static int lo_setattr();
static int lo_access();
static int lo_lookup();
static int lo_create();
static int lo_remove();
static int lo_link();
static int lo_rename();
static int lo_mkdir();
static int lo_rmdir();
static int lo_readdir();
static int lo_symlink();
static int lo_fsync();
static int lo_inactive();
static int lo_lockctl();
static int lo_cmp();
static int lo_realvp();
static int lo_cntl();
int lo_badop();		/* used by lo_vfsops.c */

/*
 * Loopback operations: Only directories are covered by loopback vnodes
 * so we only have to support operations on directories
 */
struct vnodeops lo_vnodeops = {
	lo_open,
	lo_close,
	lo_badop,	/* rdwr */
	lo_badop,	/* ioctl */
	lo_badop,	/* select */
	lo_getattr,
	lo_setattr,
	lo_access,
	lo_lookup,
	lo_create,
	lo_remove,
	lo_link,
	lo_rename,
	lo_mkdir,
	lo_rmdir,
	lo_readdir,
	lo_symlink,
	lo_badop,	/* readlink */
	lo_fsync,
	lo_inactive,
	lo_lockctl,	/* lockctl */
	lo_badop,	/* fid - can't export lofs's */
	lo_badop,	/* getpage */
	lo_badop,	/* putpage */
	lo_badop,	/* map */
	lo_badop,	/* dump */
	lo_cmp,
	lo_realvp,
	lo_cntl,
};

static int
lo_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_open vp %x realvp %x\n", *vpp, realvp(*vpp));
#endif
	return (VOP_OPEN(&realvp(*vpp), flag, cred));
}

static int
lo_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_close vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_CLOSE(realvp(vp), flag, count, cred));
}

static int
lo_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	int error;

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_getattr vp %x realvp %x\n", vp, realvp(vp));
#endif
	error = VOP_GETATTR(realvp(vp), vap, cred);
	return (error);
}

static int
lo_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_setattr vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_SETATTR(realvp(vp), vap, cred));
}

static int
lo_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_access vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_ACCESS(realvp(vp), mode, cred));
}

static int
lo_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_fsync vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_FSYNC(realvp(vp), cred));
}

/*ARGSUSED*/
static int
lo_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_inactive %x, realvp %x\n", vp, realvp(vp));
#endif
	VN_RELE(realvp(vp));
	freelonode(vtol(vp));
	return (0);
}

static int
lo_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	struct vnode *vp = NULL, *tvp;
	int error;
	struct vnode *realdvp = realvp(dvp);
	struct vfs *vfsp;

	*vpp = NULL;	/* default(error) case */

	/*
	 * Handle ".." out of mounted filesystem
	 */
	while ((realdvp->v_flag & VROOT) && strcmp(nm, "..") == 0)
		realdvp = realdvp->v_vfsp->vfs_vnodecovered;

	/*
	 * Do the normal lookup
	 */
	error = VOP_LOOKUP(realdvp, nm, &vp, cred, pnp, flags);
	if (error)
		goto out;

	/*
	 * If this vnode is mounted on, then we
	 * transparently indirect to the vnode which
	 * is the root of the mounted file system.
	 * Before we do this we must check that an unmount is not
	 * in progress on this vnode. This maintains the fs status
	 * quo while a possibly lengthy unmount is going on.
	 * (stolen from vfs_lookup.c)
	 */
mloop:
	while ((vfsp = vp->v_vfsmountedhere) != 0) {
		/*
		 * Don't traverse a loopback mountpoint unless its
		 * going to the next higher depth
		 * This prevents loops to ourselves either directly or not
		 */
		if (vfsp->vfs_op == &lo_vfsops &&
		    vtoli(vfsp)->li_depth != vtoli(dvp->v_vfsp)->li_depth+1)
			break;
		while (vfsp->vfs_flag & VFS_MLOCK) {
			vfsp->vfs_flag |= VFS_MWAIT;
			(void) sleep((caddr_t)vfsp, PVFS);
			goto mloop;
		}
		error = VFS_ROOT(vfsp, &tvp);
		VN_RELE(vp);
		if (error)
			goto out;
		vp = tvp;
	}

	/*
	 * Now make our loop vnode for the real vnode
	 * But we only have to do it if the real vnode is a directory
	 * We can't do it for shared text without hacking on distpte
	 */
	if (vp->v_type == VDIR)
		*vpp = makelonode(vp, vtoli(dvp->v_vfsp));
	else
		*vpp = vp;

out:
#ifdef LODEBUG
	lo_dprint(lodebug, 4,
	   "lo_lookup dvp %x realdvp %x nm '%s' newvp %x real vp %x error %d\n",
	   dvp, realvp(dvp), nm, *vpp, vp, error);
#endif
	return (error);
}

/*ARGSUSED*/
static int
lo_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct vnode *vp;
	int error;

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_create vp %x realvp %x\n", dvp, realvp(dvp));
#endif
	error = VOP_CREATE(realvp(dvp), nm, va, exclusive, mode,  &vp, cred);
	if (!error) {
		*vpp = vp;
	} else {
		*vpp = NULL;
	}
	return (error);
}

static int
lo_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_remove vp %x realvp %x\n", dvp, realvp(dvp));
#endif
	return (VOP_REMOVE(realvp(dvp), nm,  cred));
}

static int
lo_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_link vp %x realvp %x\n", vp, realvp(vp));
#endif
	while (vp->v_op == &lo_vnodeops)
		vp = realvp(vp);
	while (tdvp->v_op == &lo_vnodeops)
		tdvp = realvp(tdvp);
	if (vp->v_vfsp != tdvp->v_vfsp)
		return (EXDEV);
	return (VOP_LINK(vp, tdvp, tnm, cred));
}

static int
lo_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_rename vp %x realvp %x\n", odvp, realvp(odvp));
#endif
	while (odvp->v_op == &lo_vnodeops)
		odvp = realvp(odvp);
	while (ndvp->v_op == &lo_vnodeops)
		ndvp = realvp(ndvp);
	if (odvp->v_vfsp != ndvp->v_vfsp)
		return (EXDEV);
	return (VOP_RENAME(odvp, onm, ndvp, nnm, cred));
}

static int
lo_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct vnode *vp;
	int error;

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_mkdir vp %x realvp %x\n", dvp, realvp(dvp));
#endif
	error = VOP_MKDIR(realvp(dvp), nm, va, &vp, cred);
	if (!error) {
		*vpp = makelonode(vp, vtoli(dvp->v_vfsp));
	} else {
		*vpp = NULL;
	}
	return (error);
}

static int
lo_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_rmdir vp %x realvp %x\n", dvp, realvp(dvp));
#endif
	return (VOP_RMDIR(realvp(dvp), nm, cred));
}

static int
lo_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_symlink vp %x realvp %x\n", dvp, realvp(dvp));
#endif
	return (VOP_SYMLINK(realvp(dvp), lnm, tva, tnm, cred));
}

static int
lo_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_readdir vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_READDIR(realvp(vp), uiop, cred));
}

static int
lo_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	int clid;
{

#ifdef LODEBUG
	lo_dprint(lodebug, 2, "lo_lockctl vp %x realvp %x\n", vp, realvp(vp));
#endif
	return (VOP_LOCKCTL(realvp(vp), ld, cmd, cred, clid));
}

static int
lo_cmp(vp1, vp2)
        struct vnode *vp1, *vp2;
{

	if (vp1->v_op == &lo_vnodeops)
		vp1 = realvp(vp1);
	if (vp2->v_op == &lo_vnodeops)
		vp2 = realvp(vp2);
        return (VOP_CMP(vp1, vp2));
}

static int
lo_realvp(vp, vpp)
        struct vnode *vp;
	struct vnode **vpp;
{
	struct vnode *rvp;

#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_realvp %x\n", vp);
#endif
	if (vp->v_op == &lo_vnodeops) {
		vp = realvp(vp);
	}
	if (VOP_REALVP(vp, &rvp) == 0) {
		vp = rvp;
	}
	*vpp = vp;
#ifdef LODEBUG
	lo_dprint(lodebug, 4, "lo_realvp returning %x\n", vp);
#endif
	return (0);
}

int
lo_badop()
{

	return (EISDIR);
}

/*ARGSUSED*/
static int
lo_cntl(vp, cmd, idata, odata, iflg, oflg)
	struct vnode *vp;
	caddr_t idata, odata;
{
	return ENOSYS;
}
