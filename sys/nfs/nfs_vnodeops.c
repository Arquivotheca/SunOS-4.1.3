#ident	"@(#)nfs_vnodeops.c 1.1 92/07/30 SMI"
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
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <sys/proc.h>
#include <sys/pathname.h>
#include <sys/dirent.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <sys/unistd.h>
#include <sys/mount.h>
#include <sys/vmmeter.h>
#include <sys/trace.h>
#include <sys/syslog.h>

#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/xdr.h>
#include <nfs/nfs.h>
#include <nfs/nfs_clnt.h>
#include <nfs/rnode.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <vm/swap.h>

#include <krpc/lockmgr.h>

#ifdef NFSDEBUG
extern int nfsdebug;
#endif

struct vnode *makenfsnode();
struct vnode *dnlc_lookup();
char *newname();
int setdirgid();
u_int setdirmode();

/*
 * Do close to open consistency checking on all filesystems.
 * If this boolean is false, CTO checking can be selectively
 * turned off by setting actimeo to -1 at mount time.
 */
int nfs_cto = 1;

/*
 * Error flags used to pass information about certain special errors
 * back from do_bio() to nfs_getapage() (yuck).
 */
#define	NFS_CACHEINVALERR	-99
#define	NFS_EOF			-98

#define	ISVDEV(t) ((t == VBLK) || (t == VCHR) || (t == VFIFO))

/*
 * These are the vnode ops routines which implement the vnode interface to
 * the networked file system.  These routines just take their parameters,
 * make them look networkish by putting the right info into interface structs,
 * and then calling the appropriate remote routine(s) to do the work.
 *
 * Note on directory name lookup cacheing:  we desire that all operations
 * on a given client machine come out the same with or without the cache.
 * To correctly do this, we serialize all operations on a given directory,
 * by using RLOCK and RUNLOCK around rfscalls to the server.  This way,
 * we cannot get into races with ourself that would cause invalid information
 * in the cache.  Other clients (or the server itself) can cause our
 * cached information to become invalid, the same as with data pages.
 * Also, if we do detect a stale fhandle, we purge the directory cache
 * relative to that vnode.  This way, the user won't get burned by the
 * cache repeatedly.
 */

static	int nfs_open();
static	int nfs_close();
static	int nfs_rdwr();
static	int nfs_ioctl();
static	int nfs_select();
static	int nfs_getattr();
static	int nfs_setattr();
static	int nfs_access();
static	int nfs_lookup();
static	int nfs_create();
static	int nfs_remove();
static	int nfs_link();
static	int nfs_rename();
static	int nfs_mkdir();
static	int nfs_rmdir();
static	int nfs_readdir();
static	int nfs_symlink();
static	int nfs_readlink();
static	int nfs_fsync();
static	int nfs_inactive();
static	int nfs_lockctl();
static	int nfs_noop();
static	int nfs_getpage();
static	int nfs_putpage();
static	int nfs_map();
static	int nfs_dump();
static  int nfs_cmp();
static  int nfs_realvp();
static  int nfs_cntl();

struct vnodeops nfs_vnodeops = {
	nfs_open,
	nfs_close,
	nfs_rdwr,
	nfs_ioctl,
	nfs_select,
	nfs_getattr,
	nfs_setattr,
	nfs_access,
	nfs_lookup,
	nfs_create,
	nfs_remove,
	nfs_link,
	nfs_rename,
	nfs_mkdir,
	nfs_rmdir,
	nfs_readdir,
	nfs_symlink,
	nfs_readlink,
	nfs_fsync,
	nfs_inactive,
	nfs_lockctl,
	nfs_noop,
	nfs_getpage,
	nfs_putpage,
	nfs_map,
	nfs_dump,
	nfs_cmp,
	nfs_realvp,
	nfs_cntl,
};

/*ARGSUSED*/
static int
nfs_open(vpp, flag, cred)
	register struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	int error;
	struct vattr va;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_open %s %x flag %d\n",
	    vtomi(*vpp)->mi_hostname, *vpp, flag);
#endif
	VFS_RECORD((*vpp)->v_vfsp, VS_OPEN, VS_CALL);

	error = 0;
	/*
	 * if close-to-open consistency checking is turned off
	 * we can avoid the over the wire getattr.
	 */
	if (nfs_cto || !vtomi(*vpp)->mi_nocto) {
		/*
		 * Force a call to the server to get fresh attributes
		 * so we can check caches. This is required for close-to-open
		 * consistency.
		 */
		error = nfs_getattr_otw(*vpp, &va, cred);
		if (error == 0) {
			nfs_cache_check(*vpp, va.va_mtime, va.va_size);
			nfs_attrcache_va(*vpp, &va);
		} else if (error == ESTALE) {
			/*
			 *	If we fail because of a stale NFS file handle,
			 *	restart the system call (nfs_getattr_otw will
			 *	have flushed all relevant caches).
			 */
				u.u_eosys = RESTARTSYS;
		}
	}
	return (error);
}

static int
nfs_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	register struct rnode *rp;
	int error = 0;

	VFS_RECORD(vp->v_vfsp, VS_CLOSE, VS_CALL);
	if (count > 1)
		return (0);

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_close %s %x flag %d\n",
	    vtomi(vp)->mi_hostname, vp, flag);
#endif
	VFS_RECORD(vp->v_vfsp, VS_CLOSE, VS_MISS);
	rp = vtor(vp);

	/*
	 * If the file is an unlinked file, then flush the lookup
	 * cache so that nfs_inactive will be called if this is
	 * the last reference.  Otherwise, if close-to-open
	 * consistency is turned on and the file was open
	 * for writing or we had an asynchronous write error, we
	 * force the "sync on close" semantic by calling nfs_putpage.
	 */
	if (rp->r_unldvp != NULL || rp->r_error) {
		(void) nfs_putpage(vp, 0, 0, B_INVAL, cred);
		dnlc_purge_vp(vp);
		error = rp->r_error;
		rp->r_error = 0;
	} else if ((nfs_cto || !vtomi(vp)->mi_nocto) &&
			(flag & FWRITE)) {
		(void) nfs_putpage(vp, 0, 0, 0, cred);
		if (rp->r_error) {
			(void) nfs_putpage(vp, 0, 0, B_INVAL, cred);
			dnlc_purge_vp(vp);
			error = rp->r_error;
			rp->r_error = 0;
		}
	}
	return (flag & FWRITE? error : 0);
}

static int
nfs_rdwr(vp, uio, rw, ioflag, cred)
	register struct vnode *vp;
	struct uio *uio;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	int error = 0;
	struct rnode *rp;
	u_int off;
	addr_t base;
	u_int flags;
	register int n, on;
	int eof = 0;
	int adjust_resid = 0;


#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "nfs_rdwr: %s %x rw %s offset %x len %d cred 0x%x\n",
	    vtomi(vp)->mi_hostname, vp, rw == UIO_READ ? "READ" : "WRITE",
	    uio->uio_offset, uio->uio_iov->iov_len, cred);
#endif
	if (vp->v_type != VREG) {
		return (EISDIR);
	}
	if (uio->uio_resid == 0) {
		return (0);
	}
	rp = vtor(vp);
	if (rw == UIO_WRITE || (rw == UIO_READ && rp->r_cred == NULL)) {
		crhold(cred);
		if (rp->r_cred) {
			crfree(rp->r_cred);
		}
		rp->r_cred = cred;
	}

#ifdef notdef
	if (ioflag & IO_UNIT) {
		RLOCK(rp);
	}
