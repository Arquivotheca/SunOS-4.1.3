#ifndef lint
static	char sccsid[] = "@(#)pc_vnodeops.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/dirent.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_dir.h>
#include <pcfs/pc_node.h>
#include <sys/mman.h>
#include <sys/vmmeter.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <machine/seg_kmem.h>
#include <sys/unistd.h>

static int pcfs_open();
static int pcfs_close();
static int pcfs_rdwr();
static int pcfs_getattr();
static int pcfs_setattr();
static int pcfs_access();
static int pcfs_lookup();
static int pcfs_create();
static int pcfs_remove();
static int pcfs_rename();
static int pcfs_mkdir();
static int pcfs_rmdir();
static int pcfs_readdir();
static int pcfs_fsync();
static int pcfs_inactive();
static int pcfs_getpage();
static int pcfs_putpage();
static int pcfs_map();
static int pcfs_cmp();
static int pcfs_realvp();
#ifdef notdef
static int pcfs_fid();
#endif notdef
static int pcfs_cntl();
extern int pcfs_invalop();

/*
 * vnode op vectors for files, directories, and invalid files.
 */
struct vnodeops pcfs_fvnodeops = {
	pcfs_open,
	pcfs_close,
	pcfs_rdwr,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_getattr,
	pcfs_setattr,
	pcfs_access,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_fsync,
	pcfs_inactive,
	pcfs_invalop,
	pcfs_invalop,		/* VOP_FID */
	pcfs_getpage,
	pcfs_putpage,
	pcfs_map,
	pcfs_invalop,
	pcfs_cmp,
	pcfs_realvp,
	pcfs_cntl,
};

struct vnodeops pcfs_dvnodeops = {
	pcfs_open,
	pcfs_close,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_getattr,
	pcfs_invalop, /* can't we support this op? one problem is to */
	pcfs_access,  /* make attr of '.', '..' and the real dir the same.*/
	pcfs_lookup,
	pcfs_create,
	pcfs_remove,
	pcfs_invalop,
	pcfs_rename,
	pcfs_mkdir,
	pcfs_rmdir,
	pcfs_readdir,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_inactive,
	pcfs_invalop,
	pcfs_invalop,		/* VOP_FID */
	pcfs_getpage,
	pcfs_putpage,
	pcfs_invalop,
	pcfs_invalop,
	pcfs_cmp,
	pcfs_realvp,
	pcfs_cntl,
};


/*ARGSUSED*/
static int
pcfs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	return (0);
}

/*
 * files are sync'ed on close to keep floppy up to date
 */

/*ARGSUSED*/
static int
pcfs_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
static int
pcfs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct pcfs *fsp;
	register struct pcnode *pcp;
	int error;

	fsp = VFSTOPCFS(vp->v_vfsp);
	error = pc_lockfs(fsp);
	if (error)
		return (error);
	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (fsp);
		return (EIO);
	}
	if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
		/*
		 * in append mode start at end of file.
		 */
		uiop->uio_offset = pcp->pc_size;
	}
	error = rwpcp(pcp, uiop, rw, ioflag);
	pc_unlockfs(fsp);
	if (error) {
PCFSDEBUG(3)
printf("pcfs_rdwr: io error = %d\n", error);
	}
	return (error);
}

int
pc_bzero(pcp, off, len, flag)
	register struct pcnode *pcp;
	u_int off, len, flag;
{
	u_int o;
	int err;
	u_int zlen;
	addr_t addr;
	struct vnode *vp = PCTOV(pcp);

	for (; len > 0; off += MAXBSIZE, len -= zlen) {
		o = off & MAXBOFFSET;
		addr = segmap_getmap(segkmap, vp, off & MAXBMASK);
		zlen = MIN(MAXBSIZE - o, len);
		if (o) {
			off = off & MAXBMASK;
			err = as_fault(&kas, addr + o, zlen,
						F_SOFTLOCK, S_OTHER);
			if (err) {
				(void) segmap_release(segkmap, addr, 0);
				if (FC_CODE(err) == FC_OBJERR)
					return (FC_ERRNO(err));
				else
					return (EIO);
			}
		} else {
			segmap_pagecreate(segkmap, addr + o, zlen, 1);
		}
		(void) bzero(addr + o, zlen);
		(void) as_fault(&kas, addr + o, zlen, F_SOFTUNLOCK, S_WRITE);
		if (flag & IO_SYNC) {
			(void) segmap_release(segkmap, addr, SM_WRITE);
		} else {
			(void) segmap_release(segkmap, addr, 0);
		}
	}
	return (0);

}


static int
rwpcp(pcp, uio, rw, ioflag)
	register struct pcnode *pcp;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	struct vnode *vp;
	struct pcfs *fsp;
	daddr_t lbn;			/* logical block number */
	daddr_t bn;			/* phys block number */
	register int n;
	register u_int off;
	addr_t base;
	int mapon, bsize, pagecreate;
	int error = 0;
	u_int size;

