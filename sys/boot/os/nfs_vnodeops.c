/*      @(#)nfs_vnodeops.c 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include "boot/vnode.h"
#include <sys/vfs.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include "boot/cmap.h"
#include <netinet/in.h>
#include <sys/proc.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/xdr.h>
#include "boot/nfs.h"
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>


#ifdef	NFSDEBUG
static int nfsdebug = 10;
#endif	/* NFSDEBUG */

struct vnode *makenfsnode();
char *newname();

#define RNOCACHE 0x20		/* don't cache read and write blocks */

#define check_stale_fh(errno, vp) if ((errno) == ESTALE) { dnlc_purge_vp(vp); }
#define nfsattr_inval(vp)       (vtor(vp)->r_attrtime.tv_sec = 0)

#define ISVDEV(t) ((t == VBLK) || (t == VCHR) || (t == VFIFO))

#define rlock(rp) { \
	while ((rp)->r_flags & RLOCKED) { \
		(rp)->r_flags |= RWANT; \
		(void) sleep((caddr_t)(rp), PINOD); \
	} \
	(rp)->r_flags |= RLOCKED; \
}

#define runlock(rp) { \
	(rp)->r_flags &= ~RLOCKED; \
	if ((rp)->r_flags&RWANT) { \
		(rp)->r_flags &= ~RWANT; \
		wakeup((caddr_t)(rp)); \
	} \
}

#undef u
extern	struct user u;

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 *
 * Note on directory name lookup cacheing:  we desire that all operations
 * on a given client machine come out the same with or without the cache.
 * This is the same property we have with the disk buffer cache.  In order
 * to guarantee this, we serialize all operations on a given directory,
 * by using rlock and runlock around rfscalls to the server.  This way,
 * we cannot get into races with ourself that would cause invalid information
 * in the cache.  Other clients (or the server itself) can cause our
 * cached information to become invalid, the same as with data buffers.
 * Also, if we do detect a stale fhandle, we purge the directory cache
 * relative to that vnode.  This way, the user won't get burned by the
 * cache repeatedly.
 */


int nfs_open();
int nfs_close();
int nfs_rdwr();
int nfs_ioctl();
int nfs_select();
int nfs_getattr();
int nfs_access();
int nfs_lookup();
int nfs_readdir();
int nfs_readlink();
int nfs_inactive();
int nfs_bmap();
int nfs_strategy();
int nfs_badop();
int nfs_noop();

struct vnodeops nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_rdwr,
	nfs_ioctl,
	nfs_select,
	nfs_getattr,
	nfs_badop,	/* nfs_setattr */
	nfs_access,
	nfs_lookup,
	nfs_badop,	/* nfs_create */
	nfs_badop,	/* nfs_remove */
	nfs_badop,	/* nfs_link */
	nfs_badop,	/* nfs_rename */
	nfs_badop,	/* nfs_mkdir */
	nfs_badop,	/* nfs_rmdir */
	nfs_readdir,
	nfs_badop,	/* nfs_symlink */
	nfs_readlink,
	nfs_badop,	/* nfs_fsync */
	nfs_inactive,
	nfs_bmap,
	nfs_strategy,
	nfs_badop,
	nfs_badop,
	nfs_badop,	/* nfs_locktl */
	nfs_noop,
};

/*ARGSUSED*/
int
nfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	struct vattr va;
	int error;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "nfs_open %s %x flag %d\n",
	    vtomi(*vpp)->mi_hostname, *vpp, flag);
#endif

	/*
	 * validate cached data by getting the attributes from the server
	 */
	nfsattr_inval(*vpp);
	error = nfs_getattr(*vpp, &va, cred);
	return (error);

}

/*ARGSUSED*/
int
nfs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "nfs_close %s %x flag %d\n",
	    vtomi(vp)->mi_hostname, vp, flag);
#endif
	return(EACCES);
}

int
nfs_rdwr(vp, uiop, rw, ioflag, cred)
	register struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	int error = 0;
	struct rnode *rp;

#ifdef NFSDEBUG1
	dprint(nfsdebug, 6, "nfs_rdwr: %s %x rw %s offset %x len %d\n",
	    vtomi(vp)->mi_hostname, vp, rw == UIO_READ ? "READ" : "WRITE",
	    uiop->uio_offset, uiop->uio_iov->iov_len);
