#ident  "@(#)ufs_vnodeops.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/mman.h>
#include <sys/pathname.h>
#include <sys/debug.h>
#include <sys/vmmeter.h>
#include <sys/trace.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/filio.h>		/* FIOLFS */
#include <sys/vaccess.h>	/* FIOLFS */
#include <sys/lockfs.h>		/* FIOLFS */
#include <sys/filai.h>		/* FIOAI */

#include <specfs/fifo.h>
#include <ufs/fs.h>
#include <ufs/inode.h>
#include <ufs/mount.h>
#include <ufs/fsdir.h>
#include <ufs/lockf.h>		/* Defines constants for the locking code */
#include <ufs/lockfs.h>		/* FIOLFS */
#ifdef	QUOTA
#include <ufs/quota.h>
#endif
#include <sys/dirent.h>		/* must be AFTER <ufs/fsdir>! */

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <vm/swap.h>

#include <krpc/lockmgr.h>

#define	ISVDEV(t) ((t == VCHR) || (t == VBLK) || (t == VFIFO))

static	int ufs_open();
static	int ufs_close();
static	int ufs_rdwr();
static	int ufs_ioctl();
static	int ufs_select();
static	int ufs_getattr();
static	int ufs_setattr();
static	int ufs_access();
static	int ufs_lookup();
static	int ufs_create();
static	int ufs_remove();
static	int ufs_link();
static	int ufs_rename();
static	int ufs_mkdir();
static	int ufs_rmdir();
static	int ufs_readdir();
static	int ufs_symlink();
static	int ufs_readlink();
static	int ufs_fsync();
static	int ufs_inactive();
static	int ufs_lockctl();
static	int ufs_fid();
static	int ufs_getpage();
static	int ufs_putpage();
static	int ufs_map();
static  int ufs_cmp();
static  int ufs_realvp();
static	int ufs_cntl();
static	int ufs_badop();

/*
 * ulockfs intercepts
 *	Substituted for normal VOP entry points in ufs_vnodeops below
 */
static	int ufs_l_open();
static	int ufs_l_close();
static	int ufs_l_rdwr();
static	int ufs_l_select();
static	int ufs_l_getattr();
static	int ufs_l_setattr();
static	int ufs_l_access();
static	int ufs_l_lookup();
static	int ufs_l_create();
static	int ufs_l_remove();
static	int ufs_l_link();
static	int ufs_l_rename();
static	int ufs_l_mkdir();
static	int ufs_l_rmdir();
static	int ufs_l_readdir();
static	int ufs_l_symlink();
static	int ufs_l_readlink();
static	int ufs_l_fsync();
static	int ufs_l_inactive();
static	int ufs_l_lockctl();
static	int ufs_l_fid();
static	int ufs_l_getpage();
static	int ufs_l_putpage();
static	int ufs_l_map();
static	int ufs_l_cntl();


/*
 * Replace standard entries with ulockfs intercepts
 */
struct vnodeops ufs_vnodeops = {
	ufs_l_open,
	ufs_l_close,
	ufs_l_rdwr,
	ufs_ioctl,
	ufs_l_select,
	ufs_l_getattr,
	ufs_l_setattr,
	ufs_l_access,
	ufs_l_lookup,
	ufs_l_create,
	ufs_l_remove,
	ufs_l_link,
	ufs_l_rename,
	ufs_l_mkdir,
	ufs_l_rmdir,
	ufs_l_readdir,
	ufs_l_symlink,
	ufs_l_readlink,
	ufs_l_fsync,
	ufs_l_inactive,
	ufs_l_lockctl,
	ufs_l_fid,
	ufs_l_getpage,
	ufs_l_putpage,
	ufs_l_map,
	ufs_badop,		/* dump */
	ufs_cmp,
	ufs_realvp,
	ufs_l_cntl,
};

/*
 * FORCED UNMOUNT ENTRY POINTS
 * 	Alternate vnodeops branch table substitued for ufs_vnodeops
 */
static	int ufs_eio();
static	int ufs_f_close();
static	int ufs_f_inactive();

struct vnodeops ufs_forcedops = {
	ufs_eio,		/* ufs_open, */
	ufs_f_close,
	ufs_eio,		/* ufs_rdwr, */
	ufs_eio,		/* ufs_ioctl, */
	ufs_eio,		/* ufs_select, */
	ufs_eio,		/* ufs_getattr, */
	ufs_eio,		/* ufs_setattr, */
	ufs_eio,		/* ufs_access, */
	ufs_eio,		/* ufs_lookup, */
	ufs_eio,		/* ufs_create, */
	ufs_eio,		/* ufs_remove, */
	ufs_eio,		/* ufs_link, */
	ufs_eio,		/* ufs_rename, */
	ufs_eio,		/* ufs_mkdir, */
	ufs_eio,		/* ufs_rmdir, */
	ufs_eio,		/* ufs_readdir, */
	ufs_eio,		/* ufs_symlink, */
	ufs_eio,		/* ufs_readlink, */
	ufs_eio,		/* ufs_fsync, */
	ufs_f_inactive,
	ufs_eio,		/* ufs_lockctl, */
	ufs_eio,		/* ufs_fid, */
	ufs_eio,		/* ufs_getpage, */
	ufs_eio,		/* ufs_putpage, */
	ufs_eio,		/* ufs_map, */
	ufs_badop,		/* dump */
	ufs_cmp,
	ufs_eio,		/* ufs_realvp, */
	ufs_eio,		/* ufs_cntl, */
};

/*
 * FORCED UNMOUNT VOP ROUTINES
 *	VOP calls for inodes belonging to forcibly unmounted file systems
 *	enter one of the following routines.
 */
static int
ufs_eio()
{
	return (EIO);
}
/*ARGSUSED*/
static int
ufs_f_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	return (0);
}
/*ARGSUSED*/
static int
ufs_f_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	iinactive(VTOI(vp));
	return (0);
}

/*
 * ULOCKFS MACROS
 */
/*
 * ulockfs intercept routines surround the normal ufs VOP call with a
 * locking wrapper by using the following wrapper macro
 */
#define	ULOCKFS(VP, VAID, VOPCALL)			\
{							\
	int		reterr;				\
	struct mount	*mp;				\
							\
	if (reterr = ufs_lockfs_begin(VP, VAID, &mp))	\
		return (reterr);			\
	reterr = (VOPCALL);				\
	ufs_lockfs_end(VAID, mp);			\
	return (reterr);				\
}

int lock(), unlock();
void test_lock(), kill_proc_locks();

/*ARGSUSED*/
static int
ufs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
	register int error, cmd;
	register struct inode *ip;
	struct eflock ld;	/* Holder for an I/O lock */

	VFS_RECORD((*vpp)->v_vfsp, VS_OPEN, VS_CALL);
	ip = VTOI(*vpp);
	/*
	 * Mandatory file and record locking stuff. MFRL is enforced
	 * when the SGID bit is set and the XGRP bit is reset (hey I
	 * didn't come up with this scheme!) When enabled reads and
	 * writes are checked to see if they will 'violate' an existing
	 * lock on the file. Failure modes are determined by the state
	 * of the O_NDELAY flag on the file descriptor, when set the
	 * error EAGAIN is returned when reset the process blocks until
	 * there are no blocking locks. In either case if a dead lock
	 * would occur EDEADLK is returned.
	 */
	if ((((ip->i_mode) & ISGID) != 0) && ((ip->i_mode & IFMT) == IFREG)) {
		ld.l_type   = F_WRLCK;
		ld.l_start  = 0;
		ld.l_len    = 0x7fffffff;
		ld.l_whence = 0;
		cmd = F_SETLK;
		/* XXX need a better way to get pid */
		if ((error = lock(*vpp, &ld, cmd, u.u_procp->p_pid, IO_LOCK)) !=
0) {
			/* to make it SVID compliant return EAGAIN */
			if (error == EACCES)
				error = EAGAIN;
			return (error);
		}
	} else {
		cmd = 0;
	}
	if (cmd) {
		ld.l_type = F_UNLCK;
		cmd = F_SETLK;
		/* XXX need a better way to get pid */
		(void) unlock(*vpp, &ld, cmd, u.u_procp->p_pid, IO_LOCK);
	}
	return (0);
}

/*ARGSUSED*/
static int
ufs_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{
	VFS_RECORD(vp->v_vfsp, VS_CLOSE, VS_CALL);
	return (0);
}


/*
 * read or write a vnode
 */
/*ARGSUSED*/
static int
ufs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;
	int didlock;		/* TRUE if the inode was locked. */
	int cmd;		/* I/O lock command, zero if no lock */
	struct eflock ld;	/* Holder for an I/O lock */

	ip = VTOI(vp);

	/*
	 * Mandatory file and record locking stuff. MFRL is enforced
	 * when the SGID bit is set and the XGRP bit is reset (hey I
	 * didn't come up with this scheme!) When enabled reads and
	 * writes are checked to see if they will 'violate' an existing
	 * lock on the file. Failure modes are determined by the state
	 * of the O_NDELAY flag on the file descriptor, when set the
	 * error EAGAIN is returned when reset the process blocks until
	 * there are no blocking locks. In either case if a dead lock
	 * would occur EDEADLK is returned.
	 */
	if (((ip->i_mode & ISGID) != 0) && ((ip->i_mode & IFMT) == IFREG) &&
	    ((ip->i_mode & (IEXEC >> 3)) == 0)) {
		ld.l_type   = (rw == UIO_WRITE) ? F_WRLCK : F_RDLCK;
		ld.l_start  = uiop->uio_offset;
		ld.l_len    = uiop->uio_resid;
		ld.l_whence = 0;
		cmd = (ioflag & IO_NDELAY) ? F_SETLK : F_SETLKW;
		/* XXX need a better way to get pid */
		error = lock(vp, &ld, cmd, u.u_procp->p_pid, IO_LOCK);
		if (error != 0) {
			/* to make it SVID compliant return EAGAIN */
			if (error == EACCES)
				error = EAGAIN;
			return (error);
		}
	} else
		cmd = 0;	/* No lock set */

	if ((ioflag & IO_APPEND) != 0 && (rw == UIO_WRITE) &&
	    (ip->i_mode & IFMT) == IFREG) {
		/*
		 * In append mode start at end of file after locking it.
		 */
		didlock = 1;
		ILOCK(ip);
		uiop->uio_offset = ip->i_size;
	} else
		didlock = 0;

	error = rwip(ip, uiop, rw, ioflag);
	ITIMES(ip);

	if (didlock)
		IUNLOCK(ip);
	if (cmd) {
		ld.l_type = F_UNLCK;
		cmd = F_SETLK;
		/* XXX need a better way to get pid */
		(void) unlock(vp, &ld, cmd, u.u_procp->p_pid, IO_LOCK);
	}

	return (error);
}

/*
 * Don't cache write blocks to files with the sticky bit set.
 * Used to keep swap files from blowing the page cache on a server.
 */
int	stickyhack = 1;

/*
 * Bytes / inode allowed in the disk queue.
 */
int	ufs_WRITES = 512 * 1024;	

#ifdef MULTIPROCESSOR 
/*
 * release the kernel lock during uiomove.
 */
int     ufs_uiomove_nolock = 1; 
#endif
 
/*
 * prefault segmkmap mapping in rwip to avoid traps.
 */
int	dogetmapflt = 1;

/*
 * The idea behind the freebehind stuff is this:
 * We want caching but we don't want large i/o's to blow everything else
 * out.  Furthermore, it is more expensive (cpu wise) to wait for the
 * page to free up memory; it's faster to have the process do free up
 * it's own memory.
 *
 * The knobs associated with this stuff are:
 *
 * freebehind		on/off switch for both read and write
 * write_free		on/off for unconditional free's upon write completion
 * pages_before_pager	the pager turns on at 'lotsfree'; we turn on at
 *			'lotsfree + pages_before_pager'.  This wants to be
 *			at least a clusters worth.
 * smallfile		don't free behind at offsets less than this.
 */
int	freebehind = 1;
int	pages_before_pager = 30;    /* 1 cluster on a sun4c, 2 on all others */
int	write_free = 0;
int	smallfile = 32 * 1024;

/*
 * rwip does the real work of read or write requests for ufs.
 */
static int
rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	register u_int off;
	register addr_t base;
	register int n, on, mapon;
	register struct fs *fs;
	struct vnode *vp;
	int type, error, pagecreate;
	u_int flags;
	int iupdat_flag;
	long old_blocks;
	int adjust_resid = 0;
	int dofree;
	extern int freemem, lotsfree, pages_before_pager;
	int orig_resid = 0;
	int last = 0;
#ifdef MULTIPROCESSOR
	int klock_released;
#endif
	extern caddr_t segmap_getmapflt();

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwip");
	type = ip->i_mode & IFMT;
	if (type != IFREG && type != IFDIR && type != IFLNK)
		panic("rwip type");
	if (uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0)
		return (EINVAL);
	if (uio->uio_resid == 0)
		return (0);

	trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
	    TRC_RWIP_ENTER);

	ILOCK(ip);

	if (rw == UIO_WRITE) {
		if (type == IFREG && uio->uio_offset + uio->uio_resid >
		    u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
			if (uio->uio_offset >=
					u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
				psignal(u.u_procp, SIGXFSZ);
				error = EFBIG;
				goto out;
			} else {
				adjust_resid = uio->uio_resid;
				uio->uio_resid =
			u.u_rlimit[RLIMIT_FSIZE].rlim_cur - uio->uio_offset;
				adjust_resid -= uio->uio_resid;
			}
		}
		ip->i_flag |= INOACC;	/* don't update ref time in getpage */
	} else {
		if (!ULOCKFS_IS_NOIACC(ITOU(ip)))
			ip->i_flag |= IACC;
	}

	if (ioflag & IO_SYNC) {
		ip->i_flag |= ISYNC;
		old_blocks = ip->i_blocks;
		iupdat_flag = 0;
	}
	fs = ip->i_fs;
	vp = ITOV(ip);
	do {
		off = uio->uio_offset & MAXBMASK;
		mapon = uio->uio_offset & MAXBOFFSET;
		on = blkoff(fs, uio->uio_offset);
		n = MIN(fs->fs_bsize - on, uio->uio_resid);

		if (rw == UIO_READ) {
			int diff = ip->i_size - uio->uio_offset;

			VFS_RECORD(ITOV(ip)->v_vfsp, VS_READ, VS_CALL);
			if (diff <= 0) {
				error = 0;
				goto out;
			}
			if (diff < n)
				n = diff;
			dofree = freebehind &&
			    ip->i_nextr == (off & PAGEMASK) && off > smallfile;
		} else {
			int s;

			/*
			 * Limit the amount of memory that this inode can use
			 * Protected because the count is modified at interrupt
			 * level.
			 */
			s = splbio();
			while (ufs_WRITES && ip->i_writes > ufs_WRITES) {
				(void) sleep((caddr_t)&ip->i_writes, PZERO);
			}
			(void) splx(s);
			VFS_RECORD(ITOV(ip)->v_vfsp, VS_WRITE, VS_CALL);
		}

 		if ( dogetmapflt && rw == UIO_READ 
 		&& off + MAXBSIZE <= ip->i_size )
 			base = segmap_getmapflt(segkmap, vp, off);
 		else
 			base = segmap_getmap(segkmap, vp, off);

		trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
		    TRC_RWIP_GETMAP);

		if (rw == UIO_WRITE) {
			if (uio->uio_offset + n > ip->i_size) {
				/*
				 * We are extending the length of the file.
				 * bmap is used so that we are sure that
				 * if we need to allocate new blocks, that it
				 * is done here before we up the file size.
				 */
				error = bmap_write(ip,
				    (daddr_t)lblkno(fs, uio->uio_offset),
				    0, (daddr_t*)0, (int*)0,
				    (int)(on + n), mapon == 0);

				/*
				 * For Posix conformance.  If addition space
				 * can't be allocated, fill out the rest of the
				 * last block in the file.
				 */

				if (error &&  (on % fs->fs_fsize) ) {
					int avail;

					orig_resid = uio->uio_resid;
					avail = fs->fs_fsize - (on % fs->fs_fsize);
					n = uio->uio_resid = MIN(avail,n);
					orig_resid -= uio->uio_resid;
					error = 0;
				} 

				trace6(TR_UFS_RWIP, ip, uio, rw, ioflag,
				    uio->uio_offset, TRC_RWIP_BMAPALLOC);
				if (error) {
					(void) segmap_release(segkmap, base, 0);
					/*
					 * For Posix.  If the last write worked
					 * and now we're out of space return
					 * the number of bytes written.
					 */
					if (last > 0 && error == ENOSPC) 
						error = 0;
					break;
				}
				ip->i_size = uio->uio_offset + n;
				iupdat_flag = 1;
				/*
				 * If we are writing from the beginning of
				 * the mapping, we can just create the
				 * pages without having to read them.
				 */
				if (mapon == 0) {
					segmap_pagecreate(segkmap, base,
					    (u_int)n, 0);
					pagecreate = 1;
				} else
					pagecreate = 0;
			} else if (n == MAXBSIZE) {
				/*
				 * Going to do a whole mappings worth,
				 * so we can just create the pages w/o
				 * having to read them in.  But before
				 * we do that, we need to make sure any
				 * needed blocks are allocated first.
				 */
				error = bmap_write(ip,
				    (daddr_t)lblkno(fs, uio->uio_offset),
				    0, (daddr_t*)0, (int*)0, (int)(on + n), 1);
				trace6(TR_UFS_RWIP, ip, uio, rw, ioflag,
				    uio->uio_offset, TRC_RWIP_BMAPALLOC);
				/*
				 * For Posix conformance.  If addition space
				 * can't be allocated, fill out the rest of the
				 * last block in the file.
				 */

				if (error && (on % fs->fs_fsize)) {
					int avail;

					orig_resid = uio->uio_resid;
					avail = fs->fs_fsize - (on % fs->fs_fsize);
					n = uio->uio_resid = MIN(avail,n);
					orig_resid -= uio->uio_resid;
					error = 0;
				} 
				if (error) {
					(void) segmap_release(segkmap, base, 0);
					if (last > 0 && error == ENOSPC) 
						error = 0;
					break;
				}
				segmap_pagecreate(segkmap, base, (u_int)n, 0);
				pagecreate = 1;
			} else
				pagecreate = 0;
		} else
			pagecreate = 0;