PCFSDEBUG(6)
printf("rwpcp pcp=%x off=%d resid=%d rw=%d size=%d\n",
pcp, uio->uio_offset, uio->uio_resid, rw, pcp->pc_size);

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwpcp");
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0)
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);
	if (rw == UIO_WRITE &&
	    uio->uio_offset + uio->uio_resid >
	    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
	}
#ifdef notdef
	if ((ioflag & IOSYNC) && !pc_verify(pcp))
		return (EIO);
#endif notdef
	vp = PCTOV(pcp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	bsize = fsp->pcfs_clsize;
	size = roundup(pcp->pc_size, fsp->pcfs_clsize);
	do {
		off = uio->uio_offset & MAXBMASK;
		mapon = uio->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - mapon, uio->uio_resid);
		lbn = (uio->uio_offset + n -1)/ bsize;
		if (rw == UIO_READ) {
			int diff;

			diff = pcp->pc_size - uio->uio_offset;
			if (diff <= 0)
				return (0);
			if (diff < n)
				n = diff;
		}
		base = segmap_getmap(segkmap, vp, off);
		pagecreate = 0;
		if (rw == UIO_WRITE) {
			if (uio->uio_offset + n > size) {
				error = pc_balloc(pcp, lbn, &bn);
				if (error) {
					if (error == ENOSPC) {
					/*
					 * Because the page size is many times
					 * the DOS blk size (clsize), we may
					 * have allocated a few clusters before
					 * we run out of space. So we have to
					 * return those clusters here.
					 */
						int save;
						save = pcp->pc_size;
						pcp->pc_size =
						    uio->uio_offset + n;
						(void) pc_bfree(pcp, (short)
						    (size/fsp->pcfs_clsize));
						pcp->pc_size = save;
					}
					(void) segmap_release(segkmap, base, 0);
PCFSDEBUG (2)
printf("rwpcp error1=%d\n", error);
					return (error);
				}
				if (mapon == 0) {
					segmap_pagecreate(segkmap, base,
						(u_int)n, 0);
					pagecreate = 1;
				}
				if (size < uio->uio_offset) {
				/*
				 * Has to zero out date between size and
				 * uio_offset
				 */
					pcp->pc_size = uio->uio_offset + n;
					error = pc_bzero(pcp, size,
						(u_int) uio->uio_offset-size,
						(u_int) ioflag);
					if (error) {
						(void) segmap_release(segkmap,
							base, 0);
						(void) pc_bfree(pcp, (short)
						    (size/fsp->pcfs_clsize));
						pcp->pc_size = size;
						return (error);
					}
					pagecreate = 1;
				}

			} else if (n == MAXBSIZE) {
				segmap_pagecreate(segkmap, base, (u_int)n, 0);
				pagecreate = 1;
			}
			if (uio->uio_offset + n > pcp->pc_size) {
				size = uio->uio_offset + n;
				pcp->pc_size = size;
			}
		}
		error = uiomove(base + mapon, n, rw, uio);

		if (pagecreate && uio->uio_offset <
			roundup(off + mapon + n, fsp->pcfs_clsize)) {
			int nzero, nmoved;

			nmoved = uio->uio_offset - (off + mapon);
			nzero = roundup(mapon + n, fsp->pcfs_clsize) - nmoved;
			(void) kzero(base + mapon + nmoved, (u_int)nzero);
		}

		if (error == 0) {
			u_int flags = 0;

			if (rw == UIO_READ) {
				if (n + mapon == MAXBSIZE ||
				    uio->uio_offset == pcp->pc_size)
					flags = SM_DONTNEED;
			} else if (rw == UIO_WRITE) {
				if ((ioflag & IO_SYNC)
					/* %% || type == IFDIR */)
				{
					flags = SM_WRITE;
				} else if (n + mapon == MAXBSIZE) {
					flags = SM_WRITE|SM_ASYNC|SM_DONTNEED;
				}
			}

			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}
		if (rw == UIO_WRITE)
			pc_mark(pcp);

	} while (error == 0 && uio->uio_resid > 0 && n != 0);

#ifdef notdef
	if ((ioflag & IO_SYNC) && (rw == UIO_WRITE) && (pcp->pc_flags & PC_MOD))
		pc_nodeupdate(pcp);
#endif notdef

	return (error);
}


