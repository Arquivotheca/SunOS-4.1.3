#ifndef lint
static        char sccsid[] = "@(#)ufs_vnodeops.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include "boot/cmap.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include <ufs/mount.h>
#include <ufs/fsdir.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif
#include <sys/dirent.h>

#ifdef	NFS_BOOT
#undef u
extern struct user u;
static int dump_debug = 20;
#else 
#include <specfs/fifo.h>	/* this defines PIPE_BUF for ufs_getattr() */
#include <krpc/lockmgr.h>
#endif	 /* NFS_BOOT */

#define ISVDEV(t) ((t == VCHR) || (t == VBLK) || (t == VFIFO))


extern int ufs_open();
extern int ufs_close();
extern int ufs_rdwr();
extern int ufs_ioctl();
extern int ufs_select();
extern int ufs_getattr();
extern int ufs_access();
extern int ufs_lookup();
extern int ufs_readdir();
extern int ufs_readlink();
extern int ufs_fsync();
extern int ufs_inactive();
extern int ufs_bmap();
extern int ufs_strategy();
extern int ufs_bread();
extern int ufs_brelse();
extern int ufs_fid();
extern int ufs_badop();

struct vnodeops ufs_vnodeops = {
	ufs_open,
	ufs_close,
	ufs_rdwr,
	ufs_ioctl,
	ufs_select,
	ufs_getattr,
	ufs_badop,	/* ufs_setattr */
	ufs_access,
	ufs_lookup,
	ufs_badop,	/* ufs_create */
	ufs_badop,	/* ufs_remove */
	ufs_badop,	/* ufs_link */
	ufs_badop,	/* ufs_rename */
	ufs_badop,	/* ufs_mkdir */
	ufs_badop,	/* ufs_rmdir */
	ufs_readdir,
	ufs_badop,	/* ufs_symlink */
	ufs_readlink,
	ufs_fsync,
	ufs_inactive,
	ufs_bmap,
	ufs_strategy,
	ufs_bread,
	ufs_brelse,
	ufs_badop,	/* ufs_lockctl */
	ufs_fid,
	/* ufs_badop, */
};

int
ufs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6, "ufs_open(vpp 0x%x flag 0x%x cred 0x%x)\n", 
		vpp, flag, cred);
#endif	 /* DUMP_DEBUG */
	return (0);
}

/*ARGSUSED*/
int
ufs_close(vp, flag, cred)
	struct vnode *vp;
	int flag;
	struct ucred *cred;
{
	return (0);
}

/*
 * read or write a vnode
 */
/*ARGSUSED*/
int
ufs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6,
		"ufs_rdwr(vp 0x%x uiop 0x%x rw 0x%x ioflag 0x%x cred 0x%x)\n", 
		vp, uiop, rw, ioflag, cred);
#endif	 /* DUMP_DEBUG */

	ip = VTOI(vp);
	if ((ip->i_mode&IFMT) == IFREG) {
		ILOCK(ip);
		if ((ioflag & IO_APPEND) && (rw == UIO_WRITE)) {
			/*
			 * in append mode start at end of file.
			 */
			uiop->uio_offset = ip->i_size;
		}
		error = rwip(ip, uiop, rw, ioflag);
		IUNLOCK(ip);
	} else {
		error = rwip(ip, uiop, rw, ioflag);
	}

	return (error);
}

int
rwip(ip, uio, rw, ioflag)
	register struct inode *ip;
	register struct uio *uio;
	enum uio_rw rw;
	int ioflag;
{
	struct vnode *devvp;
	struct buf *bp;
	struct fs *fs;
	daddr_t lbn, bn;
	register int n, on, type;
	int size;
	long bsize;
	extern int mem_no;
	int error = 0;
	int iupdat_flag = 0;
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6, "rwip(ip 0x%x uio 0x%x rw 0x%x ioflag 0x%x)\n", 
		ip, uio, rw, ioflag);