#ifdef MULTIPROCESSOR
                if (ufs_uiomove_nolock && n >= 512) {
                        klock_exit();
                        klock_released = 1;
                } else
                        klock_released = 0;
                error = uiomove(base + mapon, n, rw, uio);
                if (klock_released)
                        klock_enter();
#else
                error = uiomove(base + mapon, n, rw, uio);
#endif

		if (pagecreate && uio->uio_offset <
		    roundup(off + mapon + n, PAGESIZE)) {
			/*
			 * We created pages w/o initializing them completely,
			 * thus we need to zero the part that wasn't set up.
			 * This happens on most EOF write cases and if
			 * we had some sort of error during the uiomove.
			 */
			int nzero, nmoved;

			nmoved = uio->uio_offset - (off + mapon);
			ASSERT(nmoved >= 0 && nmoved <= n);
			nzero = roundup(on + n, PAGESIZE) - nmoved;
			ASSERT(nzero > 0 && mapon + nmoved + nzero <= MAXBSIZE);
			(void) kzero(base + mapon + nmoved, (u_int)nzero);
		}
		trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
		    TRC_RWIP_UIOMOVE);

		if (error == 0) {
			int free;

			flags = 0;
			if (rw == UIO_WRITE) {
				if (write_free ||
				    (freebehind &&
				    freemem < lotsfree + pages_before_pager)) {
					free = SM_FREE;
				} else {
					free = 0;
				}
				/*
				 * Force write back for synchronous write cases.
				 */
				if ((ioflag & IO_SYNC) || type == IFDIR) {
					/*
					 * If the sticky bit is set but the
					 * execute bit is not set, we do a
					 * synchronous write back and free
					 * the page when done.  We set up swap
					 * files to be handled this way to
					 * prevent servers from keeping around
					 * the client's swap pages too long.
					 * XXX - there ought to be a better way.
					 */
					if (IS_SWAPVP(vp)) {
						flags = SM_WRITE | SM_FREE |
						    SM_DONTNEED;
					} else {
						iupdat_flag = 1;
						flags = SM_WRITE | free;
					}
				} else if (n + on == MAXBSIZE ||
				    IS_SWAPVP(vp)) {
					/*
					 * Have written a whole block.
					 * Start an asynchronous write and
					 * mark the buffer to indicate that
					 * it won't be needed again soon.
					 */
					flags = SM_WRITE | SM_ASYNC | free;
				}
				ip->i_flag |= IUPD | ICHG;
				if (u.u_ruid != 0 && (ip->i_mode & (IEXEC |
				    (IEXEC >> 3) | (IEXEC >> 6))) != 0) {
					/*
					 * Clear Set-UID & Set-GID bits on
					 * successful write if not super-user
					 * and at least one of the execute bits
					 * is set.  If we always clear Set-GID,
					 * mandatory file and record locking is
					 * unuseable.
					 */
					ip->i_mode &= ~(ISUID | ISGID);
				}
			} else if (rw == UIO_READ) {
				if (freebehind && dofree &&
				    freemem < lotsfree + pages_before_pager) {
					flags = SM_FREE | SM_DONTNEED;
				}
			}
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}
		trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
		    TRC_RWIP_RELEASE);

		/*
		 * For Posix conformance 
		 */
		if (orig_resid) {
			uio->uio_resid = orig_resid;
			if(error == ENOSPC) 
				error = 0;
			break;
		} 

		last = n;

	} while (error == 0 && uio->uio_resid > 0 && n != 0);

	/*
	 * If we are doing synchronous write the only time we should
	 * not be sync'ing the ip here is if we have the stickyhack
	 * activated, the file is marked with the sticky bit and
	 * no exec bit, the file length has not been changed and
	 * no new blocks have been allocated during this write.
	 */
	if ((ioflag & IO_SYNC) != 0 && rw == UIO_WRITE &&
	    (iupdat_flag != 0 || old_blocks != ip->i_blocks)) {
		iupdat(ip, 1);
		trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
		    TRC_RWIP_IUPDAT);
	}

out:
	ip->i_flag &= ~(ISYNC | INOACC);
	IUNLOCK(ip);
	if (!error && adjust_resid) {
		uio->uio_resid = adjust_resid;
		psignal(u.u_procp, SIGXFSZ);
	}
	trace6(TR_UFS_RWIP, ip, uio, rw, ioflag, uio->uio_offset,
	    TRC_RWIP_RETURN);
	return (error);
}

/*ARGSUSED*/
static int
ufs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	int	error;

	VFS_RECORD(vp->v_vfsp, VS_IOCTL, VS_CALL);

	switch (com) {
	case FIOLFS:
		/*
		 * file system locking
		 */
		if ((error = ufs_lockfs_hold(vp->v_vfsp)) == 0) {
			error = ufs_fiolfs(vp, (struct lockfs **)data);
			ufs_lockfs_rele(vp->v_vfsp);
		}
		break;
	case FIOLFSS:
		/*
		 * file system lock status
		 */
		if ((error = ufs_lockfs_hold(vp->v_vfsp)) == 0) {
			error = ufs_fiolfss(vp, (struct lockfs **)data);
			ufs_lockfs_rele(vp->v_vfsp);
		}
		break;
	case FIOFFS:
		/*
		 * file system flush (push w/invalidate)
		 */
		if ((error = ufs_lockfs_hold(vp->v_vfsp)) == 0) {
			error = ufs_fioffs(vp, (struct lockfs **)data);
			ufs_lockfs_rele(vp->v_vfsp);
		}
		break;
	case FIOAI:
		/*
		 * file allocation information
		 */
		ULOCKFS(vp, VA_GETATTR,
			ufs_fioai(vp, (struct filai **)data));
		/* NOTREACHED */
		break;
	case FIODUTIMES:
		/*
		 * file allocation information
		 */
		ULOCKFS(vp, VA_CHANGE,
			ufs_fiodutimes(vp, (struct timeval **)data));
		/* NOTREACHED */
		break;
	case FIODIO:
		/*
		 * file system meta/user data delayed io
		 */
		ULOCKFS(vp, VA_WRITE,
			ufs_fiodio(vp, (u_long **)data));
		/* NOTREACHED */
		break;
	case FIODIOS:
		/*
		 * file system meta/user data delayed io status
		 */
		ULOCKFS(vp, VA_READ,
			ufs_fiodios(vp, (u_long **)data));
		/* NOTREACHED */
		break;
	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

/*ARGSUSED*/
static int
ufs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

	VFS_RECORD(vp->v_vfsp, VS_SELECT, VS_CALL);
	return (EINVAL);
}

/*ARGSUSED*/
static int
ufs_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct inode *ip;

	VFS_RECORD(vp->v_vfsp, VS_GETATTR, VS_CALL);

	ip = VTOI(vp);
	/*
	 * Mark correct time in inode.
	 */
	ITIMES(ip);
	/*
	 * Copy from inode table.
	 */
	vap->va_type = IFTOVT(ip->i_mode);
	vap->va_mode = ip->i_mode;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_size = ip->i_size;
	vap->va_atime = ip->i_atime;
	vap->va_mtime = ip->i_mtime;
	vap->va_ctime = ip->i_ctime;
	vap->va_rdev = ip->i_rdev;
	vap->va_blocks = ip->i_blocks;

	switch (ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blocksize = MAXBSIZE;		/* was BLKDEV_IOSIZE */
		break;

	case IFCHR:
		vap->va_blocksize = MAXBSIZE;
		break;

	default:
		vap->va_blocksize = vp->v_vfsp->vfs_bsize;
		break;
	}
	return (0);
}

static int
ufs_setattr(vp, vap, cred)
	register struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct inode *ip;
	int chtime = 0;
	int error = 0;

	VFS_RECORD(vp->v_vfsp, VS_SETATTR, VS_CALL);

	/*
	 * Cannot set these attributes
	 */
	if ((vap->va_nlink != -1) || (vap->va_blocksize != -1) ||
	    (vap->va_rdev != -1) || (vap->va_blocks != -1) ||
	    (vap->va_fsid != -1) || (vap->va_nodeid != -1) ||
	    ((int)vap->va_type != -1)) {
		return (EINVAL);
	}

	ip = VTOI(vp);
	ILOCK(ip);
	/*
	 * Change file access modes.  Must be owner or su.
	 */
	if (vap->va_mode != (u_short)-1) {
		error = OWNER(cred, ip);
		if (error)
			goto out;
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		if (cred->cr_uid != 0) {
			if ((ip->i_mode & IFMT) != IFDIR)
/* DBE_FAST_OSYNC */
			    if (ip->i_mode & (IEXEC | (IEXEC>>3) | (IEXEC>>6)))
/* DBE_FAST_OSYNC */
					ip->i_mode &= ~ISVTX;
			if (!groupmember((int)ip->i_gid))
				ip->i_mode &= ~ISGID;
		}
		ip->i_flag |= ICHG;
	}
	/*
	 * To change file ownership, must be su.
	 * To change group ownership, must be su or owner and in target group.
	 * This is now enforced in chown1() below.
	 */
	if ((vap->va_uid != (uid_t)-1) || (vap->va_gid != (gid_t)-1)) {
		error = chown1(ip, vap->va_uid, vap->va_gid);
		if (error)
			goto out;
	}
	/*
	 * Truncate file.  Must have write permission (checked above vnode
	 * layer) and not be a directory.
	 */
	if (vap->va_size != (u_long)-1) {
		if ((ip->i_mode & IFMT) == IFDIR) {
			error = EISDIR;
			goto out;
		}
		if ((error = itrunc(ip, vap->va_size)) != 0) {
			goto out;
		}
	}
	/*
	 * Change file access or modified times.
	 */
	if (vap->va_atime.tv_sec != -1) {
		if (cred->cr_uid != ip->i_uid && cred->cr_uid != 0) {
			error = iaccess(ip, IWRITE);
			if (error)
				goto out;
		}
		ip->i_atime = vap->va_atime;
		chtime++;
	}
	if (vap->va_mtime.tv_sec != -1) {
		/*
		 * Allow SysV-compatible option to set access and
		 * modified times to the current time if root, owner,
		 * or write access.
		 *
		 * XXX - va_mtime.tv_usec == -1 flags this.
		 */
		if (cred->cr_uid != ip->i_uid && cred->cr_uid != 0) {
			error = iaccess(ip, IWRITE);
			if (error)
				goto out;
		}
		if (vap->va_mtime.tv_usec == -1) {
			ip->i_atime = time;
			ip->i_mtime = time;
		} else {
			ip->i_mtime = vap->va_mtime;
		}
		ip->i_flag |= IMODTIME;
		chtime++;
	}
	if (chtime) {
		ip->i_ctime = time;
		ip->i_flag |= IMOD;
	}
out:
	iupdat(ip, 1);			/* XXX - should be async for perf */
	IUNLOCK(ip);
	return (error);
}

/*
 * Perform chown operation on inode ip;
 * inode must be locked prior to call.
 */
static int
chown1(ip, uid, gid)
	register struct inode *ip;
	uid_t uid;
	gid_t gid;
{
#ifdef	QUOTA
	register long change;
#endif

	if (uid == (uid_t)-1)
		uid = ip->i_uid;
	if (gid == (gid_t)-1)
		gid = ip->i_gid;

	/*
	 * If:
	 *    1) not the owner of the file, or
	 *    2) trying to change the owner of the file, or
	 *    3) trying to change the group of the file to a group not in the
	 *	 process' group set,
	 * then must be super-user.
	 * Check super-user last, and use "suser", so that the accounting
	 * file's "used super-user privileges" flag is properly set.
	 */
	if ((u.u_uid != uid || uid != ip->i_uid || !groupmember((int)gid)) &&
	    !suser())
		return (EPERM);

#ifdef	QUOTA
	if (ip->i_uid == uid)		/* this just speeds things a little */
		change = 0;
	else
		change = ip->i_blocks;
	(void) chkdq(ip, -change, 1);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), ip, (int)ip->i_uid, 1);
	dqrele(ip->i_dquot);
#endif
	ip->i_uid = uid;
	ip->i_gid = gid;
	ip->i_flag |= ICHG;
	if (u.u_uid != 0)
		ip->i_mode &= ~(ISUID|ISGID);
#ifdef	QUOTA
	ip->i_dquot = getinoquota(ip);
	(void) chkdq(ip, change, 1);
	(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp), (struct inode *)NULL,
	    (int)uid, 1);
#endif
	return (0);
}

/*ARGSUSED*/
static int
ufs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;

	VFS_RECORD(vp->v_vfsp, VS_ACCESS, VS_CALL);

	ip = VTOI(vp);
	ILOCK(ip);
	error = iaccess(ip, mode);
	IUNLOCK(ip);
	return (error);
}

/*ARGSUSED*/
static int
ufs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register struct inode *ip;
	register int error;

	VFS_RECORD(vp->v_vfsp, VS_READLINK, VS_CALL);

	if (vp->v_type != VLNK)
		return (EINVAL);
	ip = VTOI(vp);
	if (ip->i_flag & IFASTSYMLNK) {
		ILOCK(ip);
		if (!ULOCKFS_IS_NOIACC(ITOU(ip)))
			ip->i_flag |= IACC;
		error = uiomove((caddr_t)&ip->i_db[1],
			(int) MIN(ip->i_size, uiop->uio_resid),
			UIO_READ, uiop);
		IUNLOCK(ip);
	} else {
		int size;	/* no. of byte read */
		caddr_t basep;  /* pointer to input data */
		ino_t ino;
		long  igen;

		ino = ip->i_number;
		igen = ip->i_gen;
		size = uiop->uio_resid;
		basep = uiop->uio_iov->iov_base;

		error = rwip(ip, uiop, UIO_READ, 0);
		if (error == 0 && ip->i_number == ino &&
			ip->i_gen == igen);
		else goto out;

		size -= uiop->uio_resid;
		if (ip->i_size <= FSL_SIZE && ip->i_size == size) {
			if (uiop->uio_segflg == UIO_USERSPACE ||
				uiop->uio_segflg == UIO_USERISPACE)
				error = copyin(basep,
					(caddr_t) &ip->i_db[1],
					(u_int) ip->i_size);
			else
				error = kcopy(basep,
					(caddr_t) &ip->i_db[1],
					(u_int) ip->i_size);
			if (error == 0) {
				ip->i_flag |= IFASTSYMLNK;
				/* free page */
				(void) VOP_PUTPAGE(ITOV(ip),
					(caddr_t) 0, PAGESIZE,
					(B_DONTNEED | B_FREE | B_FORCE |
					B_ASYNC), cred);
			} else {
				int i;
				/* error, clear garbage left behind */
				for (i = 1; i < NDADDR && ip->i_db[i]; i++)
					ip->i_db[i] = 0;
				for (i = 0; i < NIADDR && ip->i_ib[i]; i++)
					ip->i_ib[i] = 0;
			}
		}
	}
out:
	ITIMES(ip);
	return (error);

}

/*ARGSUSED*/
static int
ufs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	register struct inode *ip;
	int err;

	VFS_RECORD(vp->v_vfsp, VS_FSYNC, VS_CALL);

	ip = VTOI(vp);
	ILOCK(ip);
	err = syncip(ip, 0, 1);		/* do synchronous writes */
	if (!err)
		err = sync_indir(ip);	/* write back any other inode data */
	IUNLOCK(ip);
	return (err);
}

/*ARGSUSED*/
static int
ufs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

	VFS_RECORD(vp->v_vfsp, VS_INACTIVE, VS_CALL);

	iinactive(VTOI(vp));
	return (0);
}


/*
 * Unix file system operations having to do with directory manipulation.
 */