#endif

	if (vp->v_type != VREG) {
		return (EISDIR);
	}

	if (rw == UIO_WRITE || (rw == UIO_READ && vtor(vp)->r_cred == NULL)) {
		crhold(cred);
		if (vtor(vp)->r_cred) {
			crfree(vtor(vp)->r_cred);
		}
		vtor(vp)->r_cred = cred;
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		struct vattr va;

		rp = vtor(vp);
		rlock(rp);
		error = VOP_GETATTR(vp, &va, cred);
		if (!error) {
			uiop->uio_offset = rp->r_size;
		}
	}

	if (!error) {
		error = rwvp(vp, uiop, rw, cred);
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		runlock(rp);
	}
#ifdef NFSDEBUG1
	dprint(nfsdebug, 6, "nfs_rdwr returning %d\n", error);
#endif	/* NFSDEBUG */
	return (error);

}

int
rwvp(vp, uio, rw, cred)
	register struct vnode *vp;
	register struct uio *uio;
	enum uio_rw rw;
	struct ucred *cred;
{
	struct buf *bp;
	struct rnode *rp;
	daddr_t bn;
	register int n, on;
	int size;
	int error = 0;
	struct vnode *mapped_vp;
	daddr_t mapped_bn;
	int eof = 0;

#ifdef NFSDEBUG1
	dprint(nfsdebug, 6, "rwvp(vp 0x%x uio 0x%x rw 0x%x cred 0x%x)\n",
		vp, uio, rw, cred);
#endif	/* NFSDEBUG */


	if (uio->uio_resid == 0) {
		return (0);
	}
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		return (EINVAL);
	}
#ifdef notdef
	/*
	 * Soooo, why wasn't this just removed 
	 */
	if (rw == UIO_WRITE && vp->v_type == VREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
#endif notdef
	rp = vtor(vp);
	/*
	 * For now, ensure no cache.
	 */
	rp->r_flags |= RNOCACHE;
	size = vtoblksz(vp);
	size &= ~(DEV_BSIZE - 1);
	if (size <= 0) {
		panic("rwvp: zero size");
	}

	do {

		bn = uio->uio_offset / size;
		on = uio->uio_offset % size;
		n = MIN((unsigned)(size - on), uio->uio_resid);
		VOP_BMAP(vp, bn, &mapped_vp, &mapped_bn);

		if (rp->r_flags & RNOCACHE) {
			bp = geteblk(size);
			if (rw == UIO_READ) {
				error = nfsread(vp, bp->b_un.b_addr+on,
				    (int)uio->uio_offset, n, (int *)&bp->b_resid,
					 cred);
				if (error) {
					brelse(bp);
					goto bad;
				}
			}
		}
		if (bp->b_flags & B_ERROR) {
			error = geterror(bp);
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			brelse(bp);
		} else {
		}

	} while (u.u_error == 0 && uio->uio_resid > 0 && !eof);
	if (error == 0)				/* XXX */ 
		error = u.u_error;		/* XXX */
 
bad:
	return (error);
}

/*
 * Write to file.
 * Writes to remote server in largest size chunks that the server can
 * handle.  Write is synchronous.
 */
/*ARGSUSED*/
nfswrite(vp, base, offset, count, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	struct ucred *cred;
{
	int error;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfswrite %s %x offset = %d, count = %d\n",
	    vtomi(vp)->mi_hostname, vp, offset, count);
#endif
	error = EACCES;
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfswrite: returning %d\n", error);
#endif
	return (error);
}

/*
 * Read from a file.
 * Reads data in largest chunks our interface can handle
 */
nfsread(vp, base, offset, count, residp, cred)
	struct vnode *vp;
	caddr_t base;
	int offset;
	int count;
	int *residp;
	struct ucred *cred;
{
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;
	register int tsize;

#ifdef NFSDEBUG1
	dprint(nfsdebug, 4, "nfsread %s %x offset = %d, totcount = %d\n",
	    vtomi(vp)->mi_hostname, vp, offset, count);
#endif	/* NFSDEBUG */

	do {
		tsize = MIN(vtomi(vp)->mi_tsize, count);
		rr.rr_data = base;
		ra.ra_fhandle = *vtofh(vp);
		ra.ra_offset = offset;
		ra.ra_totcount = tsize;
		ra.ra_count = tsize;
		error = rfscall(vtomi(vp), RFS_READ, xdr_readargs, (caddr_t)&ra,
			xdr_rdresult, (caddr_t)&rr, cred);
		if (!error) {
			error = geterrno(rr.rr_status);
			check_stale_fh(error, vp);
		}
		if (!error) {
			count -= rr.rr_count;
			base += rr.rr_count;
			offset += rr.rr_count;
		}
	} while (!error && count && rr.rr_count == tsize);