#endif

	/* Fix bug 1045993, huey */
	if (vp->v_flag & VNOCACHE) {
		struct vattr va;
	
		error = nfs_getattr_otw(vp, &va, cred);
		if (error)
			goto out;
	}

	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		struct vattr va;

		RLOCK(rp);
		error = nfs_getattr(vp, &va, cred);
		if (error)
			goto out;
		uio->uio_offset = va.va_size;
	}

	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0) {
		error = EINVAL;
		goto out;
	}
	if (rw == UIO_WRITE &&
	    uio->uio_offset+uio->uio_resid >
	    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
		if (uio->uio_offset >= u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
			psignal(u.u_procp, SIGXFSZ);
			error = EFBIG;
			goto out;
		} else {
			adjust_resid = uio->uio_resid;
			uio->uio_resid = u.u_rlimit[RLIMIT_FSIZE].rlim_cur -
							uio->uio_offset;
			adjust_resid -= uio->uio_resid;
		}
	}
	RLOCK(rp);

	do {
		off = uio->uio_offset & MAXBMASK; /* mapping offset */
		on = uio->uio_offset & MAXBOFFSET; /* Relative offset */
		n = MIN(MAXBSIZE - on, uio->uio_resid);

		if (rw == UIO_READ) {
			int diff;

			VFS_RECORD(vp->v_vfsp, VS_READ, VS_CALL);
			if (!(vp->v_flag & VNOCACHE) && page_find(vp, off)) {
				(void) nfs_validate_caches(vp, cred);
			}

			diff = rp->r_size - uio->uio_offset;
			if (diff <= 0) {
				break;
			}
			if (diff < n) {
				n = diff;
				eof = 1;
			}
		} else {	/* UIO_WRITE */

			/*
			 * Keep returning errors on rnode until
			 * rnode goes away.
			 */
			/* Fix Bug 1030884:
			 * Completely gross hack to make writes
			 * to the middle
			 * of a file succeed if the remote filesystem is full.
			 * Hack also in place in do_bio().
			 */

			if (rp->r_error && !(rp->r_error == ENOSPC
 					      && rp->r_attr.va_size >=
						off + n)){
				error = rp->r_error;
				break;
			}
			VFS_RECORD(vp->v_vfsp, VS_WRITE, VS_CALL);

			/*
			 * For file locking:  bypass VM to retain consistency
			 * in case only part of the file is locked and we don't
			 * want to write a whole page.
			 *
			 * XXX: size of the kmem_alloc may affect performance
			 */
			if (vp->v_flag & VNOCACHE) {
				caddr_t buf;
				int count, org_offset;
				u_int bufsize = MIN(uio->uio_resid, PAGESIZE);
				buf = new_kmem_alloc(bufsize, KMEM_SLEEP);
				while ((uio->uio_resid > 0) && (!error)) {
					count = MIN(uio->uio_resid, PAGESIZE);
                                        org_offset = (int) uio->uio_offset;
					error = uiomove(buf, count, UIO_WRITE,
						uio);
					rp->r_error = error = nfswrite(vp, buf,
						(u_int)org_offset,
						(long)count, rp->r_cred);
				}
				kmem_free(buf, bufsize);
				break;
			}
		}

		base = segmap_getmap(segkmap, vp, off);
		error = (rw == UIO_READ)? uiomove(base + on, n, UIO_READ, uio):
			writerp(vp, (base + on), n, uio);

		if (error == 0) {
			flags = 0;
			if (rw == UIO_WRITE) {
				/*
				 * Invalidate if entry is not to be cached.
				 */
				if (vp->v_flag & VNOCACHE)
					flags = SM_WRITE | SM_INVAL;
				else {
					rp->r_flags |= RDIRTY;
					if (n + on == MAXBSIZE ||
					    IS_SWAPVP(vp)) {
						/*
						 * Have written a whole block.
						 * Start an asynchronous write
						 * and mark the buffer to
						 * indicate that it won't be
						 * needed again soon.
						 */
						flags = SM_WRITE | SM_ASYNC |
						    SM_DONTNEED;
					}
				}
				if (ioflag & IO_SYNC) {
					flags &= ~SM_ASYNC;
					flags |= SM_WRITE;
				}
			} else {
				if (vp->v_flag & VNOCACHE)
					flags = SM_INVAL;
				else {
					/*
					 * If read a whole block or read to eof,
					 * won't need this buffer again soon.
					 */
					if (n + on == MAXBSIZE ||
					    uio->uio_offset == rp->r_size)
						flags = SM_DONTNEED;
				}
			}
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}

	} while (error == 0 && uio->uio_resid > 0 && !eof);
	RUNLOCK(rp);

	if (!error && adjust_resid) {
		uio->uio_resid = adjust_resid;
		psignal (u.u_procp, SIGXFSZ);
	}
out:
	if ((ioflag & IO_APPEND) && rw == UIO_WRITE) {
		RUNLOCK(rp);
	}
#ifdef notdef
	if (ioflag & IO_UNIT) {
		RUNLOCK(rp);
	}
#endif
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rdwr returning %d\n", error);
#endif
	return (error);
}


static int
writerp(vp, base, tcount, uio)
	struct vnode *vp;
	addr_t base;		/* base address kernel addr space */
	int tcount;		/* Total bytes to move - < MAXBSIZE */
	struct uio *uio;
{
	struct rnode *rp = vtor(vp);
	int pagecreate;
	register int n;
	register int offset;
	int error;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "writerp: vp 0x%x base 0x%x offset %d tcount %d\n", vp, base,
	    uio->uio_offset, tcount);
#endif

	ASSERT(tcount <= MAXBSIZE && tcount <= uio->uio_resid);
	ASSERT(((u_int)base & MAXBOFFSET) + tcount <= MAXBSIZE);

	/*
	 * Move bytes in at most PAGESIZE chunks. We must avoid
	 * spanning pages in uiomove() because page faults may cause
	 * the cache to be invalidated out from under us. The r_size is not
	 * updated until after the uiomove. If we push the last page of a
	 * file before r_size is correct, we will lose the data written past
	 * the current (and invalid) r_size.
	 */
	do {
		offset = uio->uio_offset;
		pagecreate = 0;

		/*
		 * n is the number of bytes required to satisfy the request
		 *   or the number of bytes to fill out the page.
		 */
		n = MIN((PAGESIZE - ((u_int)base & PAGEOFFSET)), tcount);

		/*
		 * Check to see if we can skip reading in the page
		 * and just allocate the memory.  We can do this
		 * if we are going to rewrite the entire mapping
		 * or if we are going to write to or beyond the current
		 * end of file from the beginning of the mapping.
		 */
		if (((u_int)base & PAGEOFFSET) == 0 && (n == PAGESIZE ||
			((offset + n) >= rp->r_size))) {
			segmap_pagecreate(segkmap, base, (u_int)n, 0);
			pagecreate = 1;
		}

		error = uiomove(base, n, UIO_WRITE, uio);
		n = uio->uio_offset - offset; /* n = # of bytes written */
		base += n;
		tcount -= n;

		/*
		 * If we created pages w/o initializing them completely,
		 * we need to zero the part that wasn't set up.
		 * This happens on a most EOF write cases and if
		 * we had some sort of error during the uiomove.
		 */
		if (pagecreate &&
			((uio->uio_offset & PAGEOFFSET) || n == 0)) {
			(void) kzero(base, (u_int)(PAGESIZE - n));
		}
		/*
		 * r_size is the maximum number of
		 * bytes known to be in the file.
		 * Make sure it is at least as high as the
		 * last byte we just wrote into the buffer.
		 */
		if (rp->r_size < uio->uio_offset) {
			rp->r_size = uio->uio_offset;
		}


	} while (tcount > 0 && error == 0);

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "writerp returning %d\n", error);
#endif
	return (error);
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 */
static int
nfs_writelbn(rp, pp, off, len, flags)
	register struct rnode *rp;
	struct page *pp;
	u_int off;
	u_int len;
	int flags;
{
	struct buf *bp;
	int err;

	bp = pageio_setup(pp, len, rtov(rp), B_WRITE | flags);
	if (bp == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return (ENOMEM);
	}

	bp->b_dev = 0;
	bp->b_blkno = btodb(off);
	bp_mapin(bp);

	err = nfs_strategy(bp);
	u.u_ru.ru_oublock++;

#ifdef NFSDEBUG
	dprint(nfsdebug, 5,
	    "nfs_writelbn %s blkno %d pp %x len %d flags %x error %d\n",
	    vtomi(rtov(rp))->mi_hostname, btodb(off), pp, len, flags, err);
#endif
	return (err);
}

/*
 * Write to file.  Writes to remote server in largest size
 * chunks that the server can handle.  Write is synchronous.
 */
static int
nfswrite(vp, base, offset, count, cred)
	struct vnode *vp;
	caddr_t base;
	u_int offset;
	long count;
	struct ucred *cred;
{
	int error;
	struct nfswriteargs wa;
	struct nfsattrstat *ns;
	int tsize;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfswrite %s %x offset = %d, count = %d\n",
	    vtomi(vp)->mi_hostname, vp, offset, count);