/*ARGSUSED*/
static int
ufs_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	register struct inode *ip;
	struct inode *xip;
	register int error;

	VFS_RECORD(dvp->v_vfsp, VS_LOOKUP, VS_CALL);

	ip = VTOI(dvp);
	error = dirlook(ip, nm, &xip);
	ITIMES(ip);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		if ((ip->i_mode & ISVTX) && !(ip->i_mode & (IEXEC | IFDIR)) &&
		    stickyhack) {
			(*vpp)->v_flag |= VISSWAP;
		} else {
			(*vpp)->v_flag &= ~VISSWAP;
		}
		ITIMES(ip);
		IUNLOCK(ip);
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

static int
ufs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	register struct inode *ip;
	struct inode *xip;

	VFS_RECORD(dvp->v_vfsp, VS_CREATE, VS_CALL);

	switch ((int) vap->va_type) {
		/* Must be super-user to create a non-FIFO special device */
		case (int) VBLK:
		case (int) VCHR:
			if (cred->cr_uid != 0)
				return (EPERM);
			else
				break;

		/* Can't create directories - use ufs_mkdir instead. */
		case (int) VDIR:
			return (EISDIR);
	}

	xip = (struct inode *)0;
	ip = VTOI(dvp);

	/* Must be super-user to set sticky bit */
	if (cred->cr_uid != 0)
		vap->va_mode &= ~VSVTX;

	error = direnter(ip, nm, DE_CREATE, (struct inode *)0,
	    (struct inode *)0, vap, &xip);
	ITIMES(ip);
	ip = xip;
	/*
	 * If file exists and this is a nonexclusive create,
	 * check for not directory and access permissions.
	 * If create/read-only an existing directory, allow it.
	 */
	if (error == EEXIST) {
		if (exclusive == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode) {
				error = iaccess(ip, mode);
			} else {
				error = 0;
			}
		}
		if (error) {
			iput(ip);
		} else if (((ip->i_mode&IFMT) == IFREG) && (vap->va_size == 0)){
			/*
			 * Truncate regular files, if required
			 */
			(void) itrunc(ip, (u_long)0);
		}
	}
	if (error) {
		return (error);
	}
	*vpp = ITOV(ip);
	ITIMES(ip);
	IUNLOCK(ip);
	/*
	 * If vnode is a device return special vnode instead
	 */
	if (ISVDEV((*vpp)->v_type)) {
		struct vnode *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type);
		VN_RELE(*vpp);
		*vpp = newvp;
	}

	if (vap != (struct vattr *)0) {
		(void) VOP_GETATTR(*vpp, vap, cred);
	}
	return (error);
}

/*ARGSUSED*/
static int
ufs_remove(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register int error;
	register struct inode *ip;

	VFS_RECORD(vp->v_vfsp, VS_REMOVE, VS_CALL);

	ip = VTOI(vp);
	error = dirremove(ip, nm, (struct inode *)0, 0);
	ITIMES(ip);
	return (error);
}

/*
 * Link a file or a directory.
 * If source is a directory, must be superuser.
 */
/*ARGSUSED*/
static int
ufs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	register struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{
	register struct inode *sip;
	register int error;
	struct vnode *realvp;

	if (VOP_REALVP(vp, &realvp) == 0) {
		vp = realvp;
	}

	VFS_RECORD(vp->v_vfsp, VS_LINK, VS_CALL);

	sip = VTOI(vp);
	if (((sip->i_mode & IFMT) == IFDIR) && !suser()) {
		return (EPERM);
	}
	error = direnter(VTOI(tdvp), tnm, DE_LINK,
	    (struct inode *)0, sip, (struct vattr *)0, (struct inode **)0);
	ITIMES(sip);
	ITIMES(VTOI(tdvp));
	return (error);
}

/*
 * Rename a file or directory.
 * We are given the vnode and entry string of the source and the
 * vnode and entry string of the place we want to move the source to
 * (the target). The essential operation is:
 *	unlink(target);
 *	link(source, target);
 *	unlink(source);
 * but "atomically".  Can't do full commit without saving state in the inode
 * on disk, which isn't feasible at this time.  Best we can do is always
 * guarantee that the TARGET exists.
 */
/*ARGSUSED*/
static int
ufs_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode *sdvp;		/* old (source) parent vnode */
	char *snm;			/* old (source) entry name */
	struct vnode *tdvp;		/* new (target) parent vnode */
	char *tnm;			/* new (target) entry name */
	struct ucred *cred;
{
	struct inode *sip;		/* source inode */
	register struct inode *sdp;	/* old (source) parent inode */
	register struct inode *tdp;	/* new (target) parent inode */
	register int error;
	struct vnode *realvp;

	VFS_RECORD(sdvp->v_vfsp, VS_RENAME, VS_CALL);

	if (VOP_REALVP(tdvp, &realvp) == 0) {
		tdvp = realvp;
	}

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * Make sure we can delete the source entry.
	 */
	error = iaccess(sdp, IWRITE);
	if (error) {
		return (error);
	}
	/*
	 * Look up inode of file we're supposed to rename.
	 */
	error = dirlook(sdp, snm, &sip);
	if (error) {
		return (error);
	}

	IUNLOCK(sip);			/* unlock inode (it's held) */
	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */
	if ((strcmp(snm, ".") == 0) || (strcmp(snm, "..") == 0) ||
	    (sdp == sip)) {
		error = EINVAL;
		goto out;
	}
	/*
	 * If the source parent directory is "sticky", then the user must
	 * either own the file, or owns the directory, or is the
	 * super-user
	 */
	if ((sdp->i_mode & ISVTX) && cred->cr_uid != 0 &&
	    cred->cr_uid != sdp->i_uid && sip->i_uid != cred->cr_uid) {
			error = EPERM;
			goto out;
	}
	/*
	 * Link source to the target.
	 */
	error = direnter(tdp, tnm, DE_RENAME,
	    sdp, sip, (struct vattr *)0, (struct inode **)0);
	if (error) {
		/*
		 * ESAME isn't really an error; it indicates that the
		 * operation should not be done because the source and target
		 * are the same file, but that no error should be reported.
		 */
		if (error == ESAME)
			error = 0;
		goto out;
	}

	/*
	 * Unlink the source.
	 * Remove the source entry.  Dirremove checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it.
	 * Release the source inode.
	 */
	error = dirremove(sdp, snm, sip, 0);
	if (error == ENOENT) {
		error = 0;
	} else if (error) {
		goto out;
	}

out:
	ITIMES(sdp);
	ITIMES(tdp);
	irele(sip);
	return (error);
}

/*ARGSUSED*/
static int
ufs_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode *dvp;
	char *nm;
	register struct vattr *vap;
	struct vnode **vpp;
	struct ucred *cred;
{
	register struct inode *ip;
	struct inode *xip;
	register int error;

	VFS_RECORD(dvp->v_vfsp, VS_MKDIR, VS_CALL);

	ip = VTOI(dvp);
	/*
	 * New directory inherits the set-gid bit from the parent.
	 */
	vap->va_mode &= ~VSGID;
	if (ip->i_mode & ISGID)
		vap->va_mode |= VSGID;

	error = direnter(ip, nm, DE_CREATE,
	    (struct inode *)0, (struct inode *)0, vap, &xip);
	ITIMES(ip);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		ITIMES(ip);
		IUNLOCK(ip);
	} else if (error == EEXIST) {
		iput(xip);
	}
	return (error);
}

/*ARGSUSED*/
static int
ufs_rmdir(vp, nm, cred)
	struct vnode *vp;
	char *nm;
	struct ucred *cred;
{
	register struct inode *ip;
	register int error;

	VFS_RECORD(vp->v_vfsp, VS_RMDIR, VS_CALL);

	ip = VTOI(vp);
	error = dirremove(ip, nm, (struct inode *)0, 1);
	ITIMES(ip);
	return (error);
}

/*ARGSUSED*/
static int
ufs_readdir(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register struct iovec *iovp;
	register struct inode *ip;
	register struct direct *idp;
	register struct dirent *odp;
	register u_int offset;
	register int incount = 0;
	register int outcount = 0;
	register u_int bytes_wanted, total_bytes_wanted;
	caddr_t outbuf;
	u_int bufsize;
	int error = 0;
	struct fbuf *fbp;
	int fastalloc;
	static caddr_t dirbufp;

	VFS_RECORD(vp->v_vfsp, VS_READDIR, VS_CALL);

	ip = VTOI(vp);
	iovp = uiop->uio_iov;
	total_bytes_wanted = iovp->iov_len;

	/* Force offset to be valid (to guard against bogus lseek() values) */
	offset = uiop->uio_offset & ~(DIRBLKSIZ - 1);

	/* Quit if at end of file */
	if (offset >= ip->i_size)
		return (0);

	/*
	 * Get space to change directory entries into fs independent format.
	 * Do fast alloc for the most commonly used-request size (filesystem
	 * block size).
	 */
	fastalloc = (total_bytes_wanted == MAXBSIZE);
	bufsize = total_bytes_wanted + sizeof (struct dirent);
	if (fastalloc)
		outbuf = new_kmem_fast_alloc(
				&dirbufp, (int)bufsize, 1, KMEM_SLEEP);
	else
		outbuf = new_kmem_alloc(bufsize, KMEM_SLEEP);
	odp = (struct dirent *)outbuf;

	ILOCK(ip);
nextblk:
	bytes_wanted = total_bytes_wanted;

	/* Truncate request to file size */
	if (offset + bytes_wanted > ip->i_size)
		bytes_wanted = ip->i_size - offset;

	/* Comply with MAXBSIZE boundary restrictions of fbread() */
	if ((offset & MAXBOFFSET) + bytes_wanted > MAXBSIZE)
		bytes_wanted = MAXBSIZE - (offset & MAXBOFFSET);

	/* Read in the next chunk */
	if (error = fbread(vp, offset, bytes_wanted, S_OTHER, &fbp))
		goto out;

	incount = 0;
	idp = (struct direct *)fbp->fb_addr;

	/* Transform to file-system independent format */
	while (incount < bytes_wanted) {
		extern char *strcpy();

		/* Skip to requested offset and skip empty entries */
		if (idp->d_ino != 0 && offset >= uiop->uio_offset) {
			odp->d_fileno = idp->d_ino;
			odp->d_namlen = idp->d_namlen;
			(void) strcpy(odp->d_name, idp->d_name);
			odp->d_reclen = DIRSIZ(odp);
			odp->d_off = offset + idp->d_reclen;
			outcount += odp->d_reclen;
			/* Got as many bytes as requested, quit */
			if (outcount > total_bytes_wanted) {
				outcount -= odp->d_reclen;
				break;
			}
			odp = (struct dirent *)((int)odp + odp->d_reclen);
		}
		if (idp->d_reclen) {
			incount += idp->d_reclen;
			offset += idp->d_reclen;
			idp = (struct direct *)((int)idp + idp->d_reclen);
		} else {
			offset = (offset + DIRBLKSIZ) & ~(DIRBLKSIZ-1);
			break;
		}
	}
	/* Release the chunk */
	fbrelse(fbp, S_OTHER);

	/* Read whole block, but got no entries, read another if not eof */
	if (offset < ip->i_size && !outcount)
		goto nextblk;

	/* Copy out the entry data */
	if (error = uiomove(outbuf, outcount, UIO_READ, uiop))
		goto out;

	uiop->uio_offset = offset;
	if (!ULOCKFS_IS_NOIACC(ITOU(ip)))
		ip->i_flag |= IACC;
out:
	ITIMES(ip);
	IUNLOCK(ip);
	if (fastalloc)
		kmem_fast_free(&dirbufp, outbuf);
	else
		kmem_free(outbuf, bufsize);
	return (error);
}

/*
 * Old form of the ufs_readdir op. Returns directory entries directly
 * from the disk in the 4.2 structure instead of the new sys/dirent.h
 * structure. This routine is called directly by the old getdirentries
 * system call when it discovers it is dealing with a ufs filesystem.
 * The reason for this mess is to avoid large performance penalties
 * that occur during conversion from the old format to the new and
 * back again.
 */

/*ARGSUSED*/
int
old_ufs_readdir(vp, uiop, cred)
	struct vnode *vp;
	register struct uio *uiop;
	struct ucred *cred;
{
	register struct iovec *iovp;
	register unsigned count;
	register struct inode *ip;
	int error;
	struct mount *mp;

	if (error = ufs_lockfs_begin(vp, VA_READDIR, &mp))
		return (error);

	ip = VTOI(vp);
	iovp = uiop->uio_iov;
	count = iovp->iov_len;
	if ((uiop->uio_iovcnt != 1) || (count < DIRBLKSIZ) ||
	    (uiop->uio_offset & (DIRBLKSIZ -1))) {
		error = EINVAL;
		goto out;
	}
	count &= ~(DIRBLKSIZ - 1);
	uiop->uio_resid -= iovp->iov_len - count;
	iovp->iov_len = count;
	error = rwip(ip, uiop, UIO_READ, 0);
	ITIMES(ip);
out:
	ufs_lockfs_end(VA_READDIR, mp);
	return (error);
}

/*ARGSUSED*/
static int
ufs_symlink(dvp, lnm, vap, tnm, cred)
	register struct vnode *dvp;
	char *lnm;
	struct vattr *vap;
	char *tnm;
	struct ucred *cred;
{
	struct inode *ip;
	register int error;
	register struct fs *fs;

	VFS_RECORD(dvp->v_vfsp, VS_SYMLINK, VS_MISS);

	/* check for space availability - need at least 1 fragment */
	fs = VTOI(dvp)->i_fs;
	if (cred->cr_uid == 0) {
		if ((fs->fs_cstotal.cs_nbfree == 0) &&
		    (fs->fs_cstotal.cs_nffree == 0))
			return (ENOSPC);
	} else
		if (freespace(fs, fs->fs_minfree) <= 0)
			return (ENOSPC);

	ip = (struct inode *)0;
	vap->va_type = VLNK;
	vap->va_rdev = 0;
	error = direnter(VTOI(dvp), lnm, DE_CREATE,
	    (struct inode *)0, (struct inode *)0, vap, &ip);
	if (error == 0) {
		error = rdwri(UIO_WRITE, ip, tnm, strlen(tnm),
		    (off_t)0, UIO_SYSSPACE, (int *)0);
		if (error) {
			idrop(ip);
			error = dirremove(VTOI(dvp), lnm,
			    (struct inode *) 0, 0);
			goto out;
		}
	}
	if (error == 0) {
		/* create a fast symbolic link */
		if (ip->i_size <= FSL_SIZE) {
			if (kcopy((caddr_t) tnm, (caddr_t) &ip->i_db[1],
				(u_int) ip->i_size) == 0)
				ip->i_flag |= IFASTSYMLNK;
			else {
				int i;
				/* error, clear garbage left behind */
				for (i = 1; i < NDADDR && ip->i_db[i]; i++)
					ip->i_db[i] = 0;
				for (i = 0; i < NIADDR && ip->i_ib[i]; i++)
					ip->i_ib[i] = 0;
			}
		/*
		 * nice to free the page here, but don't bother because
		 * symbolic links are seldom created
		 */
		}
	}

	if (error == 0 || error == EEXIST)
		iput(ip);
out:
	ITIMES(VTOI(dvp));
	return (error);
}

/*
 * Ufs specific routine used to do ufs io.
 */
int
rdwri(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	off_t offset;
	int seg;
	int *aresid;
{
	struct uio auio;
	struct iovec aiov;
	register int error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = seg;
	auio.uio_resid = len;
	error = rwip(ip, &auio, rw, 0);
	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}
	return (error);
}

/*
 * Record-locking requests are passed to the local Lock-Manager daemon.
 */

extern void kill_proc_locks();

/*ARGSUSED*/
static int
ufs_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct eflock *ld;
	int cmd;
	struct ucred *cred;
	int clid;
{
	VFS_RECORD(vp->v_vfsp, VS_LOCKCTL, VS_CALL);

	if (cmd != F_RGETLK && cmd != F_RSETLK && cmd != F_RSETLKW) {
		if (vp->v_type == VBLK || vp->v_type == VCHR ||
			vp->v_type == VFIFO)
			return (EINVAL);
	} else {
		if (vp->v_type == VBLK || vp->v_type == VFIFO)
			return (EINVAL);
	}

	switch (cmd) {
		case F_GETLK :
			test_lock(vp, ld, cmd, clid, FILE_LOCK);
			return (0);
		case F_RGETLK :
			test_lock(vp, ld, cmd, clid, LOCKMGR);
			return (0);
		case F_SETLK :
		case F_SETLKW :
			if (ld->l_type  == F_UNLCK)
				return (unlock(vp, ld, cmd, clid, FILE_LOCK));
			else
				return (lock(vp, ld, cmd, clid, FILE_LOCK));
		case F_RSETLK :
		case F_RSETLKW :
			if (ld->l_type == F_UNLCK)
				return (unlock(vp, ld, cmd, clid, LOCKMGR));
			else if (ld->l_type == F_UNLKSYS) {
				kill_proc_locks(clid, ld->l_rsys);
				return (0);
			} else
				return (lock(vp, ld, cmd, clid, LOCKMGR));
		default:
			return (EINVAL);
	}
}