#endif	 /* DUMP_DEBUG */

	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("rwip");
	if (rw == UIO_READ && uio->uio_resid == 0)
		return (0);
	if ((uio->uio_offset < 0 || (uio->uio_offset + uio->uio_resid) < 0))
		return (EINVAL);
	if (rw == UIO_READ)
		imark(ip, IACC);
	if (uio->uio_resid == 0)
		return (0);
	type = ip->i_mode&IFMT;
	if (type == IFCHR || type == IFBLK || type == IFIFO) {
		panic("rwip dev inode");
	}
#ifdef notdef
	if (rw == UIO_WRITE && type == IFREG &&
	    uio->uio_offset + uio->uio_resid >
	      u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
#ifdef	NFS_BOOT
		dprint(dump_debug, 6, "rwip(1): no write\n");
#else
		psignal(u.u_procp, SIGXFSZ);
		return (EFBIG);
#endif	/* NFS_BOOT */
	}
#endif notdef
	devvp = ip->i_devvp;
	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	u.u_error = 0;
	do {
		lbn = uio->uio_offset / bsize;
		on = uio->uio_offset % bsize;
		n = MIN((unsigned)(bsize - on), uio->uio_resid);
		if (rw == UIO_READ) {
			int diff = ip->i_size - uio->uio_offset;
			if (diff <= 0)
				return (0);
			if (diff < n)
				n = diff;
		}
		bn =
		    fsbtodb(fs, bmap(ip, lbn,
			 rw == UIO_WRITE ? B_WRITE: B_READ,
			 (int)(on+n), (ioflag & IO_SYNC) ? &iupdat_flag : 0));
		if (u.u_error || rw == UIO_WRITE && (long)bn<0)
			return (u.u_error);
		if (rw == UIO_WRITE &&
		   (uio->uio_offset + n > ip->i_size) &&
		   (type == IFDIR || type == IFREG || type == IFLNK)) {
#ifdef	 NFS_BOOT
			dprint(dump_debug, 6, "rwip(2): no write\n");
#else
			ip->i_size = uio->uio_offset + n;
			if (ioflag & IO_SYNC) {
				iupdat_flag = 1;
			}
#endif	/* NFS_BOOT */
		}
		size = blksize(fs, ip, lbn);
		if (rw == UIO_READ) {
			if ((long)bn<0) {
				bp = geteblk(size);
				clrbuf(bp);
			} else if (ip->i_lastr + 1 == lbn)
#ifdef	NFS_BOOT
				bp = bread(devvp, bn, size);
#else
				bp = breada(devvp, bn, size, rablock, rasize);
#endif	/* NFS_BOOT */
			else
				bp = bread(devvp, bn, size);
			ip->i_lastr = lbn;
		} else {
#ifdef	 NFS_BOOT
			dprint(dump_debug, 6, "rwip(3): no write\n");
#else
			int i, count;
			extern struct cmap *mfind();

			count = howmany(size, DEV_BSIZE);
			for (i = 0; i < count; i += CLBYTES/DEV_BSIZE)
				if (mfind(devvp, (daddr_t)(bn + i)))
					munhash(devvp, (daddr_t)(bn + i));
			if (n == bsize) 
				bp = getblk(devvp, bn, size);
			else
				bp = bread(devvp, bn, size);
#endif	/* NFS_BOOT */
		}
		n = MIN(n, bp->b_bcount - bp->b_resid);
		if (bp->b_flags & B_ERROR) {
			error = EIO;
			brelse(bp);
			goto bad;
		}
		u.u_error = uiomove(bp->b_un.b_addr+on, n, rw, uio);
		if (rw == UIO_READ) {
			if (n + on == bsize || uio->uio_offset == ip->i_size)
				bp->b_flags |= B_DONTNEED;
			brelse(bp);
		} else {
#ifdef	 NFS_BOOT
			dprint(dump_debug, 6, "rwip(4): no write\n");
#else
			if ((ioflag & IO_SYNC) || (ip->i_mode&IFMT) == IFDIR)
				bwrite(bp);
			else if (n + on == bsize) {
				bp->b_flags |= B_DONTNEED;
				bawrite(bp);
			} else
				bdwrite(bp);
			imark(ip, IUPD|ICHG);
			if (u.u_ruid != 0)
				ip->i_mode &= ~(ISUID|ISGID);
#endif	/* NFS_BOOT */
		}
	} while (u.u_error == 0 && uio->uio_resid > 0 && n != 0);
	if (iupdat_flag) {
#ifdef	 NFS_BOOT
			dprint(dump_debug, 6, "rwip(5): no write\n");
#else
		iupdat(ip, 1);
#endif	/* NFS_BOOT */
	}
	if (error == 0)				/* XXX */
		error = u.u_error;		/* XXX */