#endif
	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_MISS);

	ns = (struct nfsattrstat *)new_kmem_alloc(sizeof (*ns), KMEM_SLEEP);
	/*
	 * Temporarily invalidate attr cache since we know mtime will change
	 */
	INVAL_ATTRCACHE(vp);

	do {
		tsize = MIN(vtomi(vp)->mi_curwrite, count);
		wa.wa_data = base;
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		error = rfscall(vtomi(vp), RFS_WRITE, xdr_writeargs,
		    (caddr_t)&wa, xdr_attrstat, (caddr_t)ns, cred);
		if (error == ENFS_TRYAGAIN) {
			error = 0;
			continue;
		}
		if (!error) {
			error = geterrno(ns->ns_status);
			/*
			 * Can't check for stale fhandle and purge caches
			 * here because pages are held by nfs_getpage.
			 */
		}
#ifdef NFSDEBUG
		dprint(nfsdebug, 3, "nfswrite: sent %d of %d, error %d\n",
		    tsize, count, error);
#endif
		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (!error) {
		nfs_attrcache(vp, &ns->ns_attr);
	} else {
		/*
		 * Since we invalidated the cache above without first
		 * purging cached pages we have to put it back in the
		 * "timed-out" state.
		 */
		PURGE_ATTRCACHE(vp);
	}
	kmem_free((caddr_t)ns, sizeof (*ns));
	switch (error) {
	case 0:
	case EDQUOT:
	case EINTR:
		break;

	case ENOSPC:
		printf("NFS write error: on host %s remote file system full\n",
			vtomi(vp)->mi_hostname);
		break;

	default:
		printf("NFS write error %d on host %s fh ",
		    error, vtomi(vp)->mi_hostname);
		printfhandle((caddr_t)vtofh(vp));
		printf("\n");
		break;
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfswrite: returning %d\n", error);
#endif
	return (error);
}

/*
 * Print a file handle
 */
printfhandle(fh)
	caddr_t fh;
{
	int i;
	int fhint[NFS_FHSIZE / sizeof (int)];

	bcopy(fh, (caddr_t)fhint, sizeof (fhint));
	for (i = 0; i < (sizeof (fhint) / sizeof (int)); i++) {
		printf("%x ", fhint[i]);
	}
}

/*
 * Read from a file.  Reads data in largest chunks our interface can handle.
 */
static int
nfsread(vp, base, offset, count, residp, cred, vap)
	struct vnode *vp;
	caddr_t base;
	u_int offset;
	long count;
	long *residp;
	struct ucred *cred;
	struct vattr *vap;
{
	int error;
	struct nfsreadargs ra;
	struct nfsrdresult rr;
	register int tsize;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfsread %s %x offset = %d, totcount = %d\n",
	    vtomi(vp)->mi_hostname, vp, offset, count);
#endif
	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_MISS);

	do {
		do {
			tsize = MIN(vtomi(vp)->mi_curread, count);
			rr.rr_data = base;
			ra.ra_fhandle = *vtofh(vp);
			ra.ra_offset = offset;
			ra.ra_totcount = tsize;
			ra.ra_count = tsize;
			error = rfscall(vtomi(vp), RFS_READ,
					xdr_readargs, (caddr_t)&ra,
					xdr_rdresult, (caddr_t)&rr,
					cred);
		} while (error == ENFS_TRYAGAIN);

		if (!error) {
			error = geterrno(rr.rr_status);
			/*
			 * Can't purge caches here because pages are held by
			 * nfs_getpage.
			 */
		}
#ifdef NFSDEBUG
		dprint(nfsdebug, 3, "nfsread: got %d of %d, error %d\n",
		    tsize, count, error);
#endif
		if (!error) {
			count -= rr.rr_count;
			base += rr.rr_count;
			offset += rr.rr_count;
		}
	} while (!error && count && rr.rr_count == tsize);

	*residp = count;

	if (!error) {
		nattr_to_vattr(vp, &rr.rr_attr, vap);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfsread: returning %d, resid %d\n",
		error, *residp);
#endif
	return (error);
}

/*ARGSUSED*/
static int
nfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{

	VFS_RECORD(vp->v_vfsp, VS_IOCTL, VS_CALL);
	return (EOPNOTSUPP);
}

/*ARGSUSED*/
static int
nfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

	VFS_RECORD(vp->v_vfsp, VS_SELECT, VS_CALL);
	return (EOPNOTSUPP);
}

static int
nfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	int error;
	struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_getattr %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif
	rp = vtor(vp);
	if (rp->r_flags & RDIRTY) {
		/*
		 * Since we know we have pages which are dirty because
		 * we went thru rwvp for writing, we sync pages so the
		 * mod time is right.  Note that if a page which is mapped
		 * in user land is modified, the page will not be flushed
		 * until the next sync or appropriate fsync or msync operation.
		 */
		(void) nfs_putpage(vp, 0, 0, 0, cred);
	}
	error = nfsgetattr(vp, vap, cred);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_getattr: returns %d\n", error);
#endif
	return (error);
}

static int
nfs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	int error;
	struct nfssaargs args;
	struct nfsattrstat *ns;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_setattr %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif
	VFS_RECORD(vp->v_vfsp, VS_SETATTR, VS_CALL);

	ns = (struct nfsattrstat *)new_kmem_alloc(sizeof (*ns), KMEM_SLEEP);
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_ctime.tv_sec != -1) || (vap->va_ctime.tv_usec != -1)) {
		error = EINVAL;
	} else {
		RLOCK(vtor(vp));
		if (vap->va_size != -1) {
#ifdef  TRACE
			{
				struct vattr oldvap;

				nfsgetattr(vp, &oldvap, cred);
				trace3(TR_MP_TRUNC, vp, vap->va_size,
					oldvap.va_size);
			}
#endif  TRACE
			pvn_vptrunc(vp, (u_int)vap->va_size, (u_int)(PAGESIZE -
			    (vap->va_size & PAGEOFFSET)));
			(vtor(vp))->r_size = vap->va_size;
		}
		(void) nfs_putpage(vp, 0, 0, 0, cred);
		RUNLOCK(vtor(vp));

		/*
		 * Allow SysV-compatible option to set access and
		 * modified times if root, owner, or write access.
		 *
		 * XXX - For now, va_mtime.tv_usec == -1 flags this.
		 *
		 * XXX - Until an NFS Protocol Revision, this may be
		 *	 simulated by setting the client time in the
		 *	 tv_sec field of the access and modified times
		 *	 and setting the tv_usec field of the modified
		 *	 time to an invalid value (1000000).  This
		 *	 may be detected by servers modified to do the
		 *	 right thing, but will not be disastrous on
		 *	 unmodified servers.
		 */
		if ((vap->va_mtime.tv_sec != -1) &&
		    (vap->va_mtime.tv_usec == -1)) {
			vap->va_atime = time;
			vap->va_mtime.tv_sec = time.tv_sec;
			vap->va_mtime.tv_usec = 1000000;
		}

		vattr_to_sattr(vap, &args.saa_sa);
		args.saa_fh = *vtofh(vp);
		VFS_RECORD(vp->v_vfsp, VS_SETATTR, VS_MISS);
		error = rfscall(vtomi(vp), RFS_SETATTR, xdr_saargs,
		    (caddr_t)&args, xdr_attrstat, (caddr_t)ns, cred);
		if (error == 0) {
			error = geterrno(ns->ns_status);
			if (error == 0) {
				nfs_cache_check(vp, ns->ns_attr.na_mtime,
				    ns->ns_attr.na_size);
				nfs_attrcache(vp, &ns->ns_attr);
			} else {
				PURGE_ATTRCACHE(vp);
				PURGE_STALE_FH(error, vp);
			}
		} else
			PURGE_ATTRCACHE(vp);
	}
	kmem_free((caddr_t)ns, sizeof (*ns));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_setattr: returning %d\n", error);
#endif
	return (error);
}

static int
nfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	struct vattr va;
	int *gp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_access %s %x mode %d uid %d\n",
	    vtomi(vp)->mi_hostname, vp, mode, cred->cr_uid);
#endif
	VFS_RECORD(vp->v_vfsp, VS_ACCESS, VS_CALL);

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
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group,
	 * then check public access.
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
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_access: returning %d\n", u.u_error);
#endif
	u.u_error = EACCES;
	return (EACCES);
}

static int
nfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	int error;
	struct nfsrdlnres rl;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_readlink %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif
	VFS_RECORD(vp->v_vfsp, VS_READLINK, VS_CALL);

	if (vp->v_type != VLNK)
		return (ENXIO);
	rl.rl_data = (char *)new_kmem_alloc(NFS_MAXPATHLEN, KMEM_SLEEP);
	VFS_RECORD(vp->v_vfsp, VS_READLINK, VS_MISS);
	error = rfscall(vtomi(vp), RFS_READLINK, xdr_fhandle,
	    (caddr_t)vtofh(vp), xdr_rdlnres, (caddr_t)&rl, cred);
	if (!error) {
		error = geterrno(rl.rl_status);
		if (!error) {
			error = uiomove(rl.rl_data, (int)rl.rl_count,
			    UIO_READ, uiop);
		} else {
			PURGE_STALE_FH(error, vp);
		}
	}
	kmem_free((caddr_t)rl.rl_data, NFS_MAXPATHLEN);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readlink: returning %d\n", error);
#endif
	return (error);
}

/*ARGSUSED*/
static int
nfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct rnode *rp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_fsync %s %x\n", vtomi(vp)->mi_hostname, vp);
#endif
	VFS_RECORD(vp->v_vfsp, VS_FSYNC, VS_CALL);

	rp = vtor(vp);
	RLOCK(rp);  /* XXX-VLS Why? */
	(void)nfs_putpage(vp, 0, 0, 0, cred);
	RUNLOCK(rp);
	return (rp->r_error);
}

/*
 * Weirdness: if the file was removed while it was open it got renamed
 * (by nfs_remove) instead.  Here we remove the renamed file.
 */
/*ARGSUSED*/
static int
nfs_inactive(vp, cred)
	register struct vnode *vp;
	struct ucred *cred;
{
	register struct rnode *rp;
	struct nfsdiropargs da;
	enum nfsstat status;
	int error = 0;
	struct rnode *unlrp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_inactive %s, %x\n",
	    vtomi(vp)->mi_hostname, vp);