static int
ufs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct ufid *ufid;

	VFS_RECORD(vp->v_vfsp, VS_FID, VS_CALL);

	ufid = (struct ufid *)new_kmem_zalloc(sizeof (struct ufid), KMEM_SLEEP);
	ufid->ufid_len = sizeof (struct ufid) - sizeof (u_short);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (struct fid *)ufid;
	return (0);
}

/*
 * For read purposes, this has to be bsize * maxcontig.
 * For write purposes, this can be larger.
 */
#define	RD_CLUSTSZ(fs)		(fs->fs_bsize * fs->fs_maxcontig)
#define	WR_CLUSTSZ(fs)		(fs->fs_bsize * fs->fs_maxcontig)

int ufs_nocluster = 0;
int ufs_ra = 1;
int ufs_lostpage;	/* number of times we lost original page */

/*
 * Called from pvn_getpages or ufs_getpage to get a particular page.
 * When we are called the inode is already locked.  If rw == S_WRITE
 * and the block is not currently allocated we need to allocate the
 * needed block(s).
 *
 * Clustering notes: when we detect sequential access, we switch to cluster
 * sized chunks of I/O.  The steady state should be that we do clusters
 * in the readahead case; we'll only do one synchronous read at the beginning.
 * fs_maxcontig controls the cluster size and is bounded by maxphys.
 *
 * We handle bsize >= PAGESIZE here; others go to oldufs_getapage().
 *
 * TODO
 * Think about mmap() writes and lastw/nextr interaction
 */
/*ARGSUSED*/
ufs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
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
	register struct inode *ip;
	register struct fs *fs;
	u_int xlen;
	struct buf *bp, *bp2;
	struct vnode *devvp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn, bn;
	u_int io_off, io_len;
	int len, boff;
	int err, do2ndread;
	dev_t dev;

	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);

	ip = VTOI(vp);
	fs = ip->i_fs;
	if (ufs_nocluster || fs->fs_bsize < PAGESIZE) {
		return (oldufs_getapage(vp, off, protp, pl,
		    plsz, seg, addr, rw, cred));
	}
	devvp = ip->i_devvp;
	dev = devvp->v_rdev;

reread:
	err = 0;
	bp = NULL;
	bp2 = NULL;
	pagefound = NULL;
	do2ndread = ufs_ra && ip->i_nextr == off;
	if (pl != NULL)
		pl[0] = NULL;
	/*
	 * It may seem that only writes need to do the bmap().  Not so -
	 * the protp needs to made readonly if the is backed by a hole.
	 * XXX - it might be possible to fix this.
	 */
	lbn = lblkno(fs, off);
	boff = blkoff(fs, off);
	if (rw == S_WRITE) {
		err = bmap_write(ip, lbn, boff,
		    &bn, &len, (int)blksize(fs, ip, lbn), 0);
	} else {
		err = bmap_read(ip, lbn, boff, &bn, &len);
		if (bn == UFS_HOLE) {
			if (protp != NULL)
				*protp &= ~PROT_WRITE;
			do2ndread = 0;
		}
	}
	if (err)
		goto out;
	if (!do2ndread)
		len = MIN(fs->fs_bsize, len);

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		if (bn == UFS_HOLE) {
			/*
			 * Block for this page is not allocated
			 * and the page was not found.
			 */
			if (pl != NULL) {
				/*
				 * If we need a page, allocate and
				 * return a zero page.  This assumes
				 * that for "async" faults it is not
				 * worth it to create the page now.
				 */
				pp = rm_allocpage(seg, addr, PAGESIZE, 1);
				trace6(TR_SEG_ALLOCPAGE, seg,
				    (u_int)addr & PAGEMASK, TRC_SEG_UNK,
				    vp, off, pp);
				if (page_enter(pp, vp, off))
					panic("ufs_getapage page_enter");
				pagezero(pp, 0, PAGESIZE);
				page_unlock(pp);
				pl[0] = pp;
				pl[1] = NULL;
				u.u_ru.ru_minflt++;
			}
		} else {
			/*
			 * Need to really do disk IO to get the page(s).
			 */
			VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_MISS);

			pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
			    off, (u_int) len, 0);

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

			bp = pageio_setup(pp, io_len, devvp, pl == NULL ?
			    (B_ASYNC | B_READ) : B_READ);

			bp->b_dev = dev;
			bp->b_blkno = fsbtodb(fs, bn) + btodb(boff);
			bp->b_un.b_addr = 0;

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 * pp->p_prev is usually the same page unless
			 * a list of pages, as with exec.
			 *
			 * The only way this can happen, I think, is
			 * at the end of file, so I turn off readahead.
			 */
			xlen = io_len & PAGEOFFSET;
			if (xlen != 0) {
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
				do2ndread = 0;
			}

			(*bdevsw[major(dev)].d_strategy)(bp);

			/*
			 * Set up where to do the next readahead.
			 */
			ip->i_nextrio = off + (io_len & PAGEMASK);
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
	}

 	ip->i_nextr = (off + fs->fs_bsize) & ~(fs->fs_bsize - 1);

	/*
	 * XXX - This can get out of sync if a page has been stolen away in
	 * the previous cluster.  Because we don't resync, this can result in
	 * two sync reads above; one for the stolen page and another on the
	 * following cluster.
	 */
	if (do2ndread &&
	    ip->i_nextrio - off <= RD_CLUSTSZ(fs) &&
	    ip->i_nextrio < ip->i_size) {
		addr_t addr2;

		io_off = ip->i_nextrio;
		addr2 = addr + (io_off - off);
		/*
		 * Read-ahead case (bsize >= PAGESIZE)
		 * If addr is now in a different seg,
		 * don't bother trying with read-ahead.
		 */
		if (addr2 >= seg->s_base + seg->s_size) {
			pp2 = NULL;
			goto out;
		}
		lbn = lblkno(fs, io_off);
		boff = blkoff(fs, io_off);
		err = bmap_read(ip, lbn, boff, &bn, &len);
		if (err || bn == UFS_HOLE)
			goto out;

		pp2 = pvn_kluster(vp, io_off, seg, addr2,
		    &io_off, &io_len, io_off, (u_int) len, 1);
		if (pp2 == NULL)
			goto out;
		bp2 = pageio_setup(pp2, io_len, devvp,
			(B_ASYNC | B_READ));
		bp2->b_dev = dev;
		ASSERT(ip->i_nextrio == pp2->p_offset);
		bp2->b_blkno = fsbtodb(fs, bn) + btodb(boff);
		bp2->b_un.b_addr = 0;

		/*
		 * Zero part of page which we are not
		 * going to be reading from disk now
		 * if it hasn't already been done.
		 */
		if (xlen = (io_len & PAGEOFFSET))
			pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);
		/*
		 * Two cases where io_len < blksz.
		 * (1) We ran out of memory.
		 * (2) The page is already in memory.
		 */
		ip->i_nextrio = (io_off + io_len) & PAGEMASK;

		(*bdevsw[major(dev)].d_strategy)(bp2);

		/*
		 * Should we bill read ahead to extra faults?
		 */
		u.u_ru.ru_majflt++;
		if (seg == segkmap)
			u.u_ru.ru_inblock++;	/* count as `read' */
		cnt.v_pgin++;
		cnt.v_pgpgin += btopr(io_len);
	}

out:
	if (pl == NULL) {
		return (err);
	}

	if (bp != NULL) {
		if (err == 0)
			err = biowait(bp);
		else
			(void) biowait(bp);
		pageio_done(bp);
	}

	if (pagefound != NULL) {
		register int s;

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
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			(void) splx(s);
			ufs_lostpage++;
			goto reread;
		}
		PAGE_HOLD(pagefound);
		(void) splx(s);
		pl[0] = pagefound;
		pl[1] = NULL;
		u.u_ru.ru_minflt++;
	}

	if (err) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return (err);
}

/*
 * Return all the pages from [off..off+len) in given file
 */
static int
ufs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
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
	struct inode *ip = VTOI(vp);
	int err;
	extern freemem, lotsfree;

	/*
	 * Normally fail if faulting beyond EOF, *except* if this
	 * is an internal access of ufs data.  This condition is
	 * detected by testing the faulting segment against segkmap.
	 * Since accessing the file through segkmap is only done
	 * in places in the kernel which have knowledge of the
	 * current file length, these places deal with EOF themselves.
	 * For example, bmap may be faulting in pages beyond the
	 * current EOF when it is creating pages needed for extending
	 * the length of the file.
	 */
	if (off + len > ip->i_size + PAGEOFFSET && seg != segkmap)
		return (EFAULT);	/* beyond EOF */

	if (protp != NULL)
		*protp = PROT_ALL;

	ILOCK(ip);
	if (len <= PAGESIZE) {
		err = ufs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	} else {
		err = pvn_getpages(ufs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cred);
	}
	/*
	 * If the inode is not already marked for IACC (in rwip() for read)
	 * and the inode is not marked for no access time update (in rwip()
	 * for write) then update the inode access time and mod time now.
	 */
	if ((ip->i_flag & (IACC | INOACC)) == 0) {
		if (rw != S_OTHER) {
			if (!ULOCKFS_IS_NOIACC(ITOU(ip)))
				ip->i_flag |= IACC;
		}
		if (rw == S_WRITE) {
			ip->i_flag |= IUPD;
		}
		ITIMES(ip);
	}
	IUNLOCK(ip);

	return (err);
}

/*
 * Called at interrupt level.
 */
static int
ufs_writedone(bp)
	register struct buf *bp;
{
	register struct inode *ip;

	ASSERT(bp->b_pages);
	ip = VTOI(bp->b_pages->p_vnode);	/* gag me */
	bp->b_flags &= ~B_CALL;
	bp->b_iodone = NULL;
	bp->b_flags |= B_DONE;
	if (ip->i_writes > 0) {
		ip->i_writes -= bp->b_bcount + bp->b_resid;
		if (ip->i_writes <= ufs_WRITES)
			wakeup((caddr_t)&ip->i_writes);
	}
	/*
	 * Stolen from biodone()
	 */
	if (bp->b_flags & B_ASYNC) {
		if (bp->b_flags & (B_PAGEIO|B_REMAPPED))
			swdone(bp);
		else
			brelse(bp);
	} else if (bp->b_flags & B_WANTED) {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
}

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED}
 * XXX - Has to be exported for 4K FS support.
 */
/* static */
int
ufs_writelbn(ip, bn, pp, len, pgoff, flags)
	register struct inode *ip;
	daddr_t bn;
	struct page *pp;
	u_int len;
	u_int pgoff;
	int flags;
{
	struct buf *bp;
	int err;

	bp = pageio_setup(pp, len, ip->i_devvp, B_WRITE | flags);
	if (bp == NULL) {
		pvn_fail(pp, B_WRITE | flags);
		return (ENOMEM);
	}
	if (ufs_WRITES) {
		int s;

		/*
		 * protected because the completion interrupt changes this.
		 */
		s = splbio();
		ip->i_writes += len;
		(void) splx(s);
		bp->b_flags |= B_CALL;
		bp->b_iodone = ufs_writedone;
	}

	bp->b_dev = ip->i_dev;
	bp->b_blkno = bn;
	bp->b_un.b_addr = (addr_t)pgoff;

	(*bdevsw[major(ip->i_dev)].d_strategy)(bp);
	u.u_ru.ru_oublock++;

	/*
	 * If async, assume that pvn_done will
	 * handle the pages when IO is done
	 */
	if (flags & B_ASYNC) {
		return (0);
	}

	err = biowait(bp);
	pageio_done(bp);

	return (err);
}

/*
 * Macro to be used to see if it is safe to ILOCK the inode.
 * This is needed because the pageout daemon cannot afford to
 * wait for an inode lock since the process that has the inode
 * lock may need more memory from the pageout daemon to complete
 * its work.  This is used to prevent deadlocking situations.
 */
#define	ICHECK(ip)	((NOMEMWAIT()) && ((ip)->i_flag & ILOCKED) && \
			((ip)->i_owner != uniqpid()))

int	ufs_delay = 1;		/* patchable while running */

/*
 * Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}
 * If len == 0, do from off to EOF.
 *
 * The normal cases should be len == 0 & off == 0 (entire vp list),
 * len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 * (from pageout).
 *
 * Note that for ufs it is possible to have dirty pages beyond
 * roundup(ip->i_size, PAGESIZE).  This can happen if the file
 * length is long enough to involve indirect blocks (which are
 * always fs->fs_bsize'd) and PAGESIZE < bsize while the length
 * is such that roundup(blkoff(fs, ip->i_size), PAGESIZE) < bsize.
 */

/*ARGSUSED*/
static int
ufs_putpage(vp, off, len, flags, cred)
	register struct vnode *vp;
	u_int off, len;
	int flags;
	struct ucred *cred;
{
	register struct inode *ip;
	register struct page *pp;
	register struct fs *fs;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	daddr_t lbn, dbn;
	daddr_t bn;
	int bmaplen, boff;
	int vpcount, err;

#ifdef	VFSSTATS
	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_CALL);
#endif

	ip = VTOI(vp);
	fs = ip->i_fs;
	if (ufs_nocluster || fs->fs_bsize < PAGESIZE) {
		return (oldufs_putpage(vp, off, len, flags, cred));
	}
	if (vp->v_pages == NULL) {
		return (0);
	}

	/*
	 * If (clustering) AND
	 * (it's a normal write, i.e., normal flags) AND
	 * (we're doing a portion of the file) AND
	 * (we've delayed less than a clusters worth) AND
	 * (this is the 1st chunk OR this chunk is contig w/the last chunk) THEN
	 * delay this chunk; we'll push it later.
	 */
	if (ufs_delay && (flags & B_ASYNC) &&
	    (flags & ~(B_ASYNC|B_DONTNEED|B_FREE)) == 0 && len &&
	    (ip->i_delaylen + len) < WR_CLUSTSZ(fs) &&
	    (ip->i_delaylen == 0 || ip->i_delayoff + ip->i_delaylen == off)) {
		if (ip->i_delaylen == 0)
			ip->i_delayoff = off;
		ip->i_delaylen += len;
		return (0);
	}
	vpcount = vp->v_count;
	VN_HOLD(vp);

again:

	/*
	 * Cannot afford to sleep on inode now, give up
	 */
	if (ICHECK(ip)) {
		err = ENOMEM;
		goto errout;
	}

	/*
	 * Hold inode lock for duration of push
	 */
	ILOCK(ip);
	if (len == 0) {
		/*
		 * Search the entire vp list for pages >= off
		 */
		dirty = pvn_vplist_dirty(vp, off, flags);
		ip->i_delaylen = ip->i_delayoff = 0;
	} else {
		u_int offlo, offhi, offclust;
		u_int d_len, d_off;

		/*
		 * if (delayed pages)
		 *	if (current request not in/adjacent to delayed pages)
		 *		push old pages
		 *	else
		 *		start at beginning of delayed pages
		 * do [offlo..off+len) clustered up to off + WR_CLUSTSZ
		 *
		 * We play fast and loose with EOF here; counting on the
		 * fact that range_dirty will just not find the pages.
		 */
			offlo = off;
		offhi = off + len;
		offclust = MAX(offhi, off + WR_CLUSTSZ(fs));
			if (ip->i_delaylen) {
				d_off = ip->i_delayoff;
				d_len = ip->i_delaylen;
				ip->i_delayoff = ip->i_delaylen = 0;
				if (off < d_off || off > d_off + d_len) {
				int	e;

				if (e = ufs_putpage(vp, d_off,
					    d_len, B_NODELAY|B_ASYNC, cred)) {
					printf("PP: vp=%x off=%d len=%d e=%d\n",
					    vp, d_off, d_len, e);
					}
				} else {
					offlo = d_off;
				}
			}
			dirty = pvn_range_dirty(vp, offlo, offhi,
			    offlo, offclust, flags);
		}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  All the pages on the pp list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	err = fs->fs_ronly && dirty != NULL ? EROFS : 0;

	if (dirty != NULL) {
		/*
		 * If the modified time on the inode has not already been
		 * set elsewhere (i.e. for write/setattr) or this is
		 * a call from msync (B_FORCE) we set the time now.
		 * This gives us approximate modified times for mmap'ed files
		 * which are modified via stores in the user address space.
		 */
		if ((ip->i_flag & IMODTIME) == 0 || (flags & B_FORCE) != 0) {
			ip->i_flag |= IUPD;
			ITIMES(ip);
		}
		/*
		 * file system was modified
		 */
		LOCKFS_SET_MOD(UTOL(ITOU(ip)));
	}

	/*
	 * Handle all the dirty pages.
	 *
	 * Clustering changes: instead of grabbing a blocks worth,
	 * take whatever the extent tells us to.
	 *
	 * This code *assumes* that the list is in increasing order.
	 * There's a performance hit if it's not.
	 */
	pp = NULL;
	while (err == 0 && dirty != NULL) {
		io_off = dirty->p_offset;
		lbn = lblkno(fs, io_off);
		boff = blkoff(fs, io_off);
		/*
		 * Normally the blocks should already be allocated for
		 * any dirty pages, we only need to use bmap_rd (S_OTHER)
		 * here and we should not get back a bn == UFS_HOLE.
		 */
		if (err = bmap_read(ip, lbn, boff, &bn, &bmaplen)) {
			break;
		}
		if (bn == UFS_HOLE) {
			if (!IS_SWAPVP(vp)) {
				printf("ip=%x lbn=%d boff=%d off=%d poff=%d\n",
				    ip, lbn, boff, off, io_off);
				panic("ufs_putpage hole");
			}
			/*
			 * Allocate for "holey" ufs file now.
			 * XXX - should redo the anon code to
			 * synchronously insure that all the
			 * needed backing store is allocated.
			 */
			err = bmap_write(ip, lbn, boff,
			    &bn, &bmaplen, (int)blksize(fs, ip, lbn), 1);
			if (err) {
				break;
			}
			ASSERT(bn != UFS_HOLE);
		}

		VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_MISS);
		/*
		 * Pull off up to clustsize as long as it's contig.
		 * bmaplen tells everything we need to know.
		 * The list from pvn_xxx is sorted, all we have to check
		 * for are gaps.
		 */
		ASSERT(bmaplen > 0);	/* leave this in */
		pp = io_list = dirty;
		io_len = 0;
		do {
			io_len += PAGESIZE;
			bmaplen -= PAGESIZE;
			pp = pp->p_next;
		} while (bmaplen > 0 &&
		    pp != dirty && pp->p_offset == io_off + io_len);

		/*
		 * Might have hit a gap or run out of extent.
		 * Have to break the list right before pp.
		 * No spls because the pages are held.
		 */
		if (pp != dirty &&
		    (pp->p_offset != io_off + io_len || bmaplen <= 0)) {
			struct page *tail;

			dirty = pp;
			tail = io_list->p_prev;
			pp = pp->p_prev;
			tail->p_next = dirty;
			dirty->p_prev = tail;
			io_list->p_prev = pp;
			pp->p_next = io_list;
		} else {
			dirty = NULL;
		}

		/*
		 * Might have gone to far (bmaplen is negative).
		 * We could have several full blocks and then a frag.
		 */
		if (bmaplen < 0)
			io_len += bmaplen;

		dbn = fsbtodb(fs, bn) + btodb(boff);
		err = ufs_writelbn(ip, dbn, io_list, io_len, 0, flags);
		pp = NULL;
	}
	IUNLOCK(ip);

	if (err != 0) {
		if (pp != NULL)
			pvn_fail(pp, B_WRITE | flags);
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && (len == 0 || len >= ip->i_size)) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 *
		 * We change len (possibly) from i_size to 0.  This will
		 * make sure we get *all* the pages, including pages that
		 * may be past EOF.  The other path may miss them.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
		    ((vp->v_pages != NULL) && (vp->v_pages->p_lckcnt == 0))) {
			len = 0;
			goto again;
		}
		/*
		 * We have just sync'ed back all the pages
		 * on the inode, turn off the IMODTIME flag.
		 */
		ip->i_flag &= ~IMODTIME;
	}

	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */
errout:
	if (--vp->v_count == 0 && vpcount > 0)
		iinactive(ip);
	return (err);
}

/*ARGSUSED*/
static int
ufs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
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
ufs_cmp(vp1, vp2)
	struct vnode *vp1, *vp2;
{

	VFS_RECORD(vp1->v_vfsp, VS_CMP, VS_CALL);
	return (vp1 == vp2);
}

/*ARGSUSED*/
static int
ufs_realvp(vp, vpp)
	struct vnode *vp;
	struct vnode **vpp;
{

	VFS_RECORD(vp->v_vfsp, VS_REALVP, VS_CALL);
	return (EINVAL);
}

static int
ufs_badop()
{

	panic("ufs_badop");
}

/*ARGSUSED*/
static int
ufs_cntl(vp, cmd, idata, odata, iflag, oflag)
	struct vnode *vp;
	int cmd, iflag, oflag;
	caddr_t idata, odata;

{
	/*
	 * Currently we only allow a cmd passed in and an int passed out
	 */
	ASSERT(odata && oflag == CNTL_INT32);
	switch (cmd) {
	default:
		return (EINVAL);
	case _PC_LINK_MAX:
		*(int *)odata = MAXLINK;
		break;
	case _PC_MAX_CANON:
		*(int *)odata = CANBSIZ;
		break;
	case _PC_NAME_MAX:
		*(int *)odata = MAXNAMLEN;
		break;
	case _PC_PATH_MAX:
		*(int *)odata = MAXPATHLEN;
		break;
	case _PC_PIPE_BUF:
		*(int *)odata = fifoinfo.fifobuf;
		break;
	case _PC_VDISABLE:
		*(int *)odata = VDISABLE;
		break;
	case _PC_CHOWN_RESTRICTED:
		*(int *)odata = 1;
		break;
	case _PC_NO_TRUNC:
		*(int *)odata = 1;
		break;
	}
	return (0);
}

#ifndef	REMOVE_OLD_UFS
/*
 * This stuff is obsolete.  Here for compat but we're phasing out
 * 4K file systems.
 */

/*
 * Called from pvn_getpages or ufs_getpage to get a particular page.
 * When we are called the inode is already locked.  If rw == S_WRITE
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
oldufs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
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
	register struct inode *ip;
	register struct fs *fs;
	register int bsize;
	u_int xlen;
	struct buf *bp, *bp2;
	struct vnode *devvp;
	struct page *pp, *pp2, **ppp, *pagefound;
	daddr_t lbn, bn, bn2;
	u_int io_off, io_len;
	u_int lbnoff, blksz;
	int err, nio, do2ndread, pgoff;
	int multi_io;
	dev_t dev;

	VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_CALL);

	ip = VTOI(vp);
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	devvp = ip->i_devvp;
	dev = devvp->v_rdev;
	multi_io = (PAGESIZE > bsize);

reread:
	bp = NULL;
	bp2 = NULL;
	pagefound = NULL;
	pgoff = 0;
	lbn = lblkno(fs, off);
	lbnoff = off & fs->fs_bmask;
	if (pl != NULL)
		pl[0] = NULL;

	err = bmap(ip, lbn, &bn, &bn2, (int)blksize(fs, ip, lbn), rw, 0);
	if (err)
		goto out;

	if (bn == UFS_HOLE && protp != NULL)
		*protp &= ~PROT_WRITE;

	if (multi_io) {
		if (bsize != PAGESIZE / 2) {
			/*
			 * This should have been prevented at mount time
			 * XXX - need to rewrite to avoid this restriction.
			 */
			panic("ufs_getapage bad bsize");
			/* NOTREACHED */
		}

		if (bn2 == UFS_HOLE && ip->i_size > lbnoff + bsize) {
			/*
			 * Try bmap with bn2 as the primary block now.
			 */
			err = bmap(ip, lbn + 1, &bn2, (daddr_t *)0,
			    (int)blksize(fs, ip, lbn + 1), rw, 0);
			if (err)
				goto out;
		}

		/*
		 * See if we are going to need to do a 2nd read
		 * to handle the bsize == PAGESIZE / 2 case.
		 */
		if (bn != UFS_HOLE && bn2 != UFS_HOLE &&
		    lbnoff + bsize < ip->i_size) {
			nio = 2;
			do2ndread = 1;
		} else {
			nio = 1;
			do2ndread = 0;
			if (bn2 == UFS_HOLE && lbnoff + bsize < ip->i_size)
				*protp &= ~PROT_WRITE;
		}
	} else {
		nio = 1;
		if (ufs_ra && ip->i_nextr == off && bn2 != UFS_HOLE &&
		    lbnoff + bsize < ip->i_size) {
			do2ndread = 1;
		} else {
			do2ndread = 0;
		}
	}