	*residp = count;

	if (!error) {
		nfs_attrcache(vp, &rr.rr_attr);
	}

#ifdef NFSDEBUG1
	dprint(nfsdebug, 5, "nfsread: returning %d, resid %d\n",
		error, *residp);
#endif
	return (error);
}

/*ARGSUSED*/
int
nfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{

	return (EOPNOTSUPP);
}

/*ARGSUSED*/
int
nfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

	return (EOPNOTSUPP);
}


/*
 * Timeout values for attributes for
 * regular files, and for directories.
 */
int nfsac_regtimeo_min = 3;
int nfsac_regtimeo_max = 60;
int nfsac_dirtimeo_min = 30;
int nfsac_dirtimeo_max = 60;

/*ARGSUSED*/
nfs_attrcache(vp, na)
	struct vnode *vp;
	struct nfsfattr *na;
{
#ifdef	NFSDEBUG1
	dprint(nfsdebug, 6,
		"nfs_attrcache(vp 0x%x na 0x%x)\n",
		vp, na);
#endif	/* NFSDEBUG */

}

int
nfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	int error;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_getattr %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif
	sync_vp(vp);    /* sync blocks so mod time is right */
	error = nfsgetattr(vp, vap, cred);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_getattr: returns %d\n", error);
#endif
	return (error);
}

int
nfsgetattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	int error = 0;
	struct nfsattrstat *ns;
	struct rnode *rp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"nfsgetattr(vp 0x%x vap 0x%x cred 0x%x)\n",
		vp, vap, cred);
#endif	/* NFSDEBUG */

	rp = vtor(vp);
	if (timercmp(&time, &rp->r_attrtime, <)) {
		/*
		 * Use cached attributes.
		 */
		rp = vtor(vp);
		nattr_to_vattr(&rp->r_attr, vap);
		vap->va_fsid = 0xff00 | vtomi(vp)->mi_mntno;
		if (rp->r_size < vap->va_size || ((rp->r_flags & RDIRTY) == 0)){
			rp->r_size = vap->va_size;
		} else if (vap->va_size < rp->r_size && (rp->r_flags & RDIRTY)){
			vap->va_size = rp->r_size;
		}
	} else {
		ns = (struct nfsattrstat *)kmem_alloc((u_int)sizeof(*ns));
		error = rfscall(vtomi(vp), RFS_GETATTR, xdr_fhandle,
		    (caddr_t)vtofh(vp), xdr_attrstat, (caddr_t)ns, cred);
		if (!error) {
			error = geterrno(ns->ns_status);
			if (!error) {
				nattr_to_vattr(&ns->ns_attr, vap);
				/*
				 * this is a kludge to make programs that use
				 * dev from stat to tell file systems apart
				 * happy.  we kludge up a dev from the mount
				 * number and an arbitrary major number 255.
				 */
				vap->va_fsid = 0xff00 | vtomi(vp)->mi_mntno;
				if (rp->r_size < vap->va_size ||
				    ((rp->r_flags & RDIRTY) == 0)){
					rp->r_size = vap->va_size;
				} else if ((vap->va_size < rp->r_size) &&
				    (rp->r_flags & RDIRTY)) {
					vap->va_size = rp->r_size;
				}
				nfs_attrcache(vp, &ns->ns_attr);
			} else {
				check_stale_fh(error, vp);
			}
		}
		kmem_free((caddr_t)ns, (u_int)sizeof(*ns));
	}
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"nfsgetattr: returning 0x%x\n", error);
#endif	/* NFSDEBUG */
	return (error);


}

/*ARGSUSED*/
int
nfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct vattr va;
#ifdef NEVER
	int *gp;
#endif NEVER

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_access %s %x mode %d uid %d\n",
	    vtomi(vp)->mi_hostname, vp, mode, cred->cr_uid);
#endif


	u.u_error = nfsgetattr(vp, &va, cred);
	if (u.u_error) {
		return (u.u_error);
	}

	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (cred->cr_uid == 0)
		return (0);