#endif
	VFS_RECORD(vp->v_vfsp, VS_INACTIVE, VS_CALL);

	rp = vtor(vp);

	if (vp->v_count != 0) {
		return (0);
	}

redo:
	if (rp->r_unldvp != NULL) {
		/*
		 * Lock down directory where unlinked-open file got renamed.
		 * This keeps a lookup from finding this rnode.
		 * Fix bug 1034328 - corbin
		 * Lock rnode down until we are finished doing the remove
		 * to prevent a race condition.
		 * The locking sequence is important here to prevent deadlock.
		 */
		unlrp = vtor(rp->r_unldvp);
		RLOCK(unlrp);
		RLOCK(rp);
	
		if (vp->v_count != 0) {
			RUNLOCK(rp);
			RUNLOCK(unlrp);
			return(0);
		}

		if (rp->r_unldvp == NULL) {
			RUNLOCK(rp);
			RUNLOCK(unlrp);
			goto redo;
		}

		rp->r_flags &= ~RDIRTY;
		trace1(TR_MP_TRUNC0, vp);
		pvn_vptrunc(vp, 0, 0);		/* toss all pages */

		/*
		 * Do the remove operation on the renamed file
		 */
		setdiropargs(&da, rp->r_unlname, rp->r_unldvp);
		VFS_RECORD(vp->v_vfsp, VS_INACTIVE, VS_MISS);
		error = rfscall(vtomi(rp->r_unldvp), RFS_REMOVE,
		    xdr_diropargs, (caddr_t)&da,
		    xdr_enum, (caddr_t)&status, rp->r_unlcred);
		if (error == 0)
			error = geterrno(status);

		/*
		 * Release stuff held for the remove
		 */
		VN_RELE(rp->r_unldvp);
		rp->r_unldvp = NULL;
		kmem_free((caddr_t)rp->r_unlname, NFS_MAXNAMLEN);
		rp->r_unlname = NULL;
		crfree(rp->r_unlcred);
		rp->r_unlcred = NULL;
		RUNLOCK(rp); /* Fix bug 1034328 */
		RUNLOCK(unlrp);
	} else {
		if (vp->v_pages != 0) {
			if (rp->r_error) {
				(void) nfs_putpage(vp, 0, 0, B_INVAL, cred);
				dnlc_purge_vp(vp);
				rp->r_error = 0;
			} else {
				(void) nfs_putpage(vp, 0, 0, 0, cred);
				if (rp->r_error) {
					(void) nfs_putpage(vp, 0, 0, B_INVAL,
					    cred);
					dnlc_purge_vp(vp);
					rp->r_error = 0;
				}
			}
		}
	}


	/*
	 * Check to be sure that the rnode has not been grabbed before
	 * freeing it.
	 */
	if (vp->v_count == 0) {
		if (rp->r_unldvp != NULL) {
			goto redo;
		}
		rp_rmhash(rp);
		rfree(rp);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_inactive done\n");
#endif
	return (error);
}

int nfs_dnlc = 1;	/* use dnlc */

/*
 * Remote file system operations having to do with directory manipulation.
 */
/* ARGSUSED */
static int
nfs_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	int error;
	struct nfsdiropargs da;
	struct  nfsdiropres *dr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_lookup %s %x '%s'\n",
	    vtomi(dvp)->mi_hostname, dvp, nm);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_CALL);

	/*
	 * Before checking dnlc, validate caches
	 */
	error = nfs_validate_caches(dvp, cred);
	if (error) {
		return (error);
	}

	RLOCK(vtor(dvp));
	*vpp = (struct vnode *)dnlc_lookup(dvp, nm, cred);
	if (*vpp) {
		VN_HOLD(*vpp);

		/*
		 *	Make sure we can search this directory (after the
		 *	fact).  It's done here because over the wire lookups
		 *	verify permissions on the server.  VOP_ACCESS will
		 *	one day go over the wire, so let's use it sparingly.
		 */
		error = VOP_ACCESS(dvp, VEXEC, cred);
		if (error) {
			VN_RELE(*vpp);
			RUNLOCK(vtor(dvp));
			return (error);
		}
	} else {
		VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_MISS);

		dr = (struct  nfsdiropres *)
			new_kmem_alloc(sizeof (*dr), KMEM_SLEEP);
		setdiropargs(&da, nm, dvp);

		error = rfscall(vtomi(dvp), RFS_LOOKUP, xdr_diropargs,
		    (caddr_t)&da, xdr_diropres, (caddr_t)dr, cred);
		if (error == 0) {
			error = geterrno(dr->dr_status);
			PURGE_STALE_FH(error, dvp);
		}
		if (error == 0) {
			*vpp = makenfsnode(&dr->dr_fhandle,
			    &dr->dr_attr, dvp->v_vfsp);
			if (nfs_dnlc && (vtomi(*vpp)->mi_noac == 0)) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}
		} else {
			*vpp = (struct vnode *)0;
		}

		kmem_free((caddr_t)dr, sizeof (*dr));
	}
	/*
	 * If vnode is a device create special vnode
	 */
	if (!error && ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type);
		VN_RELE(*vpp);
		*vpp = newvp;
	}
	RUNLOCK(vtor(dvp));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_lookup returning %d vp = %x\n", error, *vpp);
#endif
	return (error);
}

/*ARGSUSED*/
static int
nfs_create(dvp, nm, va, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;
	int file_exist = 0; /* Fix bug 1065361 */

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_create %s %x '%s' excl=%d, mode=%o\n",
	    vtomi(dvp)->mi_hostname, dvp, nm, exclusive, mode);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_CREATE, VS_CALL);

		/*
		 * This is buggy: there is a race between the lookup and the
		 * create.  We should send the exclusive flag over the wire.
		 */
		/* Fix bug 1065361: huey
		 * Need to do an nfs_lookup to determine if file exists
		 * or not, to see if the preferred gid should be used.
		 */
	error = nfs_lookup(dvp, nm, vpp, cred,
				(struct pathname *)NULL, 0);
	if (!error) {
		VN_RELE(*vpp);
		if (exclusive == EXCL)
			return (EEXIST);
		file_exist = 1;
	}

	*vpp = (struct vnode *)0;

	dr = (struct  nfsdiropres *)new_kmem_alloc(sizeof (*dr), KMEM_SLEEP);
	setdiropargs(&args.ca_da, nm, dvp);

	/*
	 * Decide what the group-id of the created file should be.
	 * Set it in attribute list as advisory...then do a setattr
	 * if the server didn't get it right the first time.
	 */
	va->va_gid = (short) setdirgid(dvp);

	/*
	 * This is a completely gross hack to make mknod
	 * work over the wire until we can wack the protocol
	 */
#define	IFCHR		0020000		/* character special */
#define	IFBLK		0060000		/* block special */
#define	IFSOCK		0140000		/* socket */
	if (va->va_type == VCHR) {
		va->va_mode |= IFCHR;
		va->va_size = (u_long)va->va_rdev;
	} else if (va->va_type == VBLK) {
		va->va_mode |= IFBLK;
		va->va_size = (u_long)va->va_rdev;
	} else if (va->va_type == VFIFO) {
		va->va_mode |= IFCHR;		/* xtra kludge for namedpipe */
		va->va_size = (u_long)NFS_FIFO_DEV;	/* blech */
	} else if (va->va_type == VSOCK) {
		va->va_mode |= IFSOCK;
	}

	vattr_to_sattr(va, &args.ca_sa);
	VFS_RECORD(dvp->v_vfsp, VS_CREATE, VS_MISS);
	RLOCK(vtor(dvp));
	dnlc_remove(dvp, nm);
	error = rfscall(vtomi(dvp), RFS_CREATE, xdr_creatargs, (caddr_t)&args,
	    xdr_diropres, (caddr_t)dr, cred);
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(dr->dr_status);
		if (!error) {
			short gid;

			*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr,
			    dvp->v_vfsp);
			if (va->va_size == 0) {
				(vtor(*vpp))->r_size = 0;
				if (((*vpp)->v_pages != NULL) &&
				((*vpp)->v_type != VCHR) &&
				((*vpp)->v_type != VSOCK)) {
					RLOCK(vtor(*vpp));
					(vtor(*vpp))->r_flags &= ~RDIRTY;
					(vtor(*vpp))->r_error = 0;
					pvn_vptrunc(*vpp, 0, 0);
					RUNLOCK(vtor(*vpp));
				}
			}
			if (nfs_dnlc && (vtomi(*vpp)->mi_noac == 0)) {
				dnlc_enter(dvp, nm, *vpp, cred);
			}

			/*
			 * Make sure the gid was set correctly.
			 * If not, try to set it (but don't lose
			 * any sleep over it).
			 */
			gid = va->va_gid;
			nattr_to_vattr(*vpp, &dr->dr_attr, va);

			/* Fix buig 1065361, huey
			 * The logix should apply to new file.
			 */
			if ((gid != va->va_gid) && (!file_exist)) {
				struct vattr vattr;

				vattr_null(&vattr);
				vattr.va_gid = gid;
				(void) nfs_setattr(*vpp, &vattr, cred);
				va->va_gid = gid;
			}

			/*
			 * If vnode is a device create special vnode
			 */
			if (ISVDEV((*vpp)->v_type)) {
				struct vnode *newvp = specvp(
				    *vpp, (*vpp)->v_rdev, (*vpp)->v_type);
				VN_RELE(*vpp);
				*vpp = newvp;
			}
		} else {
			PURGE_STALE_FH(error, dvp);
		}
	}
	RUNLOCK(vtor(dvp));
	kmem_free((caddr_t)dr, sizeof (*dr));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_create returning %d\n", error);