again:
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Compute the size of the block we actually want
		 * to read to be the smaller of a page boundary
		 * or the ufs acquired block size (i.e., we don't
		 * want to try and read the next page beyond EOF).
		 */
		blksz = MIN(roundup(ip->i_size, PAGESIZE) - lbnoff,
		    blksize(fs, ip, lbn));

		if (bn == UFS_HOLE || off >= lbnoff + blksz) {
			/*
			 * Block for this page is not allocated
			 * and the page was not found.
			 */
			if (pl != NULL) {
				/*
				 * If we need a page, allocate and
				 * return a zero page.  This assumes
				 * that for "async" faults it is not
				 * worth it to create the page now.
				 */
				pp = rm_allocpage(seg, addr, PAGESIZE, 1);
				trace6(TR_SEG_ALLOCPAGE, seg,
				    (u_int)addr & PAGEMASK, TRC_SEG_UNK,
				    vp, off, pp);
				if (page_enter(pp, vp, off))
					panic("ufs_getapage page_enter");
				pagezero(pp, 0, PAGESIZE);
				page_unlock(pp);
				pl[0] = pp;
				pl[1] = NULL;
				u.u_ru.ru_minflt++;
			}
		} else {
			/*
			 * Need to really do disk IO to get the page(s).
			 */
			VFS_RECORD(vp->v_vfsp, VS_GETPAGE, VS_MISS);

			pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
			    lbnoff, blksz, 0);
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

			if (nio > 1)
				pp->p_nio = nio;
			bp = pageio_setup(pp, io_len, devvp, pl == NULL ?
			    (B_ASYNC | B_READ) : B_READ);

			bp->b_dev = dev;
			bp->b_blkno = fsbtodb(fs, bn) +
			    btodb(blkoff(fs, io_off));
			bp->b_un.b_addr = 0;

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now.
			 * pp->p_prev is usually the same page unless
			 * a list of pages, as with exec.
			 */
			xlen = io_len & PAGEOFFSET;
			if (xlen != 0)
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[major(dev)].d_strategy)(bp);

			ip->i_nextr = io_off + io_len;
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
	}

	lbn++;
	lbnoff += fs->fs_bsize;

	if (do2ndread && !(multi_io && pagefound != NULL)) {
		addr_t addr2;

		addr2 = addr + (lbnoff - off);

		/*
		 * Compute the size of the block we actually want
		 * to read to be the smaller of a page boundary
		 * or the ufs acquired block size (i.e., we don't
		 * want to try and read the next page beyond EOF).
		 */
		blksz = MIN(roundup(ip->i_size, PAGESIZE) - lbnoff,
		    blksize(fs, ip, lbn));

		if (multi_io) {
			/*
			 * Second block for same page (bsize < PAGESIZE)
			 */
			pp2 = pp;
			if (nio < 2) {
				/*
				 * The first block was a hole, set up
				 * the page properly for io now.  Otherwise,
				 * the page should already be marked as
				 * being paged in with an nio value of 2.
				 */
				page_lock(pp2);
				PAGE_HOLD(pp2);
				pp2->p_intrans = 1;
				pp2->p_pagein = 1;
			}
			io_len = blksz;
			pgoff = bsize;
		} else {
			/*
			 * Read-ahead case (bsize >= PAGESIZE)
			 * If addr is now in a different seg,
			 * don't bother trying with read-ahead.
			 */
			if (addr2 >= seg->s_base + seg->s_size) {
				pp2 = NULL;
			} else {
				pp2 = pvn_kluster(vp, lbnoff, seg, addr2,
				    &io_off, &io_len, lbnoff, blksz, 1);
			}
			pgoff = 0;
		}

		if (pp2 != NULL) {
			/*
			 * Do a synchronous read here only if a page
			 * list was given to this routine and the
			 * block size is smaller than the page size.
			 */
			bp2 = pageio_setup(pp2, io_len, devvp,
			    (pl != NULL && multi_io) ?
				B_READ : (B_ASYNC | B_READ));

			bp2->b_dev = dev;
			bp2->b_blkno = fsbtodb(fs, bn2);
			bp2->b_un.b_addr = (caddr_t)pgoff;

			/*
			 * Zero part of page which we are not
			 * going to be reading from disk now
			 * if it hasn't already been done.
			 */
			xlen = (io_len + pgoff) & PAGEOFFSET;
			if ((xlen != 0) && !multi_io)
				pagezero(pp2->p_prev, xlen, PAGESIZE - xlen);

			(*bdevsw[major(dev)].d_strategy)(bp2);

			/*
			 * Should we bill read ahead to extra faults?
			 */
			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;	/* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
	}

out:
	if (pl == NULL)
		return (err);

	if (bp != NULL) {
		if (err == 0)
			err = biowait(bp);
		else
			(void) biowait(bp);
		pageio_done(bp);
	}

	/*
	 * Only wait for the second read operation
	 * when it is required for getting a page.
	 */
	if (multi_io && bp2 != NULL) {
		if (err == 0)
			err = biowait(bp2);
		else
			(void) biowait(bp2);
		pageio_done(bp2);
	}

	if (pagefound != NULL) {
		register int s;

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
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			(void) splx(s);
			ufs_lostpage++;
			goto reread;
		}
		PAGE_HOLD(pagefound);
		(void) splx(s);
		pl[0] = pagefound;
		pl[1] = NULL;
		u.u_ru.ru_minflt++;
		ip->i_nextr = off + PAGESIZE;
	}

	if (err) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

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
 * Note that for ufs it is possible to have dirty pages beyond
 * roundup(ip->i_size, PAGESIZE).  This can happen if the file
 * length is long enough to involve indirect blocks (which are
 * always fs->fs_bsize'd) and PAGESIZE < bsize while the length
 * is such that roundup(blkoff(fs, ip->i_size), PAGESIZE) < bsize.
 */
/*ARGSUSED*/
int
oldufs_putpage(vp, off, len, flags, cred)
	register struct vnode *vp;
	u_int off, len;
	int flags;
	struct ucred *cred;
{
	register struct inode *ip;
	register struct page *pp;
	register struct fs *fs;
	struct page *dirty, *io_list;
	register u_int io_off, io_len;
	daddr_t lbn, bn, bn2;
	u_int lbn_off;
	int bsize, bsize2;
	int vpcount;
	int err;

#ifdef VFSSTATS
	VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_CALL);
#endif

	ip = VTOI(vp);

	if (vp->v_pages == NULL || off >= ip->i_size)
		return (0);

	vpcount = vp->v_count;
	VN_HOLD(vp);
	fs = ip->i_fs;

again:

	/*
	 * Cannot afford to sleep on inode now, give up
	 */
	if (ICHECK(ip)) {
		err = ENOMEM;
		goto errout;
	}

	/*
	 * Hold inode lock for duration of push
	 */
	ilock(ip);
	if (len == 0) {
		/*
		 * Search the entire vp list for pages >= off
		 */
		dirty = pvn_vplist_dirty(vp, off, flags);
	} else {
		/*
		 * Do a range from [off...off + len) via page_find.
		 * We set limits so that we kluster to bsize boundaries.
		 */
		if (off >= ip->i_size) {
			dirty = NULL;
		} else {
			u_int fsize, eoff;

			/*
			 * Use MAXBSIZE rounding to get indirect block pages
			 * which might beyond roundup(ip->i_size, PAGESIZE);
			 */
			fsize = (ip->i_size + MAXBOFFSET) & MAXBMASK;
			eoff = MIN(off + len, fsize);
			dirty = pvn_range_dirty(vp, off, eoff,
			    (u_int)(off & fs->fs_bmask),
			    (u_int)((eoff + fs->fs_bsize - 1) & fs->fs_bmask),
			    flags);
		}
	}

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  All the pages on the pp list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	if (fs->fs_ronly && dirty != NULL)
		err = EROFS;
	else
		err = 0;

	if (dirty != NULL) {
		/*
		 * Destroy the read ahead value now
		 * since we are really going to write
		 */
		ip->i_nextr = 0;

		/*
		 * If the modified time on the inode has not already been
		 * set elsewhere (i.e. for write/setattr) or this is
		 * a call from msync (B_FORCE) we set the time now.
		 * This gives us approximate modified times for mmap'ed files
		 * which are modified via stores in the user address space.
		 */
		if ((ip->i_flag & IMODTIME) == 0 || (flags & B_FORCE) != 0) {
			ip->i_flag |= IUPD;
			ITIMES(ip);
		}
		/*
		 * file system was modified
		 */
		LOCKFS_SET_MOD(UTOL(ITOU(ip)));
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
		lbn = lblkno(fs, io_off);
		bsize = blksize(fs, ip, lbn);
		/*
		 * Normally the blocks should already be allocated for
		 * any dirty pages, we only need to use S_OTHER
		 * here and we should not get back a bn == UFS_HOLE.
		 */
		err = bmap(ip, lbn, &bn, &bn2, bsize, S_OTHER, 1);
		if (err) {
			break;
		}
		if (bn == UFS_HOLE) {
			if (!IS_SWAPVP(vp) && fs->fs_bsize >= PAGESIZE)
				panic("ufs_putpage hole");
			/*
			 * Allocate for "holey" ufs file now.
			 * XXX - should redo the anon code to
			 * synchronously insure that all the
			 * needed backing store is allocated.
			 */
			err = bmap(ip, lbn, &bn, &bn2, bsize, S_WRITE, 1);
			if (err) {
				break;
			}
			ASSERT(bn != UFS_HOLE);
		}

		VFS_RECORD(vp->v_vfsp, VS_PUTPAGE, VS_MISS);

		pp = io_list = dirty;
		io_len = PAGESIZE;
		lbn_off = lbn << fs->fs_bshift;
		page_sub(&dirty, pp);

		while (dirty != NULL && dirty->p_offset < lbn_off + bsize &&
		    dirty->p_offset == io_off + io_len) {
			pp = dirty;
			page_sub(&dirty, pp);
			/*
			 * Add the page to the end of the list.  page_sortadd
			 * can do this without walking the list.
			 */
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/* IO may be asynch, so need to set nio first */
		if (fs->fs_bsize < PAGESIZE && ip->i_size > lbn_off + bsize) {
			pp->p_nio = lblkno(fs, PAGESIZE);
		} else {
			pp->p_nio = 0;
			/*
			 * Check for page length rounding problems
			 */
			if (io_off + io_len > lbn_off + bsize) {
				ASSERT((io_off + io_len) - (lbn_off + bsize) <
				    PAGESIZE);
				io_len = lbn_off + bsize - io_off;
			}
		}

		/*
		 * Should zero any bytes beyond EOF,
		 * but it's not worth the work now.
		 */

		/*
		 * See if we need to do a 2nd bmap operation.
		 * This is needed if nio is non-zero and we
		 * didn't get a bn back from the 1st bmap().
		 */
		if (pp->p_nio) {
			ASSERT(pp->p_nio == 2);		/* XXX */
			++lbn;
			bsize2 = blksize(fs, ip, lbn);
			if (bn2 == UFS_HOLE) {
				/*
				 * Allocate backing store only if this is
				 * a swap vnode in case someone is using
				 * a "holey" ufs swap file with bsize <
				 * PAGESIZE (e.g., a 4k fs w/ 8k pages).
				 * XXX - should redo the anon code to
				 * synchronously insure that all the
				 * needed backing store is allocated.
				 */
				err = bmap(ip, lbn, &bn2, (daddr_t *)NULL,
				    bsize2, IS_SWAPVP(vp)? S_WRITE:S_OTHER, 1);

				if (err) {
					pvn_fail(pp, B_WRITE | flags);
					break;
				}
			}
			if (bn2 == UFS_HOLE)
				pp->p_nio = 1;

			/*
			 * Ok, now do it.
			 */
			err = ufs_writelbn(ip, fsbtodb(fs, bn), pp,
			    (u_int)bsize, 0, flags);
			if (err == 0 && bn2 != UFS_HOLE) {
				err = ufs_writelbn(ip, fsbtodb(fs, bn2), pp,
				    (u_int)bsize2, (u_int)fs->fs_bsize, flags);
				pp = NULL;
			}
		} else {
			bn = fsbtodb(fs, bn) + btodb(io_off - lbn_off);
			err = ufs_writelbn(ip, bn, io_list, io_len, 0, flags);
			pp = NULL;
		}
	}
	iunlock(ip);

	if (err != 0) {
		if (pp != NULL)
			pvn_fail(pp, B_WRITE | flags);
		if (dirty != NULL)
			pvn_fail(dirty, B_WRITE | flags);
	} else if (off == 0 && (len == 0 || len >= ip->i_size)) {
		/*
		 * If doing "synchronous invalidation", make
		 * sure that all the pages are actually gone.
		 */
		if ((flags & (B_INVAL | B_ASYNC)) == B_INVAL &&
		    ((vp->v_pages != NULL) && (vp->v_pages->p_lckcnt == 0)))
			goto again;
		/*
		 * We have just sync'ed back all the pages
		 * on the inode, turn off the IMODTIME flag.
		 */
		ip->i_flag &= ~IMODTIME;
	}

	/*
	 * Instead of using VN_RELE here we are careful to only call
	 * the inactive routine if the vnode reference count is now zero,
	 * but it wasn't zero coming into putpage.  This is to prevent
	 * recursively calling the inactive routine on a vnode that
	 * is already considered in the `inactive' state.
	 * XXX - inactive is a relative term here (sigh).
	 */
errout:
	if (--vp->v_count == 0 && vpcount > 0)
		iinactive(ip);
	return (err);
}
#else
oldufs_putpage(vp, off, len, flags, cred)
	register struct vnode *vp;
	u_int off, len;
	int flags;
	struct ucred *cred;
{
	return (ENOSYS);
}

oldufs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
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
	return (ENOSYS);
}
#endif	/* REMOVE_OLD_UFS */
/*
 * ULOCKFS Intercept Routines
 *	VOP calls are intercepted and wrapped with lockfs code.
 */
static int
ufs_l_open(vpp, flag, cred)
	struct vnode	**vpp;
	int		flag;
	struct ucred	*cred;
{
	ULOCKFS(*vpp, VA_OPEN,
		ufs_open(vpp, flag, cred));
}
static int
ufs_l_close(vp, flag, count, cred)
	struct vnode	*vp;
	int		flag;
	int		count;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_CLOSE,
		ufs_close(vp, flag, count, cred));
}
static int
ufs_l_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode	*vp;
	struct uio	*uiop;
	enum uio_rw	rw;
	int		ioflag;
	struct ucred	*cred;
{
	if (rw == UIO_READ) {
		ULOCKFS(vp, VA_READ,
			ufs_rdwr(vp, uiop, rw, ioflag, cred));
	} else {
		ULOCKFS(vp, VA_WRITE,
			ufs_rdwr(vp, uiop, rw, ioflag, cred));
	}
}
static int
ufs_l_select(vp, which, cred)
	struct vnode	*vp;
	int		which;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_SELECT,
		ufs_select(vp, which, cred));
}
static int
ufs_l_getattr(vp, vap, cred)
	struct vnode		*vp;
	register struct vattr	*vap;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_GETATTR,
		ufs_getattr(vp, vap, cred));
}
static int
ufs_l_setattr(vp, vap, cred)
	register struct vnode	*vp;
	register struct vattr	*vap;
	struct ucred		*cred;
{
	if (vap->va_size != (u_long)-1) {
		ULOCKFS(vp, VA_TRUNC,
			ufs_setattr(vp, vap, cred));
	} else {
		ULOCKFS(vp, VA_CHANGE,
			ufs_setattr(vp, vap, cred));
	}
}
static int
ufs_l_access(vp, mode, cred)
	struct vnode		*vp;
	int			mode;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_ACCESS,
		ufs_access(vp, mode, cred));
}
static int
ufs_l_readlink(vp, uiop, cred)
	struct vnode		*vp;
	struct uio		*uiop;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_READLINK,
		ufs_readlink(vp, uiop, cred));
}
static int
ufs_l_fsync(vp, cred)
	struct vnode		*vp;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_FSYNC,
		ufs_fsync(vp, cred));
}
static int
ufs_l_inactive(vp, cred)
	struct vnode		*vp;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_INACTIVE,
		ufs_inactive(vp, cred));
}
static int
ufs_l_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode		*dvp;
	char			*nm;
	struct vnode		**vpp;
	struct ucred		*cred;
	struct pathname		*pnp;
	int			flags;
{
	ULOCKFS(dvp, VA_LOOKUP,
		ufs_lookup(dvp, nm, vpp, cred, pnp, flags));
}
static int
ufs_l_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode		*dvp;
	char			*nm;
	struct vattr		*vap;
	enum vcexcl		exclusive;
	int			mode;
	struct vnode		**vpp;
	struct ucred		*cred;
{
	ULOCKFS(dvp, VA_CREATE,
		ufs_create(dvp, nm, vap, exclusive, mode, vpp, cred));
}
static int
ufs_l_remove(vp, nm, cred)
	struct vnode	*vp;
	char		*nm;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_REMOVE,
		ufs_remove(vp, nm, cred));
}
static int
ufs_l_link(vp, tdvp, tnm, cred)
	struct vnode		*vp;
	register struct vnode	*tdvp;
	char			*tnm;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_LINK,
		ufs_link(vp, tdvp, tnm, cred));
}
static int
ufs_l_rename(sdvp, snm, tdvp, tnm, cred)
	struct vnode	*sdvp;
	char		*snm;
	struct vnode	*tdvp;
	char		*tnm;
	struct ucred	*cred;
{
	ULOCKFS(sdvp, VA_RENAME,
		ufs_rename(sdvp, snm, tdvp, tnm, cred));
}
static int
ufs_l_mkdir(dvp, nm, vap, vpp, cred)
	struct vnode		*dvp;
	char			*nm;
	register struct vattr	*vap;
	struct vnode		**vpp;
	struct ucred		*cred;
{
	ULOCKFS(dvp, VA_MKDIR,
		ufs_mkdir(dvp, nm, vap, vpp, cred));
}
static int
ufs_l_rmdir(vp, nm, cred)
	struct vnode	*vp;
	char		*nm;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_RMDIR,
		ufs_rmdir(vp, nm, cred));
}
static int
ufs_l_readdir(vp, uiop, cred)
	struct vnode	*vp;
	struct uio	*uiop;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_READDIR,
		ufs_readdir(vp, uiop, cred));
}
static int
ufs_l_symlink(dvp, lnm, vap, tnm, cred)
	register struct vnode	*dvp;
	char			*lnm;
	struct vattr		*vap;
	char			*tnm;
	struct ucred		*cred;
{
	ULOCKFS(dvp, VA_SYMLINK,
		ufs_symlink(dvp, lnm, vap, tnm, cred));
}
static int
ufs_l_lockctl(vp, ld, cmd, cred, clid)
	struct vnode		*vp;
	struct eflock		*ld;
	int			 cmd;
	struct ucred		*cred;
	int			 clid;
{
	ULOCKFS(vp, VA_LOCKCTL,
		ufs_lockctl(vp, ld, cmd, cred, clid));
}
static int
ufs_l_fid(vp, fidpp)
	struct vnode	 *vp;
	struct fid	**fidpp;
{
	ULOCKFS(vp, VA_FID,
		ufs_fid(vp, fidpp));
}
static int
ufs_l_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode		*vp;
	u_int			off, len;
	u_int			*protp;
	struct page		*pl[];
	u_int			plsz;
	struct seg		*seg;
	addr_t			addr;
	enum seg_rw		rw;
	struct ucred		*cred;
{
	int			vaccess;

	if (seg->s_ops != &segvn_ops)
		vaccess = VA_GETPRIVATE;
	else if (((struct segvn_data *)seg->s_data)->type != MAP_SHARED)
		vaccess = VA_GETPRIVATE;
	else if (rw == S_OTHER)
		vaccess = VA_GETWRITE;
	else if ((*seg->s_ops->checkprot)(seg, addr, len, PROT_WRITE) != 0)
		vaccess = VA_GETREAD;
	else
		vaccess = VA_GETWRITE;

	ULOCKFS(vp, vaccess,
		ufs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw,
			cred));
}
static int
ufs_l_putpage(vp, off, len, flags, cred)
	register struct vnode	*vp;
	u_int			 off, len;
	int			 flags;
	struct ucred		*cred;
{
	ULOCKFS(vp, VA_PUTPAGE,
		ufs_putpage(vp, off, len, flags, cred));
}
static int
ufs_l_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
	struct vnode	*vp;
	u_int		off;
	struct as	*as;
	addr_t		*addrp;
	u_int		len;
	u_int		prot, maxprot;
	u_int		flags;
	struct ucred	*cred;
{
	ULOCKFS(vp, VA_MAP,
		ufs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred));
}
static int
ufs_l_cntl(vp, cmd, idata, odata, iflag, oflag)
	struct vnode	*vp;
	int		 cmd, iflag, oflag;
	caddr_t		 idata, odata;

{
	ULOCKFS(vp, VA_CNTL,
		ufs_cntl(vp, cmd, idata, odata, iflag, oflag));
}

/*
 * ULOCKFS ROUTINES
 */

/*
 * ufs_lockfs_end
 *	Called at end of every VOP call
 */
ufs_lockfs_end(vaid, mp)
	int		vaid;
	struct mount	*mp;
{
	struct ulockfs	*ul	= mp->m_ul;

	/*
	 * if there are no more of these accesses outstanding
	 */
	if (--(ul->ul_vacount[vaid]) == 0)
		/*
		 * lock in progress for this access
		 */
		if (ul->ul_vamask & (1<<vaid))
			/*
			 * awaken locking process
			 */
			if (ul->ul_flags & ULOCKFS_VAWANT) {
				ul->ul_flags &= ~ULOCKFS_VAWANT;
				wakeup((caddr_t)mp);
			}
}

int	lockfs_interruptible	= 0;
/*
 * ufs_lockfs_begin
 *	Called at end of every VOP call
 */
ufs_lockfs_begin(vp, vaid, mpp)
	struct vnode	*vp;
	int		vaid;
	struct mount	**mpp;
{
	struct mount	*mp	= (struct mount *)(vp->v_vfsp->vfs_data);
	struct ulockfs	*ul	= mp->m_ul;

	*mpp = mp;

	/*
	 * current lock wants this access pended
	 */
	while (ul->ul_vamask & (1<<vaid)) {
		/*
		 * can't pend it because it is recursive
		 *	e.g., VOP_RDWR causing VOP_GETPAGE
		 */
		if ((VTOI(vp)->i_flag & ILOCKED) &&
		    (u.u_procp == (struct proc *)(VTOI(vp)->i_owner)))
			break;
		/*
		 * return EIO if hard locked
		 */
		if (LOCKFS_IS_HLOCK(UTOL(ul)))
			return (EIO);
		/*
		 * Don't pend nfsd's.  Return EIO and EAGAIN in u.u_XXX[0]
		 * and nfsd will drop request
		 */
		if (u.u_XXX[0] == ENOTBLK) {
			u.u_XXX[0] = EAGAIN;
			return (EIO);
		}
		if (lockfs_interruptible) {
			int	smask;
			int	s;
			int	interrupted;

			/*
			 * pend access interruptibly (for some signals)
			 * See rpc/clnt_kudp.c.  This is like an nfs mount
			 * with the intr option.
			 */
			s = splhigh();
			smask = u.u_procp->p_sigmask;
			u.u_procp->p_sigmask |= ~(sigmask(SIGHUP) |
				sigmask(SIGINT) | sigmask(SIGQUIT) |
				sigmask(SIGTERM));
			interrupted = sleep((caddr_t)mp, PLOCK+PCATCH);
			u.u_procp->p_sigmask = smask;
			(void) splx(s);
			if (interrupted)
				return (EINTR);
		} else
			(void) sleep((caddr_t)mp, PZERO);
	}
	/*
	 * inc 'access in progress' count for this access
	 */
	ul->ul_vacount[vaid]++;
	return (0);
}
/*
 * Lock types are really indexes into the lockfs_vamask array.
 * The accesses locked by a lock type can be changed by altering
 * the mask, or by adding a new mask and incrementing LOCKFS_MAXLOCK.
 */
#define	LOCKFS_MAXMASK		(32)
u_long	lockfs_vamask[LOCKFS_MAXMASK] = {	LOCKFS_ULOCK_MASK,
						LOCKFS_WLOCK_MASK,
						LOCKFS_NLOCK_MASK,
						LOCKFS_DLOCK_MASK,
						LOCKFS_HLOCK_MASK };
u_long	lockfs_maxlock	= LOCKFS_MAXLOCK;

/*
 * ufs_fiolfs
 *	file system locking ioctl handler
 */
static int
ufs_fiolfs(vp, lfup)
	struct vnode	*vp;		/* vnode for some inode on fs */
	struct lockfs	**lfup;		/* address of user lockfs struct */
{
	int		error;		/* error return */
	struct mount	*mp;		/* mount point of vp */
	struct ulockfs	*ul;		/* ulockfs struct for mp */
	struct lockfs	*lfc;		/* lockfs struct in ulockfs */
	struct lockfs	lfs;		/* save current lock */
	struct lockfs	lfd;		/* desired lock */

	/*
	 * must be superuser
	 */
	if (!suser())
		return (EPERM);

	/*
	 * mount point, ufs lockfs, and current lockfs
	 */
	mp  = (struct mount *)(vp->v_vfsp->vfs_data);
	ul  = mp->m_ul;
	lfc = UTOL(ul);

	/*
	 * if not already busy or hlocked, mark lock as busy
	 */
	if (LOCKFS_IS_BUSY(lfc))
		return (EBUSY);
	if (LOCKFS_IS_HLOCK(lfc))
		return (EIO);
	LOCKFS_SET_BUSY(lfc);

	/*
	 * get and check the user's lockfs struct
	 */
	if (error = ufs_getlfd(vp, lfup, &lfd, lfc))
		goto erridle;

	/*
	 * Freeze the file system (pend future accesses)
	 */
	if (error = ufs_freeze(mp, &lfd, &lfs))
		goto erridle;

	/*
	 * Quiesce (wait for outstanding accesses to finish)
	 */
	if (error = ufs_quiesce(mp))
		goto errout;

	/*
	 * at least everything *currently* dirty goes out
	 */
	if (!LOCKFS_IS_ULOCK(lfc))
		if (error = ufs_flush(mp))
			goto errout;

	/*
	 * reconcile superblock and inodes if fs was wlock'ed
	 */
	if (LOCKFS_IS_WLOCK(&lfs))
		if (error = ufs_reconcile(mp))
			goto errout;

	/*
	 * thaw down to lfd.lf_lock (wakeup pended processes)
	 */
	if (error = ufs_thaw(mp))
		goto errout;