bad:
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6, "rwip: error 0x%x\n", error);
#endif	 /* DUMP_DEBUG */
	return (error);
}

/*ARGSUSED*/
int
ufs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
int
ufs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
int
ufs_getattr(vp, vap, cred)
	struct vnode *vp;
	register struct vattr *vap;
	struct ucred *cred;
{
	register struct inode *ip;

	ip = VTOI(vp);
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
	switch(ip->i_mode & IFMT) {

	case IFBLK:
		vap->va_blocksize = BLKDEV_IOSIZE;
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

/*ARGSUSED*/
int
ufs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{
	register struct inode *ip;
	int error;
#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "ufs_access(vp 0x%x mode 0x%x cred 0x%x)\n", 
		vp, mode, cred);
#endif	/* DUMP_DEBUG */


	ip = VTOI(vp);
	ilock(ip);
	error = iaccess(ip, mode);
	iunlock(ip);

#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "ufs_access: error 0x%x\n", 
		error);
#endif	/* DUMP_DEBUG */
	return (error);
}

/*ARGSUSED*/
int
ufs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register struct inode *ip;
	register int error;

	if (vp->v_type != VLNK)
		return (EINVAL);
	ip = VTOI(vp);
	ilock(ip);
	error = rwip(ip, uiop, UIO_READ, 0);
	iunlock(ip);

	return (error);
}

/*ARGSUSED*/
int
ufs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	return (0);
}

/*ARGSUSED*/
int
ufs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	return (0);
}

/*
 * Unix file system operations having to do with directory manipulation.
 */
/*ARGSUSED*/
ufs_lookup(dvp, nm, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
{
	register int error;
	struct inode *ip;

#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6,
		"ufs_lookup(dvp 0x%x nm %s vpp 0x%x cred 0x%x)\n", 
		dvp, nm, vpp, cred);
#endif	 /* DUMP_DEBUG */

	error = dirlook(VTOI(dvp), nm, &ip);
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6, "ufs_lookup: dirlook error 0x%x\n", error);
#endif	 /* DUMP_DEBUG */
	if (error == 0) {
		*vpp = ITOV(ip);
		iunlock(ip);
		/*
		 * If vnode is a device return special vnode instead
		 */
		if (ISVDEV((*vpp)->v_type)) {
#ifdef NFS_BOOT
#else
			struct vnode *newvp;
#endif NFS_BOOT

#ifdef	 NFS_BOOT
			/*
			 * Boot should not be looking up devices.
			 */
			dprint(dump_debug, 6,
				"ufs_lookup: tried to access device\n");
			
#else
			newvp = specvp(*vpp, (*vpp)->v_rdev);
			VN_RELE(*vpp);
			*vpp = newvp;
#endif	 /* NFS_BOOT */
		}
	}

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
        struct uio tuio;
        struct iovec tiov;
        register struct direct *idp;
        register struct dirent *odp;
        off_t offset;
        caddr_t inbuf, outbuf;
        int bytes_read, bytes_wanted, total_bytes_wanted;
        int incount = 0;
        int outcount = 0;
        int eof;
        int error;

        ip = VTOI(vp);
        iovp = uiop->uio_iov;
        total_bytes_wanted = iovp->iov_len;
        bytes_wanted = iovp->iov_len & ~(DIRBLKSIZ - 1);

        /* Setup temporary space to read directory entries into */
        inbuf = (caddr_t) kmem_alloc((u_int)bytes_wanted);
        tuio.uio_segflg = UIO_SYSSPACE;
        tuio.uio_iov = &tiov;
        tuio.uio_iovcnt = 1;
        tuio.uio_resid = bytes_wanted;
        tuio.uio_offset = offset = uiop->uio_offset & ~(DIRBLKSIZ - 1);

        /* Get space to change dir. entries into filesystem independent format */
        outbuf = (caddr_t)kmem_alloc((u_int)(total_bytes_wanted + sizeof(struct dirent)));
        odp = (struct dirent *) outbuf;