#endif
	return (error);
}

/*
 * Weirdness: if the vnode to be removed is open
 * we rename it instead of removing it and nfs_inactive
 * will remove the new name.
 */
static int
nfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	struct nfsdiropargs da;
	enum nfsstat status;
	struct vnode *vp;
	struct vnode *oldvp;
	struct vnode *realvp;
	char *tmpname;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_remove %s %x '%s'\n",
	    vtomi(dvp)->mi_hostname, vp, nm);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_REMOVE, VS_CALL);

	status = NFS_OK;
	error = nfs_lookup(dvp, nm, &vp, cred, (struct pathname *) NULL, 0);
	/*
	 * Lookup may have returned a non-nfs vnode!
	 * get the real vnode.
	 */
	if (error == 0 && VOP_REALVP(vp, &realvp) == 0) {
		oldvp = vp;
		vp = realvp;
	} else {
		oldvp = NULL;
	}
	if (error == 0 && vp != NULL) {
		RLOCK(vtor(dvp));
		/*
		 * We need to flush the name cache so we can
		 * check the real reference count on the vnode
		 */
		dnlc_purge_vp(vp);

		if ((vp->v_count > 1) && vtor(vp)->r_unldvp == NULL) {
			tmpname = newname();
			error = nfs_rename(dvp, nm, dvp, tmpname, cred);
			if (error) {
				kmem_free((caddr_t)tmpname, NFS_MAXNAMLEN);
			} else {
				VN_HOLD(dvp);
				vtor(vp)->r_unldvp = dvp;
				vtor(vp)->r_unlname = tmpname;
				if (vtor(vp)->r_unlcred != NULL) {
					crfree(vtor(vp)->r_unlcred);
				}
				crhold(cred);
				vtor(vp)->r_unlcred = cred;
			}
		} else {
			vtor(vp)->r_flags &= ~RDIRTY;
			trace1(TR_MP_TRUNC0, vp);
			pvn_vptrunc(vp, 0, 0);		/* toss all pages */
			setdiropargs(&da, nm, dvp);
			VFS_RECORD(dvp->v_vfsp, VS_REMOVE, VS_MISS);
			error = rfscall(vtomi(dvp), RFS_REMOVE, xdr_diropargs,
			    (caddr_t)&da, xdr_enum, (caddr_t)&status,
			    cred);
			PURGE_ATTRCACHE(dvp);	/* mod time changed */
			PURGE_ATTRCACHE(vp);	/* link count changed */
			PURGE_STALE_FH(error ? error : geterrno(status), dvp);
		}
		RUNLOCK(vtor(dvp));
		if (oldvp) {
			VN_RELE(oldvp);
		} else {
			VN_RELE(vp);
		}
	}
	if (error == 0) {
		error = geterrno(status);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_remove: returning %d\n", error);
#endif
	return (error);
}

static int
nfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfslinkargs args;
	enum nfsstat status;
	struct vnode *realvp;

	if (VOP_REALVP(vp, &realvp) == 0) {
		vp = realvp;
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_link from %s %x to %s %x '%s'\n",
	    vtomi(vp)->mi_hostname, vp, vtomi(tdvp)->mi_hostname, tdvp, tnm);
#endif
	VFS_RECORD(vp->v_vfsp, VS_LINK, VS_CALL);
	VFS_RECORD(vp->v_vfsp, VS_LINK, VS_MISS);

	args.la_from = *vtofh(vp);
	setdiropargs(&args.la_to, tnm, tdvp);
	RLOCK(vtor(tdvp));
	error = rfscall(vtomi(vp), RFS_LINK, xdr_linkargs, (caddr_t)&args,
	    xdr_enum, (caddr_t)&status, cred);
	PURGE_ATTRCACHE(tdvp);	/* mod time changed */
	PURGE_ATTRCACHE(vp);	/* link count changed */
	RUNLOCK(vtor(tdvp));
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, vp);
		PURGE_STALE_FH(error, tdvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_link returning %d\n", error);
#endif
	return (error);
}

static int
nfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsrnmargs args;
	struct vnode *realvp;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rename from %s %x '%s' to %s %x '%s'\n",
	    vtomi(odvp)->mi_hostname, odvp, onm,
	    vtomi(ndvp)->mi_hostname, ndvp, nnm);
#endif
	VFS_RECORD(odvp->v_vfsp, VS_RENAME, VS_CALL);

	if (VOP_REALVP(ndvp, &realvp) == 0) {
		ndvp = realvp;
	}

	if (!strcmp(onm, ".") || !strcmp(onm, "..") || !strcmp(nnm, ".") ||
	    !strcmp (nnm, "..")) {
		error = EINVAL;
	} else {
		RLOCK(vtor(odvp));
		dnlc_remove(odvp, onm);
		dnlc_remove(ndvp, nnm);
		if (ndvp != odvp) {
			RLOCK(vtor(ndvp));
		}
		setdiropargs(&args.rna_from, onm, odvp);
		setdiropargs(&args.rna_to, nnm, ndvp);
		VFS_RECORD(odvp->v_vfsp, VS_RENAME, VS_MISS);
		error = rfscall(vtomi(odvp), RFS_RENAME, xdr_rnmargs,
		    (caddr_t)&args, xdr_enum, (caddr_t)&status,
		    cred);
		PURGE_ATTRCACHE(odvp);	/* mod time changed */
		PURGE_ATTRCACHE(ndvp);	/* mod time changed */
		RUNLOCK(vtor(odvp));
		if (ndvp != odvp) {
			RUNLOCK(vtor(ndvp));
		}
		if (!error) {
			error = geterrno(status);
			PURGE_STALE_FH(error, odvp);
			PURGE_STALE_FH(error, ndvp);
		}
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rename returning %d\n", error);
#endif
	return (error);
}

static int
nfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	int error;
	struct nfscreatargs args;
	struct  nfsdiropres *dr;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_mkdir %s %x '%s'\n",
	    vtomi(dvp)->mi_hostname, dvp, nm);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_MKDIR, VS_CALL);
	VFS_RECORD(dvp->v_vfsp, VS_MKDIR, VS_MISS);

	dr = (struct  nfsdiropres *)
			new_kmem_alloc(sizeof (*dr), KMEM_SLEEP);
	setdiropargs(&args.ca_da, nm, dvp);

	/*
	 * Decide what the group-id and set-gid bit of the created directory
	 * should be.  May have to do a setattr to get the gid right.
	 */
	va->va_gid = (short) setdirgid(dvp);
	va->va_mode = (u_short) setdirmode(dvp, va->va_mode);

	vattr_to_sattr(va, &args.ca_sa);
	RLOCK(vtor(dvp));
	dnlc_remove(dvp, nm);
	error = rfscall(vtomi(dvp), RFS_MKDIR, xdr_creatargs, (caddr_t)&args,
	    xdr_diropres, (caddr_t)dr, cred);
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	RUNLOCK(vtor(dvp));
	if (!error) {
		error = geterrno(dr->dr_status);
		PURGE_STALE_FH(error, dvp);
	}
	if (!error) {
		short gid;

		/*
		 * The attributes returned by RFS_MKDIR are now correct and
		 * may be safely used by the clients.
		 */
		*vpp = makenfsnode(&dr->dr_fhandle, &dr->dr_attr, dvp->v_vfsp);
		PURGE_ATTRCACHE(*vpp);

		if (nfs_dnlc && (vtomi(*vpp)->mi_noac == 0)) {
			dnlc_enter(dvp, nm, *vpp, cred);
		}

		/*
		 * Make sure the gid was set correctly.
		 * If not, try to set it (but don't lose
		 * any sleep over it).
		 */
		gid = va->va_gid;
		nattr_to_vattr(*vpp, &dr->dr_attr, va);
		if (gid != va->va_gid) {
			vattr_null(va);
			va->va_gid = gid;
			(void) nfs_setattr(*vpp, va, cred);
		}
	} else {
		*vpp = (struct vnode *)0;
	}
	kmem_free((caddr_t)dr, sizeof (*dr));
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_mkdir returning %d\n", error);
#endif
	return (error);
}