	/*
	 * idle the lock struct
	 */
	LOCKFS_CLR_BUSY(lfc);

	/*
	 * free current comment
	 */
	kmem_free((caddr_t)lfs.lf_comment, (u_int)lfs.lf_comlen);

	/*
	 * return status (such as the new key)
	 */
	return (ufs_fiolfss(vp, lfup));

errout:
	/*
	 * if possible, apply original lock and clean up lock things
	 */
	ufs_unfreeze(mp, &lfs);
	(void) ufs_thaw(mp);
erridle:
	LOCKFS_CLR_BUSY(lfc);

	return (error);
}
/*
 * ufs_fioffs
 *	ioctl handler for FIOFFS
 */
static int
ufs_fioffs(vp, lfup)
	struct vnode	*vp;		/* some vnode on fs */
	struct lockfs	**lfup;		/* user's struct (must be NULL) */
{
	/*
	 * no struct needed, yet
	 */
	if (*lfup != NULL)
		return (EINVAL);
	/*
	 * at least everything *currently* dirty goes out
	 */
	return (ufs_flush((struct mount *)(vp->v_vfsp->vfs_data)));
}
/*
 * ufs_fiolfss
 *	ioctl handler for FIOLFSS
 */
static int
ufs_fiolfss(vp, lfup)
	struct vnode	*vp;		/* some vnode on fs */
	struct lockfs	**lfup;		/* user's lockfs struct */
{
	int		error;
	u_int		comlen;		/* length of user's comment buf */
	struct mount	*mp;
	struct ulockfs	*ul;
	struct lockfs	*lfc;		/* current lockfs struct */
	struct lockfs	lfu;		/* copy of user's lockfs struct */

	/*
	 * mount point and ulockfs and lockfs structs
	 */
	mp  = (struct mount *)(vp->v_vfsp->vfs_data);
	ul  = mp->m_ul;
	lfc = UTOL(ul);

	/*
	 * get user's lockfs struct
	 */
	if (error = copyin((caddr_t)*lfup, (caddr_t)&lfu,
		(u_int)(sizeof (struct lockfs))))
		goto errout;

	/*
	 * length of comment to return
	 */
	if (lfu.lf_comlen > lfc->lf_comlen)
		comlen = lfc->lf_comlen;
	else
		comlen = lfu.lf_comlen;

	/*
	 * return current lockfs struct to user
	 */
	lfu.lf_lock   = lfc->lf_lock;
	lfu.lf_key    = lfc->lf_key;
	lfu.lf_flags  = lfc->lf_flags;
	if (lfu.lf_comlen = comlen)
		if (error = copyout(lfc->lf_comment, lfu.lf_comment, comlen))
			goto errout;
	error = copyout((caddr_t)&lfu, (caddr_t)*lfup,
		(u_int)(sizeof (struct lockfs)));
errout:
	return (error);
}

/*
 * ufs_freeze
 *	pend future accesses for current lock and desired lock
 */
ufs_freeze(mp, lfd, lfs)
	struct mount	*mp;
	struct lockfs	*lfd;		/* desired lock */
	struct lockfs	*lfs;		/* save current lock here */
{
	struct ulockfs	*ul	= mp->m_ul;
	struct lockfs	*lfc	= UTOL(ul);	/* current lock */

	/*
	 * save current lock
	 */
	bcopy((caddr_t)lfc, (caddr_t)lfs, (u_int)sizeof (struct lockfs));

	/*
	 * move over selected lock fields into lockfs struct
	 */
	lfc->lf_lock	= lfd->lf_lock;
	lfc->lf_key	= lfd->lf_key;
	lfc->lf_comlen	= lfd->lf_comlen;
	lfc->lf_comment	= lfd->lf_comment;

	/*
	 * pend current and desired lock's vop accesses for now
	 */
	ul->ul_vamask |= lockfs_vamask[lfc->lf_lock];

	return (0);
}

/*
 * ufs_unfreeze
 *	lock failed, reset the old lock
 */
ufs_unfreeze(mp, lfr)
	struct mount	*mp;
	struct lockfs	*lfr;			/* reset this lock */
{
	u_int		comlen;
	caddr_t		comment;
	struct ulockfs	*ul	= mp->m_ul;
	struct lockfs	*lff	= UTOL(ul);	/* from this failed lock */

	/*
	 * can't unfreeze a hlock
	 */
	if (LOCKFS_IS_HLOCK(lff)) {
		/*
		 * free up comment from reset lock
		 */
		comlen  = lfr->lf_comlen;
		comment = lfr->lf_comment;
		goto errout;
	} else {
		/*
		 * free up comment from failed lock
		 */
		comlen  = lff->lf_comlen;
		comment = lff->lf_comment;
	}

	/*
	 * move over the LOCKFS_MOD flag
	 */
	if (LOCKFS_IS_MOD(lff))
		LOCKFS_SET_MOD(lfr);
	/*
	 * reset lock
	 */
	bcopy((caddr_t)lfr, (caddr_t)lff, (u_int)sizeof (struct lockfs));

	/*
	 * reset vop access mask
	 */
	ul->ul_vamask = lockfs_vamask[lfr->lf_lock];

errout:
	kmem_free(comment, comlen);
}
/*
 * ufs_quiesce
 *	wait for outstanding accesses to finish
 */
ufs_quiesce(mp)
	struct mount	*mp;			/* mount point */
{
	int		i;			/* index */
	u_long		vamask;			/* access mask */
	struct ulockfs	*ul	= mp->m_ul;	/* mp's ulockfs */

	/*
	 * for each access
	 */
	for (i = 0, vamask = ul->ul_vamask; i < VA_MAX; ++i) {
		/*
		 * if these accesses should finish
		 */
		if (vamask & (1<<i))
			/*
			 * wait for outstanding ones to finish
			 */
			while (ul->ul_vacount[i]) {
				ul->ul_flags |= ULOCKFS_VAWANT;
				if (sleep((caddr_t)mp, PLOCK+PCATCH))
					return (EINTR);
			}
	}

	return (0);
}

/*
 * ufs_thaw
 *	thaw file system lock down to current value
 */
ufs_thaw(mp)
	struct mount	*mp;
{
	int		error	= 0;
	struct ulockfs	*ul	= mp->m_ul;
	struct lockfs	*lfc	= UTOL(ul);
	int		noidel	= ULOCKFS_IS_NOIDEL(ul);

	/*
	 * if wlock or hlock
	 */
	if (LOCKFS_IS_WLOCK(lfc) || LOCKFS_IS_HLOCK(lfc)) {

		/*
		 * don't keep access times
		 * don't free deleted files
		 * if superblock writes are allowed, limit them to me for now
		 */
		ul->ul_flags |= (ULOCKFS_NOIACC|ULOCKFS_NOIDEL);
		if (ul->ul_sbowner != (struct proc *)-1)
			ul->ul_sbowner = u.u_procp;

		/*
		 * wait for writes for deleted files and superblock updates
		 */
		if (error = ufs_flush(mp))
			goto errout;

		/*
		 * no one can write the superblock
		 */
		ul->ul_sbowner = (struct proc *)-1;

		/*
		 * reset modified
		 */
		LOCKFS_CLR_MOD(lfc);

		/*
		 * special processing for wlock/hlock
		 */
		if (LOCKFS_IS_WLOCK(lfc))
			if (error = ufs_thaw_wlock(mp))
				goto errout;
		if (LOCKFS_IS_HLOCK(lfc))
			while (ufs_thaw_hlock(mp))
				if (error = ufs_flush(mp))
					goto errout;
	} else {

		/*
		 * okay to keep access times
		 * okay to free deleted files
		 * okay to write the superblock
		 */
		ul->ul_flags &= ~(ULOCKFS_NOIACC|ULOCKFS_NOIDEL);
		ul->ul_sbowner = NULL;

		/*
		 * flush in case deleted files are in memory
		 */
		if (noidel)
			if (error = ufs_flush(mp))
				goto errout;
	}

	/*
	 * allow all accesses except those needed for this lock
	 */
	ul->ul_vamask = lockfs_vamask[lfc->lf_lock];

	/*
	 * wakeup any pended accesses (appropriate ones will sleep again)
	 */
errout:
	wakeup((caddr_t)mp);
	return (error);
}

/*
 * ufs_flush
 *	flush at least everything that is currently dirty
 */
ufs_flush(mp)
	struct mount	*mp;
{
	int		error;
	int		saverror	= 0;
	struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	union ihead	*ih;
	struct inode	*ip;
	ino_t		*inop;		/* array of ino_t's (0 terminated) */
	ino_t		*cinop;		/* pointer into array */
	u_long		tino;		/* total length of array */

	/*
	 * get rid of dnlc entries
	 */
	(void) dnlc_purge();

#ifdef	QUOTA
	/*
	 * flush quota records
	 */
	(void) qsync(mp);
#endif	/* QUOTA */

	/*
	 * flush and synchronously invalidate page cache and inodes
	 */
	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		ufs_getino(mp, ih, &inop, &tino);
		for (cinop = inop; *cinop; ++cinop) {
			if (error = iget(mp->m_dev, fs, *cinop, &ip)) {
				saverror = error;
				continue;
			}
			if ((error = syncip(ip, B_ASYNC, 0)) == 0)
				error = syncip(ip, B_INVAL, 0);
			if (error)
				saverror = error;
			(void) iput(ip);
		}
		/*
		 * free the array of inode numbers
		 */
		if (inop != NULL)
			kmem_free((caddr_t)inop, (u_int)tino * sizeof (ino_t));
	}

	/*
	 * Push buf cache and block device page cache
	 */
	if (error = VOP_PUTPAGE(mp->m_devvp, 0, 0, B_ASYNC, u.u_cred))
		saverror = error;
	(void) bflush(mp->m_devvp);

	/*
	 * synchronously flush superblock and summary info
	 */
	if (fs->fs_ronly == 0) {
		fs->fs_fmod = 0;
		(void) sbupdate(mp);
	}

	/*
	 * synchronously flush and invalidate buf and page cache
	 */
	if (error = VOP_PUTPAGE(mp->m_devvp, 0, 0, B_INVAL, u.u_cred))
		saverror = error;
	(void) bsinval(mp->m_devvp);

	/*
	 * set the clean flag
	 */
	ufs_checkclean(mp);

	return (saverror);
}
/*
 * ufs_thaw_wlock
 *	special processing when thawing down to wlock
 */
ufs_thaw_wlock(mp)
	struct mount	*mp;
{
	int		s;
	union ihead	*ih;
	struct inode	*ip;
	struct vnode	*vp;
	struct page	*pp;
	ino_t		*inop;		/* array of ino_t's (0 terminated) */
	ino_t		*cinop;		/* pointer into array */
	u_long		tino;		/* total entries in inop */
	int		mlocks		= 0;
	struct fs	*fs		= mp->m_bufp->b_un.b_fs;

	/*
	 * look for mlock'ed pages
	 */
	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		ufs_getino(mp, ih, &inop, &tino);
		for (cinop = inop; *cinop; ++cinop) {
			if (iget(mp->m_dev, fs, *cinop, &ip))
				continue;
			if (fs->fs_ronly)
				ip->i_flag &= ~(IMOD|IMODACC|IACC|IUPD|ICHG);
			vp = ITOV(ip);
			if ((vp->v_type != VCHR) && (vp->v_type != VSOCK)) {
				s = splvm();
				if (pp = vp->v_pages)
					do {
						mlocks += pp->p_lckcnt;
						pp = pp->p_vpnext;
					} while (pp != vp->v_pages);
				(void) splx(s);
			}
			(void) iput(ip);
		}
		if (inop != NULL)
			kmem_free((caddr_t)inop, (u_int)tino * sizeof (ino_t));
	}

	return ((mlocks) ? EPERM : 0);
}
/*
 * ufs_thaw_hlock
 *	special processing when thawing down to hlock
 */
ufs_thaw_hlock(mp)
	struct mount	*mp;
{
	int		s;
	union ihead	*ih;
	struct inode	*ip;
	struct vnode	*vp;
	struct page	*pp;
	int		reflush;	/* reflush the file system */
	ino_t		*inop;		/* array of ino_t's (0 terminated) */
	ino_t		*cinop;		/* pointer into array */
	u_long		tino;		/* total entries in inop */
	struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	extern u_int	pages_pp_locked;

	/*
	 * clear i_flags and page locks and page mods just in case an
	 * error prevented them from being cleared during ufs_flush()
	 */
	for (ih = ihead, reflush = 0; ih < &ihead[INOHSZ]; ih++) {
		ufs_getino(mp, ih, &inop, &tino);
		for (cinop = inop; *cinop; ++cinop) {
			if (iget(mp->m_dev, fs, *cinop, &ip))
				continue;
			ip->i_flag &= ~(IMOD|IMODACC|IACC|IUPD|ICHG);
			vp = ITOV(ip);
			if ((vp->v_type != VCHR) && (vp->v_type != VSOCK)) {
				s = splvm();
				if (pp = vp->v_pages)
					do {
						reflush = 1;
						if (pp->p_lckcnt)
							--pages_pp_locked;
						pp->p_lckcnt = 0;
						hat_pagesync(pp);
						pp->p_mod = 0;
						pp->p_ref = 0;
						pp = pp->p_vpnext;
					} while (pp != vp->v_pages);
				(void) splx(s);
			}
			(void) iput(ip);
		}
		if (inop != NULL)
			kmem_free((caddr_t)inop, (u_int)tino * sizeof (ino_t));
	}
	return (reflush);
}

/*
 * ufs_reconcile_ip
 *	reconcile ondisk inode with incore inode
 */
ufs_reconcile_ip(mp, ip)
	struct mount	*mp;
	struct inode	*ip;		/* incore inode */
{
	int		i;
	int		ndaddr;
	int		niaddr;
	struct dinode	*dp;		/* ondisk inode */
	struct buf	*bp	= NULL;
	struct fs	*fs	= mp->m_bufp->b_un.b_fs;

	/*
	 * BIG BOO-BOO, reconciliation fails
	 */
	if (ip->i_flag & (IMOD|IMODACC|IACC|IUPD|ICHG))
		return (EPERM);

	/*
	 * get the dinode
	 */
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, itod(fs, ip->i_number)),
	    (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	}
	dp  = bp->b_un.b_dino;
	dp += itoo(fs, ip->i_number);

	/*
	 * some fields are not allowed to change
	 */
	if ((ip->i_mode  != dp->di_mode) ||
	    (ip->i_uid   != dp->di_uid) ||
	    (ip->i_gid   != dp->di_gid)) {
		brelse(bp);
		return (EACCES);
	}

	/*
	 * and some are allowed to change
	 */
	ip->i_size		= dp->di_size;
	ip->i_ic.ic_flags	= dp->di_ic.ic_flags;
	ip->i_blocks		= dp->di_blocks;
	ip->i_gen		= dp->di_gen;
	ip->i_nlink		= dp->di_nlink;
	if (ip->i_flag & IFASTSYMLNK) {
		ndaddr = 1;
		niaddr = 0;
	} else {
		ndaddr = NDADDR;
		niaddr = NIADDR;
	}
	for (i = 0; i < ndaddr; ++i)
		ip->i_db[i] = dp->di_db[i];
	for (i = 0; i < niaddr; ++i)
		ip->i_ib[i] = dp->di_ib[i];

	brelse(bp);
	return (0);
}
/*
 * ufs_reconcile_inodes
 *	reconcile all incore inodes for this fs with ondisk inodes
 */
ufs_reconcile_inodes(mp)
	struct mount	*mp;
{
	int		error	= 0;
	struct fs	*fs	= mp->m_bufp->b_un.b_fs;
	union ihead	*ih;
	struct inode	*ip;
	u_long		tino;
	ino_t		*inop;		/* array of ino_t's */
	ino_t		*cinop;		/* pointer into array */

	/*
	 * scan inode hash and reconcile all inodes found for this fs
	 */
	for (ih = ihead; (error == 0) && (ih < &ihead[INOHSZ]); ih++) {
		ufs_getino(mp, ih, &inop, &tino);
		for (cinop = inop; (error == 0) && *cinop; ++cinop) {
			if ((error = iget(mp->m_dev, fs, *cinop, &ip)) == 0) {
				error = ufs_reconcile_ip(mp, ip);
				(void) iput(ip);
			}
		}

		/*
		 * free the array of inode numbers
		 */
		if (inop != NULL)
			kmem_free((caddr_t)inop, (u_int)tino * sizeof (ino_t));
	}
	return (error);
}
/*
 * ufs_getino
 *	return array of ino_t's for inodes on the hash for given fs
 */