/*ARGSUSED*/
static int
pcfs_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct pcnode *pcp;
	register struct pcfs *fsp;
	int error;
	char attr;

	fsp = VFSTOPCFS(vp->v_vfsp);
	error = pc_lockfs(fsp);
	if (error)
		return (error);
	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (fsp);
		return (EIO);
	}
	/*
	 * Copy from pcnode.
	 */
	vap->va_type = vp->v_type;
	attr = pcp->pc_entry.pcd_attr;
	if (attr & (PCA_HIDDEN|PCA_SYSTEM))
		vap->va_mode = 0;
	else if (attr & PCA_RDONLY)
		vap->va_mode = 0555;
	else
		vap->va_mode = 0777;
	if (attr & PCA_DIR)
		vap->va_mode |= S_IFDIR;
	else
		vap->va_mode |= S_IFREG;
	vap->va_uid = u.u_uid;
	vap->va_gid = u.u_gid;
	vap->va_fsid = fsp->pcfs_devvp->v_rdev;
	vap->va_nodeid =
	    pc_makenodeid(pcp->pc_eblkno, pcp->pc_eoffset, &pcp->pc_entry);
	vap->va_nlink = 1;
	vap->va_size = pcp->pc_size;
	pc_pcttotv(&pcp->pc_entry.pcd_mtime, &vap->va_mtime);
	vap->va_ctime = vap->va_atime = vap->va_mtime;
	vap->va_rdev = -1;
	vap->va_blocks = howmany(pcp->pc_size, DEV_BSIZE);
	vap->va_blocksize = fsp->pcfs_clsize;
	pc_unlockfs(fsp);
	return (0);
}


/*ARGSUSED*/
static int
pcfs_setattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct pcnode *pcp;
	int error, error2;

	/*
	 * cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    (vap->va_uid != (uid_t)-1) || (vap->va_gid != (uid_t)-1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}

	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (VFSTOPCFS (vp -> v_vfsp));
		return (EIO);
	}
	/*
	 * Change file access modes.
	 * If nobody has write permision, file is marked readonly.
	 * Otherwise file is writeable by anyone.
	 */
	if (vap->va_mode != (u_short)-1) {
		if ((vap->va_mode & 0222) == 0)
			pcp->pc_entry.pcd_attr |= PCA_RDONLY;
		else
			pcp->pc_entry.pcd_attr &= ~PCA_RDONLY;
		pcp->pc_flags |= PC_CHG;
	}
	/*
	 * Truncate file. Must have write permission and not be a directory.
	 */
	if (vap->va_size != (u_long)-1) {
		if (pcp->pc_entry.pcd_attr & PCA_DIR) {
			error = EISDIR;
			goto out;
		}
		if (pcp->pc_entry.pcd_attr & PCA_RDONLY) {
			error = EACCES;
			goto out;
		}
		error = pc_truncate(pcp, (long)vap->va_size);
		if (error)
			goto out;
	}
	/*
	 * Change file modified times.
	 */
	if (vap->va_mtime.tv_sec != -1) {
		/*
		 * If SysV-compatible option to set access and
		 * modified times if root, owner, or write access,
		 * use current time rather than va_mtime.
		 *
		 * XXX - va_mtime.tv_usec == -1 flags this.
		 */
		pc_tvtopct((vap->va_mtime.tv_usec == -1) ? &time :
		    &vap->va_mtime, &pcp->pc_entry.pcd_mtime);
		pcp->pc_flags |= PC_CHG;
	}
out:
	error2 = pc_nodesync (pcp);
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return ((error2) ? error2 : error);
}


/*ARGSUSED*/
static int
pcfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct pcnode * pcp;

	if ((pcp = VTOPC (vp)) == NULL)
		return (EIO);
	if ((mode & VWRITE) && (pcp -> pc_entry . pcd_attr & PCA_RDONLY))
		return (EACCES);
	else
		return (0);
}


/*ARGSUSED*/
static int
pcfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct pcnode * pcp;
	int error;

	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (VFSTOPCFS (vp -> v_vfsp));
		return (EIO);
	}
	error = pc_nodesync (pcp);
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return (error);
}


/*ARGSUSED*/
static int
pcfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct pcnode *pcp;
	struct pcfs * fsp;

	/*
	 * Here's the scoop. V_count is 0 while we're waiting to lock the
	 * file system. This means that if any other process operates on
	 * this node while we're waiting it can rerelease it. So we
	 * mark this node as release pending.
	 */
	if (vp->v_count)
		return (0);

	if ((pcp = VTOPC (vp)) != NULL) {
		if (pcp -> pc_flags & PC_RELE_PEND)
			return (0);
		pcp->pc_flags |= PC_RELE_PEND;
	}

	fsp = VFSTOPCFS (vp -> v_vfsp);
	PC_LOCKFS (fsp);

	/*
	 * Check again to confirm that no intervening I/O error
	 * with a subsequent pc_diskchanged() call has released
	 * the pcnode.  If it has then release the vnode as above.
	 */
	if ((pcp = VTOPC (vp)) != NULL)
		pcp->pc_flags &= ~PC_RELE_PEND;

	if (! vp->v_count) {
		if (pcp == NULL) {
			kmem_free((caddr_t) vp, sizeof (struct vnode));
		} else {
			pc_rele (pcp);
		}
	}
	PC_UNLOCKFS (fsp);
	return (0);
}