nextblk:
        tuio.uio_resid = bytes_wanted;
        tiov.iov_len = bytes_wanted;
        tiov.iov_base = inbuf;

        /* Read a chunk */
        if (error = rwip(ip, &tuio, UIO_READ, 0))
                goto out;

        bytes_read = bytes_wanted - tuio.uio_resid;
        eof = (bytes_read < bytes_wanted);
        incount = 0;
        idp = (struct direct *) inbuf;
 
        /* Transform to file-system independent format */
        while (incount < bytes_read) {
                /* Skip to requested offset, and skip empty entries */
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
                incount += idp->d_reclen;
                offset += idp->d_reclen;
                idp = (struct direct *)((int)idp + idp->d_reclen);
        }
 
        /* Read whole block, but got no entries, read another if not eof */
        if (!eof && !outcount)
                goto nextblk;

        /* Copy out the entry data */
        if (error = uiomove(outbuf, outcount, UIO_READ, uiop))
                goto out;

        uiop->uio_offset = offset;
out:
        ITIMES(ip);
        kmem_free(inbuf, (u_int)bytes_wanted);
        kmem_free(outbuf, (u_int)(total_bytes_wanted + sizeof(struct dirent)));
        return (error);
}

/*ARGSUSED*/
rdwri(rw, ip, base, len, offset, seg, aresid)
	enum uio_rw rw;
	struct inode *ip;
	caddr_t base;
	int len;
	int offset;
	int seg;
	int *aresid;
{
	register int error;

	error = EACCES;
	return (error);
}

/*ARGSUSED*/
int
ufs_bmap(vp, lbn, vpp, bnp)
	struct vnode *vp;
	daddr_t lbn;
	struct vnode **vpp;
	daddr_t *bnp;
{
	return (0);
}

/*
 * read a logical block and return it in a buffer
 */
/*ARGSUSED*/
int
ufs_bread(vp, lbn, bpp, sizep)
	struct vnode *vp;
	daddr_t lbn;
	struct buf **bpp;
	long *sizep;
{
#ifdef	 NEVER
	register struct inode *ip;
	register struct buf *bp;
	register daddr_t bn;
	register int size;
#ifdef	 DUMP_DEBUG1
	dprint(dump_debug, 6, "ufs_bread(vp 0x%x lbn 0x%x bpp 0x%x sizep 0x%x)\n", 
		vp, lbn, bpp, sizep);
#endif	 /* DUMP_DEBUG */

	ip = VTOI(vp);
	size = blksize(ip->i_fs, ip, lbn);
	bn = fsbtodb(ip->i_fs, bmap(ip, lbn, B_READ));
	if ((long)bn < 0) {
		bp = geteblk(size);
		clrbuf(bp);
	} else if (ip->i_lastr + 1 == lbn) {
		bp = breada(ip->i_devvp, bn, size, rablock, rasize);
	} else {
		bp = bread(ip->i_devvp, bn, size);
	}
	ip->i_lastr = lbn;
	imark(ip, IACC);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	} else {
		*bpp = bp;
		return (0);
	}
#endif	 /* NEVER */
}

/*
 * release a block returned by ufs_bread
 */
/*ARGSUSED*/
ufs_brelse(vp, bp)
	struct vnode *vp;
	struct buf *bp;
{
}

int
ufs_badop()
{
	panic("ufs_badop");
}

/*
 * Record-locking requests are passed to the local Lock-Manager daemon.
 */
/*ARGSUSED*/
int
ufs_lockctl(vp, ld, cmd, cred)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
{
}

/*ARGSUSED*/
ufs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	return (0);
}

/*ARGSUSED*/
ufs_strategy(bp)
	register struct buf *bp;
{
}

/*
 * Stubs
 */

/*
 * Zero out a buffer's data portion.
 */
blkclr(addr, count)
	char *addr;
	int count;
{
        bzero(addr, count);

}