static int
nfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	int error;
	enum nfsstat status;
	struct nfsdiropargs da;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_rmdir %s %x '%s'\n",
	    vtomi(dvp)->mi_hostname, dvp, nm);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_RMDIR, VS_CALL);
	VFS_RECORD(dvp->v_vfsp, VS_RMDIR, VS_MISS);

	setdiropargs(&da, nm, dvp);
	RLOCK(vtor(dvp));
	dnlc_purge_vp(dvp);
	error = rfscall(vtomi(dvp), RFS_RMDIR, xdr_diropargs, (caddr_t)&da,
	    xdr_enum, (caddr_t)&status, cred);
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	RUNLOCK(vtor(dvp));
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, dvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_rmdir returning %d\n", error);
#endif
	return (error);
}

static int
nfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{
	int error;
	struct nfsslargs args;
	enum nfsstat status;

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_symlink %s %x '%s' to '%s'\n",
	    vtomi(dvp)->mi_hostname, dvp, lnm, tnm);
#endif
	VFS_RECORD(dvp->v_vfsp, VS_SYMLINK, VS_CALL);
	VFS_RECORD(dvp->v_vfsp, VS_SYMLINK, VS_MISS);

	setdiropargs(&args.sla_from, lnm, dvp);
	vattr_to_sattr(tva, &args.sla_sa);
	args.sla_tnm = tnm;
	error = rfscall(vtomi(dvp), RFS_SYMLINK, xdr_slargs, (caddr_t)&args,
	    xdr_enum, (caddr_t)&status, cred);
	PURGE_ATTRCACHE(dvp);	/* mod time changed */
	if (!error) {
		error = geterrno(status);
		PURGE_STALE_FH(error, dvp);
	}
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_sysmlink: returning %d\n", error);
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
static int
nfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	int error = 0;
	struct iovec *iovp;
	unsigned alloc_count, count;
	struct nfsrddirargs rda;
	struct nfsrddirres  rd;
	struct rnode *rp;

	VFS_RECORD(vp->v_vfsp, VS_READDIR, VS_CALL);

	rp = vtor(vp);

	/*
	 *	N.B.:  it appears here that we're treating the directory
	 *	cookie as an offset.  Not true.  It's simply that getdents
	 *	passes us the cookie to use in the uio_offset field of a
	 *	uio structure.
	 */
	if ((rp->r_lastcookie == (u_long)uiop->uio_offset) &&
	(rp->r_flags & REOF) && (timercmp(&time, &rp->r_attrtime, <))) {
		return (0);
	}
	iovp = uiop->uio_iov;
	alloc_count = count = iovp->iov_len;
#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_readdir %s %x count %d offset %ld\n",
	    vtomi(vp)->mi_hostname, vp, count, uiop->uio_offset);
#endif
	/*
	 * XXX We should do some kind of test for count >= DEV_BSIZE
	 */
	if (uiop->uio_iovcnt != 1) {
		return (EINVAL);
	}
	rda.rda_offset = uiop->uio_offset;
	rd.rd_entries = (struct dirent *)
				new_kmem_alloc(alloc_count, KMEM_SLEEP);
	rda.rda_fh = *vtofh(vp);
	do {
		count = MIN(count, vtomi(vp)->mi_curread);
		rda.rda_count = count;
		rd.rd_size = count;
		VFS_RECORD(vp->v_vfsp, VS_READDIR, VS_MISS);

		error = rfscall(vtomi(vp), RFS_READDIR, xdr_rddirargs,
				(caddr_t)&rda, xdr_getrddirres, (caddr_t)&rd,
				cred);
	} while (error == ENFS_TRYAGAIN);

	if (!error) {
		error = geterrno(rd.rd_status);
		PURGE_STALE_FH(error, vp);
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
			rp->r_lastcookie = uiop->uio_offset;
		}
	}
	kmem_free((caddr_t)rd.rd_entries, alloc_count);
#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_readdir: returning %d resid %d, offset %ld\n",
	    error, uiop->uio_resid, uiop->uio_offset);
#endif
	return (error);
}

static struct buf async_bufhead = {
	B_HEAD,
	(struct buf *)NULL, (struct buf *)NULL,
	&async_bufhead, &async_bufhead,
};

static int async_daemon_ready;		/* number of async biod's ready */
static int async_daemon_count;		/* number of existing biod's */

int nfs_wakeup_one_biod = 1;

static int
nfs_strategy(bp)
	register struct buf *bp;
{

#ifdef NFSDEBUG
	dprint(nfsdebug, 4, "nfs_strategy bp %x lbn %d\n", bp, bp->b_blkno);
#endif

	if (async_daemon_ready > 0 && (bp->b_flags & B_ASYNC)) {

		binstailfree(bp, &async_bufhead);

		async_daemon_ready--;
		if (nfs_wakeup_one_biod == 1)	{
			wakeup_one((caddr_t)&async_bufhead);
		} else {
			wakeup((caddr_t)&async_bufhead);
		}
		return (0);
	} else {
		return (do_bio(bp));
	}
}

async_daemon()
{
	register struct buf *bp;
	struct rnode *rp;
	char exitsig;

	relvm(u.u_procp);	/* First, release resources */

	async_daemon_count++;
	if (setjmp(&u.u_qsave)) {
		if (async_daemon_count == 0) {
			/*
			 * We already were processing requests below
			 * and we were signaled again.  So this time,
			 * just give up and abort all the requests.
			 */
			while ((bp = async_bufhead.b_actf) != &async_bufhead) {
				bremfree(bp);
				bp->b_flags |= B_ERROR;
				/*
				 * Since we are always ASYNC pvn_done
				 * will free the buf.
				 */
				pvn_done(bp);
			}
		} else {
			async_daemon_count--;
			async_daemon_ready--;
			/*
			 * If we were the last async daemon,
			 * process all the queued requests.
			 */
			if (async_daemon_count == 0) {
				while ((bp = async_bufhead.b_actf) !=
				    &async_bufhead) {
					bremfree(bp);
					rp = vtor(bp->b_vp);
					/*
					 * Since we are ASYNC do_bio will
					 * free the bp.
					 */
					if (do_bio(bp) == NFS_CACHEINVALERR) {
						nfs_purge_caches(rtov(rp));
					}
				}
			}
		}
		exitsig = u.u_procp->p_cursig;
		if (exitsig != SIGTERM && exitsig != SIGKILL) {
			log(LOG_WARNING,
				"async_daemon (pid %d) exiting on signal %d\n",
				u.u_procp->p_pid, exitsig);
		}
		exit(0);
	}

	for (;;) {
		async_daemon_ready++;
		while ((bp = async_bufhead.b_actf) == &async_bufhead) {
			(void) sleep((caddr_t)&async_bufhead, PZERO + 1);
		}
		bremfree(bp);
		rp = vtor(bp->b_vp);
		if (do_bio(bp) == NFS_CACHEINVALERR) {
			nfs_purge_caches(rtov(rp));
		}
	}
}

static int
do_bio(bp)
	register struct buf *bp;
{
	register struct rnode *rp = vtor(bp->b_vp);
	struct vattr va;
	long count;
	int error;
	int read, async;

	read = bp->b_flags & B_READ;
	async = bp->b_flags & B_ASYNC;
#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
"do_bio: addr %x, blk %ld, offset %ld, size %ld, B_READ %x B_ASYNC %x\n",
	    bp->b_un.b_addr, bp->b_blkno, dbtob(bp->b_blkno),
	    bp->b_bcount, read, async);
#endif

	if (read) {
		error = bp->b_error = nfsread(bp->b_vp, bp->b_un.b_addr,
		    (u_int)dbtob(bp->b_blkno), bp->b_bcount,
		    &bp->b_resid, rp->r_cred, &va);
		if (!error) {
			if (bp->b_resid) {
				/*
				 * Didn't get it all because we hit EOF,
				 * zero all the memory beyond the EOF.
				 */
				bzero(bp->b_un.b_addr +
				    (bp->b_bcount - bp->b_resid),
				    (u_int)bp->b_resid);
			}
			if (bp->b_resid == bp->b_bcount &&
			    dbtob(bp->b_blkno) >= rp->r_size) {
				/*
				 * We didn't read anything at all as we are
				 * past EOF.  Return an error indicator back
				 * but don't destroy the pages (yet).
				 */
				error = NFS_EOF;
			}
		}
	} else {
		/*
		 * If the write fails all future writes will get
		 * an error until the file is closed or munmapped.
		 */
		/*
		 * Fix bug 1030884
		 * Completely gross hack to make writes to the middle
		 * of a file succeed if the remote filesystem is full.
		 * Hack also in place in nfs_rdwr().
		 */

		if (rp->r_error == 0 || rp->r_error == ENOSPC) {
			count = MIN(bp->b_bcount, rp->r_size -
			    dbtob(bp->b_blkno));
			if (count < 0) {
				panic("do_bio: write count < 0");
			}
			if (rp->r_error == ENOSPC &&
			    (count + dbtob(bp->b_blkno) >
				rp->r_attr.va_size))
			    error = bp->b_error = rp->r_error;
			else
			    rp->r_error = error = bp->b_error = nfswrite(bp->b_vp,
			    bp->b_un.b_addr, (u_int)dbtob(bp->b_blkno),
			    count, rp->r_cred);
		} else
			error = bp->b_error = rp->r_error;
	}

	if (!error && read) {
		if (!CACHE_VALID(rp, va.va_mtime, va.va_size)) {
			/*
			 * read, if cache is not valid mark this bp
			 * with an error so it will be freed by pvn_done
			 * and return a special error, NFS_CACHEINVALERR,
			 * so caller can flush caches and re-do the operation.
			 */
			error = NFS_CACHEINVALERR;
			bp->b_error = EINVAL;
		} else {
			nfs_attrcache_va(rtov(rp), &va);
		}
	}

	if (error != 0 && error != NFS_EOF) {
		bp->b_flags |= B_ERROR;
	}

	/*
	 * Call pvn_done() to free the bp and pages.  If not ASYNC
	 * then we have to call pageio_done() to free the bp.
	 */
	pvn_done(bp);
	if (!async) {
		pageio_done(bp);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "do_bio: error %d, bp %x B_READ %x B_ASYNC %d\n",
	    error, bp, read, async);