#ifdef	NEVER
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
#endif  /* NEVER */
	if ((va.va_mode & mode) == mode) {
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_access: OK\n");
#endif	/* NFSDEBUG */
		return (0);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_access: returning %d\n", u.u_error);
#endif /* NFSDEBUG */
	u.u_error = EACCES;
	return (EACCES);
}

int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	int error;
	struct nfsrdlnres rl;

#ifdef NFSDEBUG
	dprint(nfsdebug, 6,
		"nfs_readlink %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif

	if(vp->v_type != VLNK)
		return (ENXIO);
	rl.rl_data = (char *)kmem_alloc((u_int)NFS_MAXPATHLEN);
	error =
	    rfscall(vtomi(vp), RFS_READLINK, xdr_fhandle, (caddr_t)vtofh(vp),
	      xdr_rdlnres, (caddr_t)&rl, cred);
	if (!error) {
		error = geterrno(rl.rl_status);
		if (!error) {
			error = uiomove(rl.rl_data, (int)rl.rl_count,
			    UIO_READ, uiop);
		} else {
			check_stale_fh(error, vp);
		}
	}
	kmem_free((caddr_t)rl.rl_data, (u_int)NFS_MAXPATHLEN);


#ifdef NFSDEBUG
	dprint(nfsdebug, 6,
		"nfs_readlink: returning %d\n", error);
#endif
	return (error);
}

/*ARGSUSED*/
static
sync_vp(vp)
	struct vnode *vp;
{
}

/*
 * Weirdness: if the file was removed while it was open it got
 * renamed (by nfs_remove) instead.  Here we remove the renamed
 * file.
 */
/*ARGSUSED*/
int
nfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	register struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_inactive '%s' 0x%x\n",
	     vtomi(vp)->mi_hostname,  vp);
#endif


	rp = vtor(vp);
	/*
	 * Pull rnode off of the hash list so it won't be found
	 */
	runsave(rp);
 
	if (rp->r_unldvp != NULL) {
		rlock(vtor(rp->r_unldvp));
		setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
		error = rfscall( vtomi(rp->r_unldvp), RFS_REMOVE,
		    xdr_diropargs, (caddr_t)&da,
		    xdr_enum, (caddr_t)&status, rp->r_unlcred);
 
		if (!error) {
			error = geterrno(status);
 
		}
		runlock(vtor(rp->r_unldvp));
		VN_RELE(rp->r_unldvp);
		rp->r_unldvp = NULL;
		kmem_free((caddr_t)rp->r_unlname, (u_int)NFS_MAXNAMLEN);
		rp->r_unlname = NULL;
		crfree(rp->r_unlcred);
		rp->r_unlcred = NULL;
	}
	((struct mntinfo *)vp->v_vfsp->vfs_data)->mi_refct--;
	if (rp->r_cred) {
		crfree(rp->r_cred);
		rp->r_cred = NULL;
	}
	rfree(rp);


#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_inactive done\n");
#endif
	return (0);
}

int nfs_dnlc = 1;	/* use dnlc */
/*
 * Remote file system operations having to do with directory manipulation.
 */

nfs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	struct  nfsdiropres *dr;
	struct vattr va;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_lookup %s %x '%s'\n",
	    vtomi(dvp)->mi_hostname, dvp, nm);
#endif

 
	/*
	 * Before checking dnlc, call getattr to be
	 * sure directory hasn't changed.  getattr
	 * will purge dnlc if a change has occurred.
	 */
	if (error = nfs_getattr(dvp, &va, cred)) {
		return (error);
	}
	rlock(vtor(dvp));
	*vpp = (struct vnode *)0;
	if (*vpp) {
		VN_HOLD(*vpp);
	} else {
		dr = (struct  nfsdiropres *)kmem_alloc((u_int)sizeof(*dr));
		setdiropargs(&da, nm, dvp);
 
		error = rfscall(vtomi(dvp), RFS_LOOKUP, xdr_diropargs,
		    (caddr_t)&da, xdr_diropres, (caddr_t)dr, cred);
		if (!error) {
			error = geterrno(dr->dr_status);
			check_stale_fh(error, dvp);
		}
		if (!error) {
			*vpp = makenfsnode(&dr->dr_fhandle,
			    &dr->dr_attr, dvp->v_vfsp);
		} else {
			*vpp = (struct vnode *)0;
		}
 
		kmem_free((caddr_t)dr, (u_int)sizeof(*dr));
	}
	runlock(vtor(dvp));