/*ARGSUSED*/
static int
pcfs_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	struct pcnode *pcp;
	register int error;

	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	if (VTOPC(dvp) == NULL) {
		pc_unlockfs (VFSTOPCFS (dvp->v_vfsp));
		return (EIO);
	}
	error = pc_dirlook(VTOPC(dvp), nm, &pcp);
	if (!error) {
		*vpp = PCTOV(pcp);
		pcp->pc_flags |= PC_EXTERNAL;
	}
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (error);
}


/*ARGSUSED*/
static int
pcfs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	struct pcnode *pcp;
	struct vnode * vp;

	/*
	 * can't create directories. use pcfs_mkdir.
	 */
	if (vap->va_type == VDIR)
		return (EISDIR);
	pcp = (struct pcnode *) 0;
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	if (VTOPC(dvp) == NULL) {
		pc_unlockfs (VFSTOPCFS (dvp -> v_vfsp));
		return (EIO);
	}
	error = pc_direnter(VTOPC(dvp), nm, vap, &pcp);
	/*
	 * if file exists and this is a nonexclusive create,
	 * check for access permissions
	 */
	if (error == EEXIST) {
		vp = PCTOV (pcp);
		if (exclusive == NONEXCL) {
			if (vp -> v_type == VDIR) {
				error = EISDIR;
			} else if (mode) {
				error = pcfs_access(PCTOV(pcp), mode,
					(struct ucred *) 0);
			} else {
				error = 0;
			}
		}
		if (error) {
#ifdef notdef
			if (vp->v_count == 1) {
				vp->v_count = 0;
				if (!(pcp->pc_flags & PC_RELE_PEND))
					pc_rele(pcp);
			} else
#endif notdef
				VN_RELE(PCTOV(pcp));

		}
	}
	if (error) {
		pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
		return (error);
	}
	/*
	 * truncate regular files, if required
	 */
	if ((vp->v_type == VREG) && (vap->va_size == 0)) {
		error = pc_truncate(pcp, 0L);
		if (error) {
			VN_RELE(PCTOV(pcp));
			pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
			return (error);
		}
		if (error = pc_nodesync(pcp)) {
			pc_unlockfs (VFSTOPCFS (dvp->v_vfsp));
			return (error);
		}
	}
	*vpp = PCTOV(pcp);
	pcp->pc_flags |= PC_EXTERNAL;
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (0);
}

/*ARGSUSED*/
static int
pcfs_remove(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;
	struct pcnode * pcp;

	error = pc_lockfs(VFSTOPCFS(vp->v_vfsp));
	if (error)
		return (error);
	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (VFSTOPCFS (vp -> v_vfsp));
		return (EIO);
	}
	error = pc_dirremove (pcp, nm, VREG);
	pc_unlockfs(VFSTOPCFS(vp->v_vfsp));
	return (error);
}

/*
 * Rename a file or directory
 * This rename is restricted to only rename files within a directory.
 * XX should make rename more general
 */
/*ARGSUSED*/
static int
pcfs_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct ucred *cred;
{
	register struct pcnode *dp;	/* parent pcnode */
	register int error;

	if (((dp = VTOPC(sdvp)) == NULL) || (VTOPC (tdvp) == NULL)) {
		return (EIO);
	}
	/*
	 * make sure source and target directories are the same
	 */
	if (dp->pc_scluster != VTOPC(tdvp)->pc_scluster)
		return (EXDEV);		/* XXX */
	/*
	 * make sure we can muck with this directory.
	 */
	error = pcfs_access(sdvp, VWRITE, (struct ucred *) 0);
	if (error) {
		return (error);
	}
	error = pc_lockfs (VFSTOPCFS (sdvp -> v_vfsp));
	if (error)
		return (error);
	if ((VTOPC(sdvp) == NULL) || (VTOPC (tdvp) == NULL)) {
		pc_unlockfs (VFSTOPCFS (sdvp -> v_vfsp));
		return (EIO);
	}
	error = pc_rename(dp, snm, tnm);
	pc_unlockfs (VFSTOPCFS (sdvp -> v_vfsp));
	return (error);
}

/*ARGSUSED*/
static int
pcfs_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;
{
	struct pcnode *pcp;
	register int error;

	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	if (VTOPC (dvp) == NULL) {
		pc_unlockfs (VFSTOPCFS (dvp -> v_vfsp));
		return (EIO);
	}
	error = pc_direnter(VTOPC(dvp), nm, vap, &pcp);

	if (!error) {
		pcp -> pc_flags |= PC_EXTERNAL;
		*vpp = PCTOV(pcp);
	} else if (error == EEXIST) {
		VN_RELE(PCTOV(pcp));
	}
	pc_unlockfs (VFSTOPCFS (dvp -> v_vfsp));
	return (error);
}