#endif
	return (error);
}

static int
nfs_noop()
{

	return (EREMOTE);
}

/*
 * Record-locking requests are passed to the local Lock-Manager daemon.
 */
static int
nfs_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	int clid;
{
	lockhandle_t lh;
	struct eflock eld;
	int error;

	ASSERT(sizeof (lh.lh_id) == sizeof (fhandle_t));

	VFS_RECORD(vp->v_vfsp, VS_LOCKCTL, VS_CALL);

	/*
	 * If we are setting a lock mark the vnode VNOCACHE so the page
	 * cache does not give inconsistent results on locked files shared
	 * between clients.  The VNOCACHE flag is never turned off as long
	 * as the vnode is active because it is hard to figure out when the
	 * last lock is gone.
	 * XXX - what if some already has the vnode mapped in?
	 */
/*	if (((vp->v_flag & VNOCACHE) == 0) &&
			Fix bug 1052330 */
	if ((ld->l_type != F_UNLCK) && (cmd != F_GETLK)) {
		vp->v_flag |= VNOCACHE;
		(void)nfs_putpage(vp, 0, 0, B_INVAL, cred);
		PURGE_ATTRCACHE(vp);
	}
	lh.lh_vp = vp;
	lh.lh_servername = vtomi(vp)->mi_hostname;
	bcopy((caddr_t)vtofh(vp), (caddr_t)&lh.lh_id, sizeof (fhandle_t));
	eld.l_type = ld->l_type;
	eld.l_whence = ld->l_whence;
	eld.l_start = ld->l_start;
	eld.l_len = ld->l_len;
	eld.l_pid = ld->l_pid;
	eld.l_xxx = ld->l_xxx;
	error = klm_lockctl(&lh, &eld, cmd, cred, clid);
	if (cmd == F_GETLK) {
		ld->l_type = eld.l_type;
		if (eld.l_type != F_UNLCK) {
			ld->l_whence = eld.l_whence;
			ld->l_start = eld.l_start;
			ld->l_len = eld.l_len;
			ld->l_pid = eld.l_pid;
			ld->l_xxx = eld.l_xxx;
		}
	}
	return (error);
}

int nfs_nra = 1;	/* number of pages to read ahead */
int nfs_lostpage;	/* number of times we lost original page */

/*
 * Called from pvn_getpages or nfs_getpage to get a particular page.
 * When we are called the rnode has already locked by nfs_getpage.
 */
/*ARGSUSED*/
static int
nfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	u_int off;
	u_int *protp;
	struct page *pl[];		/* NULL if async IO is requested */
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	register struct rnode *rp;
	register u_int bsize;
	struct buf *bp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn;
	u_int io_off, io_len;
	u_int blksize, blkoff;
	int err;
	int readahead;

	rp = vtor(vp);
#ifdef NFSDEBUG
	dprint(nfsdebug, 4,
	    "nfs_getapage: vp %x size %d off %d pl %x addr %x\n",
	    vp, rp->r_size, off, pl, addr);
#endif
	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);

	bsize = vp->v_vfsp->vfs_bsize;
reread:
	err = 0;
	lbn = off / bsize;
	blkoff = lbn * bsize;

	if (rp->r_nextr == off && !(vp->v_flag & VNOCACHE)) {
		readahead = nfs_nra;
	} else {
		readahead = 0;
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 1, "nfs_getapage: nextr %d off %d size %d ra %d ",
	    rp->r_nextr, off, rp->r_size, readahead);
#endif

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Need to go to server to get a block
		 */

		if (blkoff < rp->r_size && blkoff + bsize > rp->r_size) {
			/*
			 * If less than a block left in
			 * file read less than a block.
			 */
			if (rp->r_size <= off) {
				/*
				 * Trying to access beyond EOF,
				 * set up to get at least one page.
				 */
				blksize = off + PAGESIZE - blkoff;
			} else {
				blksize = rp->r_size - blkoff;
			}
		} else {
			blksize = bsize;
		}

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
		    blkoff, blksize, 0);
		/*
		 * Somebody has entered the page before us, so
		 * just use it.
		 */
		if (pp == NULL)
			goto again;

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
					ASSERT(pp2->p_next->p_offset !=
					    pp->p_offset);
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

		/*
		 * Now round the request size up to page boundaries.
		 * This insures that the entire page will be
		 * initialized to zeroes if EOF is encountered.
		 */
		io_len = ptob(btopr(io_len));

		bp = pageio_setup(pp, io_len, vp, pl == NULL ?
		    (B_ASYNC | B_READ) : B_READ);

		bp->b_blkno = btodb(io_off);
		bp->b_dev = 0;
		bp_mapin(bp);

		/*
		 * If doing a write beyond what we believe is EOF,
		 * don't bother trying to read the pages from the
		 * server, we'll just zero the pages here.  We
		 * don't check that the rw flag is S_WRITE here
		 * because some implementations may attempt a
		 * read access to the buffer before copying data.
		 */
		if (io_off >= rp->r_size && seg == segkmap) {
			bzero(bp->b_un.b_addr, io_len);
			pvn_done(bp);
			if (pl != NULL)
				pageio_done(bp);
		} else {
			err = nfs_strategy(bp);
		}
		/* bp is now invalid! */

		if (err == NFS_EOF) {
			/*
			 * If doing a write system call just return
			 * zeroed pages, else user tried to get pages
			 * beyond EOF, return error.  We don't check
			 * that the rw flag is S_WRITE here because
			 * some implementations may attempt a read
			 * access to the buffer before copying data.
			 */
			if (seg == segkmap) {
				err = 0;
			} else {
				err = EFAULT;
			}
		}

		rp->r_nextr = io_off + io_len;
		u.u_ru.ru_majflt++;
		if (seg == segkmap)
			u.u_ru.ru_inblock++;	/* count as `read' operation */
		cnt.v_pgin++;
		cnt.v_pgpgin += btopr(io_len);
#ifdef NFSDEBUG
		dprint(nfsdebug, 1, "OTW\n");
#endif
	}

	while (!err && readahead > 0 && (blkoff + bsize < rp->r_size)) {
		addr_t addr2;

		readahead--;
		lbn++;
		blkoff += bsize;
		addr2 = addr + (blkoff - off);

		if (blkoff < rp->r_size && blkoff + bsize > rp->r_size) {
			/*
			 * If less than a block left in
			 * file read less than a block.
			 */
			blksize = rp->r_size - blkoff;
		} else {
			blksize = bsize;
		}

		/*
		 * If addr is now in a different seg,
		 * don't bother trying with read-ahead.
		 */
		if (addr2 >= seg->s_base + seg->s_size) {
			pp2 = NULL;
#ifdef NFSDEBUG
			dprint(nfsdebug, 1, "nfs_getapage: ra out of seg\n");
#endif
		} else {
			VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);
			pp2 = pvn_kluster(vp, blkoff, seg, addr2,
			    &io_off, &io_len, blkoff, blksize, 1);
#ifdef NFSDEBUG
			if (pp2 == NULL) {
				dprint(nfsdebug, 1,
				    "nfs_getapage: RA CACHE off %d size %d\n",
				    off, rp->r_size);
			}
#endif
		}

		if (pp2 != NULL) {
			/*
			 * Now round the request size up to page boundaries.
			 * This insures that the entire page will be
			 * initialized to zeroes if EOF is encountered.
			 */
			io_len = ptob(btopr(io_len));

			bp = pageio_setup(pp2, io_len, vp, B_READ | B_ASYNC);

			bp->b_dev = 0;
			bp->b_blkno = btodb(io_off);
			bp_mapin(bp);

#ifdef NFSDEBUG
			dprint(nfsdebug, 1,
			    "nfs_getapage: RA OTW off %d size %d\n",
			    off, rp->r_size);
#endif
			err = nfs_strategy(bp);	/* bp is now invalid! */

			/*
			 * Ignore all read ahead errors except those
			 * that might invalidate the primary read.
			 */
			if (err != NFS_EOF && err != NFS_CACHEINVALERR) {
				err = 0;
			}

			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
	}

	if (pagefound != NULL) {
		register int s;
#ifdef NFSDEBUG
		dprint(nfsdebug, 1, "CACHE\n");
#endif
		/*
		 * We need to be careful here because if the page was
		 * previously on the free list, we might have already
		 * lost it at interrupt level.
		 */
		s = splvm();
		if (pagefound->p_vnode == vp && pagefound->p_offset == off) {
			/*
			 * If the page is intransit or if
			 * it is on the free list call page_lookup
			 * to try and wait for / reclaim the page.
			 */
			if (pagefound->p_intrans || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			(void) splx(s);
			nfs_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			u.u_ru.ru_minflt++;
			rp->r_nextr = off + PAGESIZE;
		}
		(void) splx(s);
	}

	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

#ifdef NFSDEBUG
	dprint(nfsdebug, 5, "nfs_getapage: returning %d\n", err);
#endif
	return (err);
}