ufs_getino(mp, ih, inopp, tinop)
	struct mount	*mp;
	union ihead	*ih;
	ino_t		**inopp;
	u_long		*tinop;
{
	struct inode	*ip;
	struct inode	*aip	= (struct inode *)ih;
	struct fs	*fs	= mp->m_bufp->b_un.b_fs;
	ino_t		*inop	= NULL;
	u_long		tino	= 16;
	u_long		nino;

	/*
	 * allocate an array of inode numbers (null terminated)
	 */
again:
	if (inop)
		kmem_free((caddr_t)inop, (u_int)tino * sizeof (ino_t));
	tino <<= 1;
	inop = (ino_t *)kmem_zalloc((u_int)tino * sizeof (ino_t));

	/*
	 * fill in the array from the inodes for fs on hash ih
	 */
	for (ip = aip->i_forw, nino = 0; ip && ip != aip; ip = ip->i_forw) {
		if (ip->i_fs != fs) continue;
		if (nino == (tino-1))
			goto again;
		*(inop + nino++) = ip->i_number;
	}

	/*
	 * return the array
	 */
	*inopp = inop;
	*tinop = tino;
}
/*
 * ufs_reconcile
 *	reconcile ondisk superblock/inodes with any incore
 */
ufs_reconcile(mp)
	struct mount	*mp;
{
	int		error	= 0;

	/*
	 * get rid of as much inmemory data as possible
	 */
	if (error = ufs_flush(mp))
		goto errout;

	/*
	 * reconcile the superblock and inodes
	 */
	if (error = ufs_reconcile_fs(mp))
		goto errout;
	if (error = ufs_reconcile_inodes(mp))
		goto errout;

	/*
	 * get rid of as much inmemory data as possible
	 */
	if (error = ufs_flush(mp))
		goto errout;
errout:

	return (error);
}

/*
 * ufs_reconcile_fs
 *	reconcile incore superblock with ondisk superblock
 */
ufs_reconcile_fs(mp)
	struct mount	*mp;
{
	int		i;
	int		error;
	struct fs	*mfs; 	/* in-memory superblock */
	struct fs	*dfs;	/* on-disk   superblock */
	struct buf	*bp;	/* on-disk   superblock buf */

	/*
	 * BIG BOO-BOO
	 */
	mfs = mp->m_bufp->b_un.b_fs;
	if (mfs->fs_fmod)
		return (EPERM);

	/*
	 * get the on-disk copy of the superblock
	 */
	bp = bread(mp->m_devvp, SBLOCK, (int)mfs->fs_sbsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	}
	dfs = bp->b_un.b_fs;

	/*
	 * if superblock has changed too much, abort
	 */
	if ((mfs->fs_sblkno		!= dfs->fs_sblkno) ||
	    (mfs->fs_cblkno		!= dfs->fs_cblkno) ||
	    (mfs->fs_iblkno		!= dfs->fs_iblkno) ||
	    (mfs->fs_dblkno		!= dfs->fs_dblkno) ||
	    (mfs->fs_cgoffset		!= dfs->fs_cgoffset) ||
	    (mfs->fs_cgmask		!= dfs->fs_cgmask) ||
	    (mfs->fs_bsize		!= dfs->fs_bsize) ||
	    (mfs->fs_fsize		!= dfs->fs_fsize) ||
	    (mfs->fs_frag		!= dfs->fs_frag) ||
	    (mfs->fs_bmask		!= dfs->fs_bmask) ||
	    (mfs->fs_fmask		!= dfs->fs_fmask) ||
	    (mfs->fs_bshift		!= dfs->fs_bshift) ||
	    (mfs->fs_fshift		!= dfs->fs_fshift) ||
	    (mfs->fs_fragshift		!= dfs->fs_fragshift) ||
	    (mfs->fs_fsbtodb		!= dfs->fs_fsbtodb) ||
	    (mfs->fs_sbsize		!= dfs->fs_sbsize) ||
	    (mfs->fs_nindir		!= dfs->fs_nindir) ||
	    (mfs->fs_nspf		!= dfs->fs_nspf) ||
	    (mfs->fs_trackskew		!= dfs->fs_trackskew) ||
	    (mfs->fs_cgsize		!= dfs->fs_cgsize) ||
	    (mfs->fs_ntrak		!= dfs->fs_ntrak) ||
	    (mfs->fs_nsect		!= dfs->fs_nsect) ||
	    (mfs->fs_spc		!= dfs->fs_spc) ||
	    (mfs->fs_cpg		!= dfs->fs_cpg) ||
	    (mfs->fs_ipg		!= dfs->fs_ipg) ||
	    (mfs->fs_fpg		!= dfs->fs_fpg) ||
	    (mfs->fs_postblformat	!= dfs->fs_postblformat) ||
	    (mfs->fs_magic		!= dfs->fs_magic)) {
		brelse(bp);
		return (EACCES);
	}

	/*
	 * get new summary info
	 */
	if (error = ufs_getsummaryinfo(mp, dfs)) {
		brelse(bp);
		return (error);
	}

	/*
	 * release old summary info and update in-memory superblock
	 */
	kmem_free((caddr_t)mfs->fs_csp[0], (u_int)mfs->fs_cssize);
	for (i = 0; i < MAXCSBUFS; ++i)
		mfs->fs_csp[i] = dfs->fs_csp[i];

	/*
	 * update fields allowed to change
	 */
	mfs->fs_size		= dfs->fs_size;
	mfs->fs_dsize		= dfs->fs_dsize;
	mfs->fs_ncg		= dfs->fs_ncg;
	mfs->fs_minfree		= dfs->fs_minfree;
	mfs->fs_rotdelay	= dfs->fs_rotdelay;
	mfs->fs_rps		= dfs->fs_rps;
	mfs->fs_maxcontig	= dfs->fs_maxcontig;
	mfs->fs_maxbpg		= dfs->fs_maxbpg;
	mfs->fs_csmask		= dfs->fs_csmask;
	mfs->fs_csshift		= dfs->fs_csshift;
	mfs->fs_optim		= dfs->fs_optim;
	mfs->fs_csaddr		= dfs->fs_csaddr;
	mfs->fs_cssize		= dfs->fs_cssize;
	mfs->fs_ncyl		= dfs->fs_ncyl;
	mfs->fs_cstotal		= dfs->fs_cstotal;

	/* XXX What to do about sparecon? */

	/*
	 * ondisk clean flag overrides inmemory clean flag iff == FSBAD
	 */
	if (FSOKAY != (fs_get_state(dfs) + dfs->fs_time))
		mfs->fs_clean = FSBAD;
	if (dfs->fs_clean == FSBAD)
		mfs->fs_clean = FSBAD;
	brelse(bp);
	return (0);
}

/*
 * ufs_getlfd
 *	copy desired-lock struct from user to kernel space
 */
ufs_getlfd(vp, lfup, lfd, lfc)
	struct vnode	*vp;		/* vnode on fs to be locked */
	struct lockfs	**lfup;		/* address in user space */
	struct lockfs	*lfd;		/* desired lock */
	struct lockfs	*lfc;		/* current lock */
{
	int		error	= 0;
	u_int		comlen	= 0;
	caddr_t		comment	= NULL;

	/*
	 * copy user's lockfs struct into kernel memory
	 */
	if (error = copyin((caddr_t)*lfup, (caddr_t)lfd,
		(u_int)(sizeof (struct lockfs))))
		goto errout;

	/*
	 * check key
	 */
	if (!LOCKFS_IS_ULOCK(lfc))
		if (lfd->lf_key != lfc->lf_key) {
			error = EINVAL;
			goto errout;
		}
	lfd->lf_key = lfc->lf_key + 1;

	/*
	 * check bounds -- lf_lock is index into array of access masks
	 */
	if (lfd->lf_lock >= lockfs_maxlock) {
		error = EINVAL;
		goto errout;
	}

	/*
	 * can't wlock fs with accounting or local swap file
	 */
	if (LOCKFS_IS_WLOCK(lfd)) {
#ifdef	SYSACCT
		if (error = ufs_checkaccton(vp))
			goto errout;
#endif	SYSACCT
		if (error = ufs_checkswapon(vp))
			goto errout;
	}

	/*
	 * no input flags defined
	 */
	if (lfd->lf_flags != 0) {
		error = EINVAL;
		goto errout;
	}

	/*
	 * get comment
	 */
	if (comlen = lfd->lf_comlen) {
		if (comlen > LOCKFS_MAXCOMMENTLEN) {
			error = ENAMETOOLONG;
			goto errout;
		}
		comment = (caddr_t)kmem_alloc(comlen);
		if (error = copyin(lfd->lf_comment, comment, comlen))
			goto errout;
		lfd->lf_comment = comment;
	}

	return (error);
errout:
	if (comment)
		kmem_free(comment, comlen);
	return (error);
}

#ifdef	SYSACCT
/*
 * ufs_checkaccton
 *	check if accounting is turned on on this fs
 */
extern struct vnode	*acctp;
extern struct vnode	*savacctp;
ufs_checkaccton(vp)
	struct vnode	*vp;
{
	if (acctp && acctp->v_vfsp == vp->v_vfsp)
		return (EDEADLK);
	if (savacctp && savacctp->v_vfsp == vp->v_vfsp)
		return (EDEADLK);
	return (0);
}
#endif	/* SYSACCT */

/*
 * ufs_checkswapon
 *	check if local swapping is to file on this fs
 */
extern struct swapinfo	*swapinfo;
ufs_checkswapon(vp)
	struct vnode	*vp;
{
	struct swapinfo	*sip;

	for (sip = swapinfo; sip; sip = sip->si_next)
		if (sip->si_vp->v_vfsp == vp->v_vfsp)
			return (EDEADLK);
	return (0);
}
/*
 * ufs_lockfs_hold
 */
ufs_lockfs_hold(vfsp)
	struct vfs	*vfsp;
{
	struct mount	*mp;
	struct ulockfs	*ul;

	if ((mp = (struct mount *)vfsp->vfs_data) == NULL)
		return (EIO);
	ul = mp->m_ul;

	if (ul->ul_flags & ULOCKFS_FUMOUNT)
		return (EIO);
	ul->ul_hold++;
	return (0);
}
/*
 * ufs_lockfs_rele
 */
ufs_lockfs_rele(vfsp)
	struct vfs	*vfsp;
{
	struct ulockfs	*ul;

	ul = ((struct mount *)(vfsp->vfs_data))->m_ul;

	if (ul->ul_hold-- == 1)
		if (ul->ul_flags & ULOCKFS_WANT) {
			ul->ul_flags &= ~ULOCKFS_WANT;
			wakeup((caddr_t)ul);
		}
}
/*
 * ufs_lockfs_fumount
 */
ufs_lockfs_fumount(ul)
	struct ulockfs	*ul;
{
	ul->ul_flags |= ULOCKFS_FUMOUNT;
	while (ul->ul_hold) {
		ul->ul_flags |= ULOCKFS_WANT;
		if (sleep ((caddr_t)ul, PLOCK+PCATCH)) {
			ul->ul_flags &= ~ULOCKFS_FUMOUNT;
			return (EINTR);
		}
	}
	return (0);
}
/*
 * ufs_fioai
 *	file allocation information
 */
ufs_fioai(vp, faip)
	struct vnode	*vp;		/* file's vnode */
	struct filai	**faip;		/* user address of struct filai */
{
	int		error;
	int		boff;		/* offset within file system block */
	int		na;		/* # allocations returned */
	int		ne;		/* # entries left in array */
	size_t		size;		/* byte length of range */
	daddr_t		off;		/* byte offset into file */
	daddr_t		lbn;		/* logical fs block */
	daddr_t		bn;		/* disk sector number */
	daddr_t		bor;		/* beginning of range (sector) */
	daddr_t		lor;		/* length of range (sector) */
	daddr_t		lof;		/* length of file (sector) */
	struct filai	fai;		/* copy of users filai */
	struct fs	*fs;		/* file system (superblock) */
	struct inode	*ip;		/* vnode's inode */
	daddr_t		*da;		/* address of user array */

	/*
	 * inode and superblock
	 */
	ip = VTOI(vp);
	fs = ITOF(ip);

	/*
	 * get user's filia struct
	 */
	if (error = copyin((caddr_t)*faip, (caddr_t)&fai, (u_int)sizeof (fai)))
		return (error);

	ILOCK(ip);
	/*
	 * range checks
	 *	offset >= 2G || size >= 2G || (offset+size) >= 2G
	 *	offset >= length of file
	 *
	 */
	na = 0;
	if ((size = fai.fai_size) == 0)
		size = ip->i_size - fai.fai_off;

	if ((int)fai.fai_off < 0)
		goto errrange;
	if ((int)size < 0)
		goto errrange;
	if ((int)(fai.fai_off + size) < 0)
		goto errrange;
	if (fai.fai_off >= ip->i_size)
		goto errrange;

	/*
	 * beginning of range in sectors
	 * length of range in sectors
	 * length of file in sectors
	 */
	bor = btodb(fai.fai_off);
	off = dbtob(bor);
	lor = btodb(size) + ((size & (DEV_BSIZE-1)) ? 1 : 0);
	lof = btodb(ip->i_size) + ((ip->i_size & (DEV_BSIZE-1)) ? 1 : 0);
	if (lof < (bor + lor))
		lor = lof - bor;

	/*
	 * return allocation info until:
	 *	array fills
	 *	range is covered (end of file accounted for above)
	 */
	ne = fai.fai_num;
	da = fai.fai_daddr;
	while (lor && ne) {

		/*
		 * file system block and offset within block
		 */
		lbn  = lblkno(fs, off);
		boff = blkoff(fs, off);

		/*
		 * get frag address and convert to disk address
		 */
		if (error = bmap(ip, lbn, &bn, (daddr_t *)NULL,
				DEV_BSIZE, S_READ, 1))
			goto errout;
		if (bn == UFS_HOLE)
			bn = FILAI_HOLE;
		else
			bn = fsbtodb(fs, bn) + btodb(boff);

		/*
		 * return disk addresses.
		 * 	(file system blocks are contiguous on disk)
		 */
		do {
			if (error = suword((caddr_t)da, (int)bn))
				goto errout;
			if (bn != FILAI_HOLE)
				bn++;
			off += DEV_BSIZE;
			na++;
			da++;
			lor--;
			ne--;
		} while ((lbn == lblkno(fs, off)) && lor && ne);
	}
	/*
	 * update # of entries returned and current offset
	 */
	fai.fai_off = off;
errrange:
	fai.fai_num = na;
	if (error = copyout((caddr_t)&fai, (caddr_t)*faip, sizeof (fai)))
		goto errout;

errout:
	IUNLOCK(ip);
	return (error);
}
/*
 * ufs_fiodutimes
 *	set access/modified times but not change time.  Also, delay the update
 */
ufs_fiodutimes(vp, tvp)
	struct vnode	*vp;		/* file's vnode */
	struct timeval	**tvp;		/* user address of struct timeval */
{
	int		error	= 0;
	struct inode	*ip;
	struct timeval	tv;

	if (!suser())
		return (EPERM);

	ip = VTOI(vp);

	/*
	 * if NULL, use current time
	 */
	if (*tvp) {
		if (error = copyin((caddr_t)*tvp, (caddr_t)&tv, sizeof (tv)))
			return (error);
		if (tv.tv_usec < 0 || tv.tv_usec >= 1000000)
			return (EINVAL);
	} else
		tv = time;

	ILOCK(ip);
	ITIMES(ip);
	ip->i_atime = tv;
	ip->i_flag |= IMODACC;
	IUNLOCK(ip);

	return (error);
}
/*
 * ufs_fiodios
 *	return status of metadata updates
 */
ufs_fiodios(vp, diop)
	struct vnode	*vp;		/* file's vnode */
	u_long		**diop;		/* m_dio returned here */
{
	u_long		dio;

	dio = (ITOM(VTOI(vp)))->m_dio & ~MDIO_LOCK;
	return (suword((caddr_t)*diop, (int)(dio)));
}
/*
 * ufs_fiodio
 *	sandbag metadata updates
 */
ufs_fiodio(vp, diop)
	struct vnode	*vp;		/* file's vnode */
	u_long		**diop;		/* dio flags */
{
	int		error;
	int		clean;
	u_long		dio;
	struct inode	*ip;
	struct fs	*fs;
	struct mount	*mp;
	struct buf	*bp;

	/*
	 * check input conditions
	 */
	if (!suser())
		return (EPERM);

	error = copyin((caddr_t)*diop, (caddr_t)&dio, (u_int)(sizeof (u_long)));
	if (error)
		return (error);

	if (dio > 1)
		return (EINVAL);

	/*
	 * setup
	 */
	ip = VTOI(vp);
	fs = ITOF(ip);
	mp = ITOM(ip);

	/*
	 * lock access to the dio field
	 */
	while (mp->m_dio & MDIO_LOCK)
		if (sleep((caddr_t)mp, PLOCK+PCATCH))
			return (EINTR);
	if (mp->m_dio == dio)
		goto out;
	mp->m_dio = dio | MDIO_LOCK;

	/*
	 * enable/disable clean flag processing
	 */
	if ((mp->m_dio & MDIO_ON) || (ufs_flush(mp)))
		clean = FSSUSPEND;
	else
		clean = FSACTIVE;
	if (fs->fs_ronly == 0) {
		bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
		if (fs->fs_clean != FSBAD) {
			fs->fs_clean = clean;
			ufs_sbwrite(mp, fs, bp);
		} else
			brelse(bp);
	}
out:
	mp->m_dio &= ~MDIO_LOCK;
	wakeup((caddr_t)mp);

	return (0);
}