/*ARGSUSED*/
static int
pcfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	struct pcfs * fsp;
	struct pcnode * pcp;
	register int error;

	fsp = VFSTOPCFS (dvp -> v_vfsp);
	if (error = pc_lockfs (fsp))
		return (error);

	if ((pcp = VTOPC (dvp)) == NULL) {
		pc_unlockfs (fsp);
		return (EIO);
	}
	error = pc_dirremove (pcp, nm, VDIR);
	pc_unlockfs (fsp);
	return (error);
}

/*
 * read entries in a directory.
 * we must convert pc format to unix format
 */

/*ARGSUSED*/
static int
pcfs_readdir(dvp, uiop, cred)
	struct vnode *dvp;
	register struct uio *uiop;
	struct ucred *cred;
{
	register char *fp, *tp;
	register struct pcnode *pcp;
	struct pcdir *ep;
	struct buf *bp = NULL;
	register char c;
	register int i;
	long offset;
	register int n;
	register int boff;
	struct dirent d;
	int error, direntsiz;
	char *strcpy();

	direntsiz = sizeof (struct dirent) - MAXNAMLEN - 1 +
		roundup((PCFNAMESIZE + PCFEXTSIZE + 2), sizeof (int));

	if ((uiop->uio_iovcnt != 1) || (uiop->uio_offset % direntsiz))
		return (EINVAL);
	error = pc_lockfs(VFSTOPCFS(dvp->v_vfsp));
	if (error)
		return (error);
	if ((pcp = VTOPC(dvp)) == NULL) {
		pc_unlockfs (VFSTOPCFS (dvp->v_vfsp));
		return (EIO);
	}
	/*
	 * verify that the dp is still valid on the disk
	 */
	(void) pc_verify(pcp);
	n = uiop->uio_resid / direntsiz;
	offset = (uiop->uio_offset / direntsiz) * sizeof (struct pcdir);
	d.d_reclen = direntsiz;

	if (dvp->v_flag & VROOT) {
		/*
		 * kludge up entries for "." and ".." in the root.
		 */
		if (offset == 0 && n) {
			d.d_off = uiop->uio_offset + direntsiz;
			d.d_fileno = -1;
			d.d_namlen = 1;
			(void) strcpy(d.d_name, ".");
			(void) uiomove((caddr_t)&d, direntsiz, UIO_READ, uiop);
			offset = sizeof (struct pcdir);
			n--;
		}
		if (offset == sizeof (struct pcdir) && n) {
			d.d_off = uiop->uio_offset + direntsiz;
			d.d_fileno = -1;
			d.d_namlen = 2;
			(void) strcpy(d.d_name, "..");
			(void) uiomove((caddr_t)&d, direntsiz, UIO_READ, uiop);
			offset = 2 * sizeof (struct pcdir);
			n--;
		}
		offset -= 2 * sizeof (struct pcdir);
	}

	for (; n--; ep++, offset += sizeof (struct pcdir)) {
		boff = pc_blkoff(VFSTOPCFS(dvp->v_vfsp), offset);
		if (boff == 0 || bp == NULL || boff >= bp->b_bcount) {
			if (bp != NULL) {
				brelse(bp);
				bp = NULL;
			}
			error = pc_blkatoff(pcp, offset, &bp, &ep);
			if (error) {
				/*
				 * When is an error not an error?
				 */
				if (error == ENOENT)
					error = 0;
				break;
			}
		}
		if (ep->pcd_filename[0] == PCD_UNUSED)
			break;
		/*
		 * Don't display label because it may contain funny characters.
		 */
		if ((ep->pcd_filename[0] == PCD_ERASED) ||
		    (ep->pcd_attr & (PCA_HIDDEN | PCA_SYSTEM | PCA_LABEL))) {
			uiop->uio_offset += direntsiz;
			continue;
		}
		d.d_fileno = pc_makenodeid(bp->b_blkno, boff, ep);
		tp = &d.d_name[0];
		fp = &ep->pcd_filename[0];
		i = PCFNAMESIZE;
		while (i-- && ((c = *fp) != ' ')) {
			if (!(c == '.' || pc_validchar(c))) {
				tp = &d.d_name[0];
				break;
			}
			*tp++ = tolower(c);
			fp++;
		}
		fp = &ep->pcd_ext[0];
		if (tp != &d.d_name[0] && *fp != ' ') {
			*tp++ = '.';
			i = PCFEXTSIZE;
			while (i-- && ((c = *fp) != ' ')) {
				if (!pc_validchar(c)) {
					tp = &d.d_name[0];
					break;
				}
				*tp++ = tolower(c);
				fp++;
			}
		}
		*tp = 0;
		d.d_namlen = tp - &d.d_name[0];
		d.d_off = uiop->uio_offset + direntsiz;
		if (d.d_namlen) {
			(void) uiomove((caddr_t)&d, direntsiz, UIO_READ, uiop);
		} else {
			uiop->uio_offset += direntsiz;
		}
	}
	if (bp)
		brelse(bp);
	pc_unlockfs(VFSTOPCFS(dvp->v_vfsp));
	return (error);
}