/*
 * Return all the pages from [off..off+len) in file
 */
static int
nfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
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
	struct rnode *rp = vtor(vp);
	int err;

	if (protp != NULL)
		*protp = PROT_ALL;

	RLOCK(rp);

	if (rp->r_cred == NULL) {
		if (cred == NULL) {
			cred = u.u_cred;	/* XXX need real cred! */
		}
		crhold(cred);
		rp->r_cred = cred;
	}

	/*
	 * Now valididate that the caches are up to date.
	 */
	(void) nfs_validate_caches(vp, rp->r_cred);

	/*
	 * If we are getting called as a side effect of a nfs_rdwr()
	 * write operation the local file size might not be extended yet.
	 * In this case we want to be able to return pages of zeroes.
	 */
	if (off + len > rp->r_size + PAGEOFFSET && seg != segkmap) {
		RUNLOCK(rp);
		return (EFAULT);		/* beyond EOF */
	}
retry:

	if (len <= PAGESIZE)
		err = nfs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	else
		err = pvn_getpages(nfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cred);

 
	switch (err) {
	case NFS_CACHEINVALERR:
	case NFS_EOF:
		nfs_purge_caches(vp);
		goto retry;
	case ESTALE:
		PURGE_STALE_FH(err, vp);
	}
	RUNLOCK(rp);

	return (err);
}

/*ARGSUSED*/
static int
nfs_putpage(vp, off, len, flags, cred)
	struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct ucred *cred;
{
	register struct rnode *rp;
	register struct page *pp;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	daddr_t lbn;
	u_int lbn_off;
	u_int bsize;
	int vpcount;
	int err = 0;

	if (len == 0 && (flags & B_INVAL) == 0 &&
	    (vp->v_vfsp->vfs_flag & VFS_RDONLY)) {
		return (0);
	}

	rp = vtor(vp);
	if ((vp->v_pages == NULL) || (vp->v_type == VCHR) ||
	(vp->v_type == VSOCK))
                       /* Fix bug 1047557, huey 
                        * The reason this condition is taken out
			* is because it's possible that the rnode
                        * doesn't have the attributes up to date.
		        * Thus old pages might not be deleted. */
                       /*     || (off >= rp->r_size)) */
		return (0);

	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_CALL);

	bsize = MAX(vp->v_vfsp->vfs_bsize, PAGESIZE);
	vpcount = vp->v_count;
	if (vp->v_count == 0) {
		((struct mntinfo *)(vp->v_vfsp->vfs_data))->mi_refct++;
	}
	VN_HOLD(vp);

again:
	if (len == 0) {
		/*
		 * We refuse to act on behalf of the pageout daemon to push
		 * out a page to a rnode which is currently locked.
		 */
		if ((rp->r_flags & RLOCKED) && u.u_procp == &proc[2]) {
			err = EWOULDBLOCK;		/* XXX */
			goto out;
		}

		/*
		 * Search the entire vp list for pages >= off
		 */
		RLOCK(rp);
		dirty = pvn_vplist_dirty(vp, off, flags);
		if (dirty == NULL && off == 0 && (flags & B_ASYNC) == 0) {
			/*
			 * No dirty pages over the whole vnode, clear RDIRTY
			 * flag. This is the only safe place to do this since
			 * there is a possibility that we will sleep flushing
			 * the pages in a non-NULL list, and someone else
			 * could come in an write another ASYNC block.
			 */
			rp->r_flags &= ~RDIRTY;
		}
		RUNLOCK(rp);
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to bsize boundaries.
		 */
		if (off >= rp->r_size) {
			dirty = NULL;
		} else {
			u_int fsize, eoff, offlo, offhi;

			fsize = (rp->r_size + PAGEOFFSET) & PAGEMASK;
			eoff = MIN(off + len, fsize);
			offlo = (off / bsize) * bsize;
			offhi = roundup(eoff, bsize);
			dirty = pvn_range_dirty(vp, off, eoff, offlo, offhi,
			    flags);
		}
	}

	/*
	 * Destroy read ahead value (since we are really going to write)
	 * and save credentials for async writes.
	 */
	if (dirty != NULL) {
		rp->r_nextr = 0;
		if (rp->r_cred == NULL) {
			if (cred == NULL) {
				cred = u.u_cred; /* XXX need real cred! */
			}
			crhold(cred);
			if (rp->r_cred) {
				crfree(rp->r_cred);
			}
			rp->r_cred = cred;
		}
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  It will also handle invalidation and freeing
	 * of pages that are not dirty.  All the pages on the list
	 * returned need to still be dealt with here.
	 */

	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk that fits in one lbn
		 */
		io_off = pp->p_offset;
		lbn = io_off / bsize;

		page_sub(&dirty, pp);
		io_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn * bsize;

		while (dirty != NULL && dirty->p_offset < lbn_off + bsize &&
		    dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/*
		 * Check for page length rounding problems
		 */
		if (io_off + io_len > lbn_off + bsize) {
			ASSERT((io_off+io_len) - (lbn_off+bsize) < PAGESIZE);
			io_len = lbn_off + bsize - io_off;
		}

		err = nfs_writelbn(rp, io_list, io_off, io_len, flags);
		if (err)
			break;
	}

	if (err != 0) {
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && (len == 0 || len >= rp->r_size)) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
		    ((vp->v_pages != NULL) && (vp->v_pages->p_lckcnt == 0)))
			goto again;
	}

out:
	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */

	if (--vp->v_count == 0) {
		if (vpcount > 0) {
			(void) nfs_inactive(vp, rp->r_cred);
		} else {
			((struct mntinfo *)(vp->v_vfsp->vfs_data))->mi_refct--;
		}
	}
	return (err);
}

/*ARGSUSED*/
static int
nfs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
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

	/*
	 * Check to see if the vnode is currently marked as not cachable.
	 * If so, we have to refuse the map request as this violates the
	 * don't cache attribute.
	 */
	if (vp->v_flag & VNOCACHE)
		return (EIO);

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
nfs_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{

	VFS_RECORD(vp1->v_vfsp, VS_CMP, VS_CALL);
	return (vp1 == vp2);
}

/*ARGSUSED*/
static int
nfs_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{

	VFS_RECORD(vp->v_vfsp, VS_REALVP, VS_CALL);
	return (EINVAL);
}

/*ARGSUSED*/
static int
nfs_cntl(vp, cmd, idata, odata, iflag, oflag)
	struct vnode *vp;
	int cmd, iflag, oflag;
	caddr_t idata, odata;

{
	int error = 0;

	/*
	 * This looks a little weird because it's written in a general
	 * manner but we make use of only one case.  If cntl() ever gets
	 * widely used, the outer switch will make more sense.
	 */
	switch (cmd) {
	default:
		return (EINVAL);

	case _PC_LINK_MAX:
	case _PC_NAME_MAX:
	case _PC_PATH_MAX:
	case _PC_CHOWN_RESTRICTED:
	case _PC_NO_TRUNC: {
		struct mntinfo *mi;
		struct pathcnf *pc;

		if (!(mi = vtomi(vp)) || !(pc = mi->mi_pathconf))
			return (EINVAL);
		ASSERT(oflag == CNTL_INT32);
		error = _PC_ISSET(cmd, pc->pc_mask);	/* error or bool */
		switch (cmd) {
		case _PC_LINK_MAX:
			*(int*)odata = pc->pc_link_max;
			break;
		case _PC_NAME_MAX:
			*(int*)odata = pc->pc_name_max;
			break;
		case _PC_PATH_MAX:
			*(int*)odata = pc->pc_path_max;
			break;
		case _PC_CHOWN_RESTRICTED:
			*(int*)odata = error;	/* see above */
			break;
		case _PC_NO_TRUNC:
			*(int*)odata = error;	/* see above */
			break;
		}
		return (error ? EINVAL : 0);
	    }
	}
}