#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_lookup returning %d vp = %x\n", error, *vpp);
#endif
	return (error);
}

/*
 * Read directory entries.
 * There are some weird things to look out for here.  The uio_offset
 * field is either 0 or it is the offset returned from a previous
 * readdir.  It is an opaque value used by the server to find the
 * correct directory block to read.  The byte count must be at least
 * vtoblksz(vp) bytes.  The count field is the number of blocks to
 * read on the server.  This is advisory only, the server may return
 * only one block's worth of entries.  Entries may be compressed on
 * the server.
 */
nfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	int error = 0;
	struct iovec *iovp;
	unsigned count;
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;
	struct rnode *rp;
#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "nfs_readdir(vp 0x%x uiop 0x%x cred 0x%x)\n",
	    vp, uiop, cred);
#endif

	rp = vtor(vp);
	if ((rp->r_flags & REOF) && (rp->r_size == (u_long)uiop->uio_offset)) {
		return (0);
	}
	iovp = uiop->uio_iov;
	count = iovp->iov_len;
#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "nfs_readdir %s %x count %d offset %d\n",
	    vtomi(vp)->mi_hostname, vp, count, uiop->uio_offset);
#endif
	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	count = MIN(count, vtomi(vp)->mi_tsize);
	rda.rda_count = count;
	rda.rda_offset = uiop->uio_offset;
	rda.rda_fh = *vtofh(vp);
	rd.rd_size = count;
	rd.rd_entries = (struct direct *)kmem_alloc((u_int)count);

	error = rfscall(vtomi(vp), RFS_READDIR, xdr_rddirargs, (caddr_t)&rda,
	    xdr_getrddirres, (caddr_t)&rd, cred);
	if (!error) {
		error = geterrno(rd.rd_status);
		check_stale_fh(error, vp);
	}
	if (!error) {
		/*
		 * move dir entries to user land
		 */
		if (rd.rd_size) {
			error = uiomove((caddr_t)rd.rd_entries,
			    (int)rd.rd_size, UIO_READ, uiop);
			rda.rda_offset = rd.rd_offset;
			uiop->uio_offset = rd.rd_offset;
		}
		if (rd.rd_eof) {
			rp->r_flags |= REOF;
			rp->r_size = uiop->uio_offset;
		}
	}
	kmem_free((caddr_t)rd.rd_entries, (u_int)count);
#ifdef NFSDEBUG
	dprint(nfsdebug, 6, "nfs_readdir: returning %d resid %d, offset %d\n",
	    error, uiop->uio_resid, uiop->uio_offset);
#endif
	return (error);
}

/*
 * Convert from file system blocks to device blocks
 */
int
nfs_bmap(vp, bn, vpp, bnp)
	struct vnode *vp;	/* file's vnode */
	daddr_t bn;		/* fs block number */
	struct vnode **vpp;	/* RETURN vp of device */
	daddr_t *bnp;		/* RETURN device block number */
{
	int bsize;		/* server's block size in bytes */

#ifdef NFSDEBUG1
	dprint(nfsdebug, 4, "nfs_bmap %s %x blk %d vpp 0x%x\n",
	    vtomi(vp)->mi_hostname, vp, bn, vpp);
#endif	/* NFSDEBUG */

	if (vpp)
		*vpp = vp;
	if (bnp) {
		bsize = vtoblksz(vp);
		*bnp = bn * (bsize / DEV_BSIZE);
	}
	return (0);
}

struct buf *async_bufhead;
int async_daemon_count;

int
nfs_strategy(bp)
	register struct buf *bp;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_strategy bp %x\n", bp);
#endif
}

async_daemon()
{
}

do_bio(bp)
	register struct buf *bp;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "do_bio: addr %x, blk %d, offset %d, size %d, B_READ %d\n",
	    bp->b_un.b_addr, bp->b_blkno, bp->b_blkno * DEV_BSIZE,
	    bp->b_bcount, bp->b_flags & B_READ);
#endif
}

int
nfs_badop()
{
#ifdef notdef
	panic("nfs_badop");
#endif
	return (0);
}

int
nfs_noop()
{
	return (EREMOTE);
}

dnlc_purge_vp(vp)
	register struct vnode *vp;
{
#ifdef	NFSDEBUG
        dprint(nfsdebug, 6, "dnlc_purge_vp(vp 0x%x)\n", vp);
#endif	/* NFSDEBUG */

}