/*
 * Called from pvn_getpages or pcfs_getpage to get a particular page.
 * When we are called the pcfs is already locked.  If rw == S_WRITE
 * and the block is not currently allocated we need to allocate the
 * needed block(s).
 *
 * bsize is either 4k or 8k.  To handle the case of 4k bsize and 8k pages
 * we will do two reads to get the data and don't bother with read ahead.
 * Thus having 4k file systems on a Sun-3 works, but it is not recommended.
 *
 * XXX - should handle arbritrary file system block and page sizes.
 */
/*ARGSUSED*/
static int
pcfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	register u_int off;
	u_int *protp;
	struct page *pl[];		/* NULL if async IO is requested */
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	register struct pcnode * pcp;
	register struct pcfs *fsp = VFSTOPCFS(vp->v_vfsp);
	struct vnode *devvp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn, bn;
	u_int io_off, io_len;
	u_int lbnoff, blksz;
	int i, err, nio;
	u_int pgoff;
	dev_t dev;

	if ((pcp = VTOPC (vp)) == NULL)
		return (EIO);

	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);

	devvp = fsp->pcfs_devvp;
	dev = devvp->v_rdev;

reread:
	pagefound = NULL;
	pgoff = i = 0;
	lbn = pc_lblkno(fsp, off);
	lbnoff = off & ~(fsp->pcfs_clsize -1);
	if (pl != NULL)
		pl[0] = NULL;

	err = 0;
	if ((pagefound = page_find(vp, off)) == NULL) {

		struct buf *bp;

		/*
		 * Need to really do disk IO to get the page(s).
		 */
		VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_MISS);

		blksz = pc_blksize(fsp, pcp, off);

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    lbnoff, blksz, 0);

		if (pp == NULL)
			panic("pcfs_getapage pvn_kluster");

		if (pl != NULL) {
			register int sz;

			if (plsz >= io_len) {
				/*
				 * Everything fits, set up to load
				 * up and hold all the pages.
				 */
				pp2 = pp;
				sz = io_len;
			} else {
				/*
				 * Set up to load plsz worth
				 * starting at the needed page.
				 */
				for (pp2 = pp; pp2->p_offset != off;
				    pp2 = pp2->p_next) {
				}
				sz = plsz;
			}

			ppp = pl;
			do {
				PAGE_HOLD(pp2);
				*ppp++ = pp2;
				pp2 = pp2->p_next;
				sz -= PAGESIZE;
			} while (sz > 0);
			*ppp = NULL;		/* terminate list */
		}

		nio = MIN(PAGESIZE/fsp->pcfs_clsize,
				howmany(pcp->pc_size-lbnoff, fsp->pcfs_clsize));
PCFSDEBUG(6)
printf("pc_getapage off=%d nio=%d\n", off, nio);
		if (nio > 1)
			pp->p_nio = nio;

		for (i = 0, pgoff = 0; i < nio; ++i, ++lbn) {
			/* ?? should also use read-ahead */
			err = pc_bmap(pcp, lbn, &bn, (daddr_t *) 0);

			if (err) {
				goto out;
			}
			blksz = pc_blksize(fsp, pcp, lbnoff);
			bp = pageio_setup(pp, blksz, devvp, pl == NULL ?
				    (B_ASYNC | B_READ) : B_READ);

			bp->b_dev = dev;
			bp->b_blkno = bn;
			bp->b_un.b_addr = (caddr_t)pgoff;

			(*bdevsw[major(dev)].d_strategy)(bp);

			lbnoff += blksz;
			pgoff += blksz;
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(blksz);
			if (pl != NULL) {
				if (err == 0)
					err = biowait(bp);
				else
					(void) biowait(bp);
				pageio_done(bp);
			}
		}
		if (pgoff != PAGESIZE) {
			pagezero(pp->p_prev, pgoff, PAGESIZE - pgoff);
		}
	} else {
PCFSDEBUG(6)
printf("pc_getapage page found, off=%d\n", off);
	}

out:
	if (pagefound != NULL) {
		int s;

		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is still intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		(void) splx(s);
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			goto reread;
		}
		PAGE_HOLD(pagefound);
		pl[0] = pagefound;
		pl[1] = NULL;
		u.u_ru.ru_minflt++;
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return (err);
}

/*
 * Return all the pages from [off..off+len) in given file
 */
static int
pcfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
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
	struct pcnode * pcp;
	register struct pcfs *fsp = VFSTOPCFS(vp->v_vfsp);
	int err;

	if ((pcp = VTOPC (vp)) == NULL)
		return (EIO);

	if (vp->v_type != VREG) {
PCFSDEBUG (2)
printf("pcfs_getpage doesn't support non-regular files now.\n");
		return (EINVAL);
	}

	/*
	 * Normally fail if faulting beyond EOF, *except* if this
	 * is an internal access of pcfs data.  This condition is
	 * detected by testing the faulting segment against segkmap.
	 * Since accessing the file through segkmap is only done
	 * in places in the kernel which have knowledge of the
	 * current file length, these places deal with EOF themselves.
	 * For example, bmap may be faulting in pages beyond the
	 * current EOF when it is creating pages needed for extending
	 * the length of the file.
	 */
	if (off + len > pcp->pc_size + PAGEOFFSET && seg != segkmap) {
		return (EFAULT);	/* beyond EOF */
	}
	if (protp != NULL)
		*protp = PROT_ALL;

	if (off & PAGEOFFSET) {
		printf("pcfs: off %d\n", off);
		panic("pcfs_getpage1");
	}

	if (off +len > roundup(pcp->pc_size, PAGESIZE)) { /* ?? */
		printf("pcfs: off %d pc_size %d\n", off, pcp->pc_size);
		panic("pcfs_getpage2");
	}

	err = pc_lockfs(fsp);
	if (err)
		return (err);

	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (fsp);
		return (EIO);
	}

	if (len <= PAGESIZE) {
		err = pcfs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	} else {
		err = pvn_getpages(pcfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cred);
	}

#ifdef notdef
	/*
	 * ??
	 * If the inode is not already marked for IACC (in rwip() for read)
	 * and the inode is not marked for no access time update (in rwip()
	 * for write) then update the inode access time and mod time now.
	 */

	if ((ip->i_flag & (IACC | INOACC)) == 0) {
		if (rw != S_OTHER) {
			ip->i_flag |= IACC;
		}
		if (rw == S_WRITE) {
			ip->i_flag |= IUPD;
		}
		ITIMES(ip);
	}
#endif notdef

	pc_unlockfs(fsp);

	return (err);
}


/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}
 * If len == 0, do from off to EOF.
 *
 * The normal cases should be len == 0 & off == 0 (entire vp list),
 * len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 * (from pageout).
 *
 */
/*ARGSUSED*/
static int
pcfs_putpage(vp, off, len, flags, cred)
	register struct vnode *vp;
	u_int off, len;
	int flags;
	struct ucred *cred;
{
	register struct pcnode *pcp;
	register struct page *pp;
	register struct pcfs *fsp;
	struct page *dirty;
	struct vnode *devvp;
	register u_int io_off;
	daddr_t lbn, bn;
	u_int lbn_off;
	u_int blksz, pgoff;
	int vpcount, nio, i;
	int err;
	dev_t dev;

#ifdef VFSSTATS
	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_CALL);
#endif

	if ((pcp = VTOPC (vp)) == NULL)
		return (EIO);

	if (vp->v_pages == NULL || off >= pcp->pc_size)
		return (0);

	vpcount = vp->v_count;
	VN_HOLD(vp);
	fsp = VFSTOPCFS(vp->v_vfsp);

	err = pc_lockfs(fsp);
	if (err)
		return (err);

	if ((pcp = VTOPC (vp)) == NULL) {
		pc_unlockfs (fsp);
		return (EIO);
	}

again:
	if (len == 0) {
		/*
		 * Search the entire vp list for pages >= off
		 */
		dirty = pvn_vplist_dirty(vp, off, flags);
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 */
		if (off >= pcp->pc_size) {
			dirty = NULL;
		} else {
			u_int fsize, eoff;

			fsize = (pcp->pc_size + MAXBOFFSET) & MAXBMASK;
			eoff = MIN(off + len, fsize);
			dirty = pvn_range_dirty(vp, off, eoff,
			    (u_int)(off), (u_int)(eoff), flags);
		}
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  All the pages on the pp list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	if ((vp->v_vfsp->vfs_flag & VFS_RDONLY) && dirty != NULL)
		err = EROFS;
	else
		err = 0;

	if (dirty != NULL) {
		/*
		 * ??
		 * If the modified time on the inode has not already been
		 * set elsewhere (i.e. for write/setattr) or this is
		 * a call from msync (B_FORCE) we set the time now.
		 * This gives us approximate modified times for mmap'ed files
		 * which are modified via stores in the user address space.
		 */
		pc_tvtopct(&time, &pcp->pc_entry.pcd_mtime);
		pcp->pc_flags |= PC_MOD | PC_CHG;
		devvp = fsp->pcfs_devvp;
		dev = devvp->v_rdev;
	}

	/*
	 * Handle all the dirty pages.
	 */
	pp = NULL;
	while (err == 0 && dirty != NULL) {
		/*
		 * Pull off a contiguous chunk that fits in one lbn.
		 */

		io_off = dirty->p_offset;
PCFSDEBUG(6)
printf("pc_putpage writing dirty page off=%d\n", io_off);

		lbn = pc_lblkno(fsp, io_off);

		VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_MISS);

		pp = dirty;
		lbn_off = io_off & ~(fsp->pcfs_clsize - 1);
		page_sub(&dirty, pp);

		/* IO may be asynch, so need to set nio first */
		nio = MIN(PAGESIZE/fsp->pcfs_clsize,
			howmany(pcp->pc_size-lbn_off, fsp->pcfs_clsize));
		if (nio > 1) {
			pp->p_nio = nio;
		} else {
			pp->p_nio = 0;
		}

		for (i = 0, pgoff = 0; i < nio; ++i, ++lbn) {
			struct buf *bp;
			/*
			 * ??
			 * Normally the blocks should already be allocated for
			 * any dirty pages.
			 */
			err = pc_bmap(pcp, lbn, &bn, (daddr_t *) 0);
			if (err)
				break;
			blksz = pc_blksize(fsp, pcp, lbn_off);
			lbn_off += blksz;

			bp = pageio_setup(pp, blksz, devvp, B_WRITE | flags);
			if (bp == NULL) {
				pvn_fail(pp, B_WRITE | flags);
				err = ENOMEM;
				break;
			}

			bp->b_dev = dev;
			bp->b_blkno = bn;
			bp->b_un.b_addr = (addr_t)pgoff;

			(*bdevsw[major(dev)].d_strategy)(bp);
			u.u_ru.ru_oublock++;

			/*
			 * If async, assume that pvn_done will
			 * handle the pages when IO is done
			 */
			if ((flags & B_ASYNC) == 0) {
				err = biowait(bp);
				pageio_done(bp);
			}
			if (err)
				break;
			pgoff += blksz;
		}
		if (err)
			break;
		pp = NULL;
	}

	if (err != 0) {
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && (len == 0 || len >= pcp->pc_size)) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
		    (vp->v_pages != NULL))
			goto again;
	}

	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */
	if (--vp->v_count == 0 && vpcount > 0)
		pc_rele(pcp); /* ?? */
	pc_unlockfs(fsp);
	return (err);
}

/*ARGSUSED*/
static int
pcfs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segvn_crargs vn_a;

	VFS_RECORD(vp->v_vfsp, VS_MAP, VS_CALL);

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_type != VREG)
		return (ENODEV);

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

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
}

static int
pcfs_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{
	VFS_RECORD(vp1->v_vfsp, VS_CMP, VS_CALL);
	return (vp1 == vp2);
}

/*ARGSUSED*/
static int
pcfs_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{
	VFS_RECORD(vp->v_vfsp, VS_REALVP, VS_CALL);
	return (EINVAL);
}

#ifdef notdef
static int
pcfs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct pcfid *pcfid;
	struct pcnode *pcp;

	VFS_RECORD(vp->v_vfsp, VS_FID, VS_CALL);

	if ((pcp = VTOPC (vp)) == NULL)
		return (EIO);
	pcfid = (struct pcfid *)kmem_zalloc(sizeof (struct pcfid));
	pcfid->pcfid_len = sizeof (struct pcfid) - sizeof (u_short);
	pcfid->pcfid_fileno = pc_makenodeid(pcp->pc_eblkno,
	    pcp->pc_eoffset, &(pcp->pc_entry));
	pcfid->pcfid_gen = pcp->pc_entry.pcd_gen;
	*fidpp = (struct fid *)pcfid;
	return (0);
}
#endif notdef

/*ARGSUSED*/
static int
pcfs_cntl(vp, cmd, idata, odata, iflag, oflag)
	struct vnode *vp;
	int cmd, iflag, oflag;
	caddr_t idata, odata;
{
	switch (cmd) {
	default:
		return (EINVAL);
	case _PC_LINK_MAX:
		*(int *)odata = 1;
		break;
	case _PC_NAME_MAX:
		if (vp->v_type == VREG)
			return (EINVAL);
		*(int *)odata = PCFNAMESIZE+PCFEXTSIZE+1;
		break;
	case _PC_PATH_MAX:
		if (vp->v_type == VREG)
			return (EINVAL);
		*(int *)odata = PCMAXPATHLEN;
		break;
	case _PC_CHOWN_RESTRICTED:
		*(int *)odata = 1;
		break;
	case _PC_NO_TRUNC:
		*(int *)odata = 0;
		break;
	}
	return (0);
}

int
pcfs_badop()
{
	panic("pcfs_badop");
}

int
pcfs_invalop()
{
PCFSDEBUG(1)
printf("pcfs_invalop\n");
	return (EINVAL);
}
