#ifndef lint 
#ident "@(#)hsfs_vnodeops.c 1.1 92/07/30" 
#endif

/*
 * Vnode operations for High Sierra filesystem
 *
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/dirent.h>
#include <sys/pathname.h>
#include <sys/mman.h>
#include <sys/debug.h>
#include <sys/vmmeter.h>
#include <sys/file.h>
#include <vm/hat.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <vm/swap.h>

#include <hsfs/hsfs_spec.h>
#include <hsfs/hsfs_isospec.h>
#include <hsfs/hsfs_node.h>
#include <hsfs/hsfs_private.h>

extern char *strcpy();


/*ARGSUSED*/
static int
hsfs_rdwr(vp, uiop, rw, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	enum uio_rw rw;
	int ioflag;
	struct ucred *cred;
{

	if (rw != UIO_READ) {
		return (EINVAL);
	}

	if ((uiop->uio_offset < 0) ||
	    ((uiop->uio_offset + uiop->uio_resid) < 0))
		return (EINVAL);
	if (uiop->uio_resid == 0)
		return (0);
	return (hs_read(vp, uiop, ioflag, cred));
}

/*ARGSUSED*/
int
hs_read(vp, uiop, ioflag, cred)
	struct vnode *vp;
	struct uio *uiop;
	int ioflag;
	struct ucred *cred;
{
	struct hsnode *hp;
	register u_int off;
	register int mapon, on;
	register addr_t base;
	u_int	filesize;
	int nbytes, n;
	u_int flags;
	int error;

	hp = VTOH(vp);

	/*
	 * if vp is of type VDIR, make sure dirent
	 * is filled up with all info (because of ptbl)
	 */
	if (vp->v_type == VDIR) {
		if (hp->hs_dirent.ext_size == 0)
			hs_filldirent(vp, &hp->hs_dirent);
	}
	filesize = hp->hs_dirent.ext_size;

	if (uiop->uio_offset >= filesize)
		return (0);		/* at or beyond EOF */

	do {
		/* map file to correct page boundary */
		off = uiop->uio_offset & MAXBMASK;
		mapon = uiop->uio_offset & MAXBOFFSET;
		/* set read in data size */
		on = (uiop->uio_offset) & PAGEOFFSET;
		nbytes = MIN(PAGESIZE - on, uiop->uio_resid);
		/* adjust down if > EOF */
		n = MIN((filesize - uiop->uio_offset), nbytes);
		if (n == 0) {
			error = 0;
			goto out;
		}

		/* map the file into memory */
		base = segmap_getmap(segkmap, vp, off);
		error = uiomove(base+mapon, n, UIO_READ, uiop);
		if (error == 0) {
		/*
		 * if read a whole block, or read to eof,
		 *  won't need this buffer again soon.
		 */
			if (n + on == PAGESIZE ||
			    uiop->uio_offset == filesize)
				flags = SM_DONTNEED;
			else
				flags = 0;
			error = segmap_release(segkmap, base, flags);
		} else
			(void) segmap_release(segkmap, base, 0);

	} while (error == 0 && uiop->uio_resid > 0);

out:
	return (error);
}

/*ARGSUSED*/
static int
hsfs_getattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	register struct hsnode *hp;
	register struct vfs *vfsp;

	hp = VTOH(vp);
	vfsp = vp->v_vfsp;

	if ((hp->hs_dirent.ext_size == 0) && (vp->v_type == VDIR))
		hs_filldirent(vp, &hp->hs_dirent);
	vap->va_type = IFTOVT(hp->hs_dirent.mode);
	vap->va_mode = hp->hs_dirent.mode;
	vap->va_uid = hp->hs_dirent.uid;
	vap->va_gid = hp->hs_dirent.gid;

	vap->va_fsid = vfsp->vfs_fsid.val[0];

	vap->va_nodeid = hp->hs_dirent.ext_lbn;

	vap->va_nlink = hp->hs_dirent.nlink;
	vap->va_size =  hp->hs_dirent.ext_size;

	vap->va_blocksize = vfsp->vfs_bsize;

	vap->va_atime.tv_sec = hp->hs_dirent.adate.tv_sec;
	vap->va_atime.tv_usec = hp->hs_dirent.adate.tv_usec;
	vap->va_mtime.tv_sec = hp->hs_dirent.mdate.tv_sec;
	vap->va_mtime.tv_usec = hp->hs_dirent.mdate.tv_usec;
	vap->va_ctime.tv_sec = hp->hs_dirent.cdate.tv_sec;
	vap->va_ctime.tv_usec = hp->hs_dirent.cdate.tv_usec;

	if (vp->v_type == VCHR || vp->v_type == VBLK)
		vap->va_rdev = hp->hs_dirent.r_dev;
	else
		vap->va_rdev = 0;


	/* va_blocks should be in unit of kilobytes */
	/* xar should also be included */
	/* min. logical block size always at least 512 bytes) */
	vap->va_blocks =
		(howmany(vap->va_size, vap->va_blocksize) /* data blocks */
			+ hp->hs_dirent.xar_len)	/* xar blocks */
		* (vap->va_blocksize/512);

	return (0);
}

/*ARGSUSED*/
static int
hsfs_readlink(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct ucred *cred;
{
	register int error;
	struct hsnode *hp;


	if (vp->v_type != VLNK)
		return (EINVAL);

	hp = VTOH(vp);

	if (hp->hs_dirent.sym_link == (char *)NULL) {
		return (ENOENT);
	}

	error = uiomove((caddr_t)hp->hs_dirent.sym_link,
		(int) MIN(strlen(hp->hs_dirent.sym_link),
		uiop->uio_resid), UIO_READ, uiop);

	return (error);
}

/*ARGSUSED*/
static int
hsfs_inactive(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

	/* proceed to free the hsnode */
	/*
	 * if there is no pages associated with the
	 * hsnode, put at front of the free queue,
	 * else put at the end
	 */
	(void) hs_freenode(VTOH(vp), vp->v_vfsp,
		vp->v_pages == NULL? 1 : 0);
	return (0);
}


/*ARGSUSED*/
static int
hsfs_lookup(dvp, nm, vpp, cred, pnp, flags)
	struct vnode *dvp;
	char *nm;
	struct vnode **vpp;
	struct ucred *cred;
	struct pathname *pnp;
	int flags;
{
	int namelen;

	namelen = strlen(nm);

	/*
	 * If we're looking for ourself, life is simple.
	 */
	if ((namelen == 1) && (*nm == '.')) {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	return (hs_dirlook(dvp, nm, namelen, vpp, cred));
}



/*ARGSUSED*/
static int
hsfs_readdir(vp, uiop, cred)
	struct vnode *vp;
	struct uio *uiop;
	struct cred *cred;
{
	struct hsnode *dhp;
	struct hsfs *fsp;
	struct hs_direntry hd;
	register struct dirent *nd;
	int error;
	register u_int offset;		/* real offset in directory */
	u_int dirsiz;			/* real size of directory */
	u_char *blkp;
	register int hdlen;		/* length of hs directory entry */
	register int ndlen;		/* length of dirent entry */
	int bytes_wanted;
	int bufsize;			/* size of dirent buffer */
	register char *outbuf;		/* ptr to dirent buffer */
	char *dname;
	int dnamelen;
	int direntsz;
	struct fbuf *fbp;
	u_int last_offset;		/* last index into current dir block */


	dhp = VTOH(vp);
	fsp = VFS_TO_HSFS(vp->v_vfsp);
	if (dhp->hs_dirent.ext_size == 0)
		hs_filldirent(vp, &dhp->hs_dirent);
	dirsiz = dhp->hs_dirent.ext_size;
	offset = uiop->uio_offset;
	if (offset >= dirsiz) {			/* at or beyond EOF */
		return (0);
	}

	dname = (char *)new_kmem_alloc(MAXNAMELEN + 1, KMEM_SLEEP);
	if (dname == (char *)NULL) {
		/* XXX something better to return  here? */
		return (0);
	}

	bufsize = uiop->uio_resid + sizeof (struct dirent);
	outbuf = (char *) new_kmem_alloc((u_int) bufsize, KMEM_SLEEP);

	if (outbuf == (char *)NULL) {
		/* XXX something better to return here? */
		kmem_free(dname, MAXNAMELEN + 1);
		return (0);
	}

	nd = (struct dirent *)outbuf;
	direntsz = (char *)nd->d_name - (char *)nd;

	while (offset < dirsiz) {
		if ((offset & MAXBMASK) + MAXBSIZE > dirsiz)
			bytes_wanted = dirsiz - (offset & MAXBMASK);
		else
			bytes_wanted = MAXBSIZE;

		error = fbread(vp, offset & MAXBMASK,
			(unsigned int)bytes_wanted, S_READ, &fbp);
		if (error) {
			fbrelse(fbp, S_READ);
			goto done;
		}
		blkp = (u_char *) fbp->fb_addr;
		last_offset = (offset & MAXBMASK) + fbp->fb_count - 1;

#define	rel_offset(offset) ((offset) & MAXBOFFSET)	/* index into blkp */

		while (offset < last_offset) {
			/*
			 * Directory Entries cannot span sectors.
			 * Unused bytes at the end of each sector are zeroed.
			 * Therefore, detect this condition when the size
			 * field of the directory entry is zero.
			 */
			hdlen = (int)((u_char)
				HDE_DIR_LEN(&blkp[rel_offset(offset)]));
			if (hdlen == 0) {
				/* advance to next sector boundary */
				offset = (offset & MAXHSMASK) + HS_SECTOR_SIZE;

				/*
				 * Have we reached the end of current block?
				 */
				if (offset > last_offset)
					break;
				else
					continue;
			}

			/* make sure this is nullified before  reading it */
			bzero ((caddr_t)&hd, sizeof (hd));

			/*
			 * Just ignore invalid directory entries.
			 * XXX - maybe hs_parsedir() will detect EXISTENCE bit
			 */
			if (!hs_parsedir(fsp, &blkp[rel_offset(offset)],
				&hd, dname, &dnamelen)) {
				/*
				 * Determine if there is enough room
				 */
				ndlen = (direntsz + dnamelen + 1 + (NBPW-1))
					& ~(NBPW-1);
				if ((ndlen + (char *)nd -
					outbuf) > uiop->uio_resid) {
					fbrelse(fbp, S_READ);
					goto done; /* output buffer full */
				}
				nd->d_off = offset + hdlen;

				nd->d_fileno = hd.ext_lbn;
				if (nd->d_fileno == 0)
					nd->d_fileno = hd.r_dev + (u_int)1;

				nd->d_namlen = dnamelen;
				(void) strcpy(nd->d_name, dname);
				nd->d_reclen = (u_short)ndlen;
				nd = (struct dirent *)((char *)nd + ndlen);
			}

			offset += hdlen;
		}
		fbrelse(fbp, S_READ);
	}

	/*
	 * Got here for one of the following reasons:
	 *	1) outbuf is full (error == 0)
	 *	2) end of directory reached (error == 0)
	 *	3) error reading directory sector (error != 0)
	 *	4) directory entry crosses sector boundary (error == 0)
	 *
	 * If any directory entries have been copied, don't report
	 * case 4.  Instead, return the valid directory entries.
	 *
	 * If no entries have been copied, report the error.
	 * If case 4, this will be indistiguishable from EOF.
	 */
done:
	ndlen = ((char *)nd - outbuf);
	if (ndlen != 0) {
		error = uiomove(outbuf, ndlen, UIO_READ, uiop);
		uiop->uio_offset = offset;
	}
	(void) kmem_free(dname, MAXNAMELEN + 1);
	(void) kmem_free(outbuf, (u_int) bufsize);
	return (error);
}


static int
hsfs_fid(vp, fidpp)
	struct vnode *vp;
	struct fid **fidpp;
{
	register struct hsnode *hp;
	register struct hsfid *fid;

	hp = VTOH(vp);
	fid = (struct hsfid *)new_kmem_alloc(sizeof (*fid), KMEM_SLEEP);
	fid->hf_len = sizeof (*fid) - sizeof (fid->hf_len);
	fid->hf_ext_lbn = hp->hs_dirent.ext_lbn;
	fid->hf_dir_lbn = hp->hs_dir_lbn;
	fid->hf_dir_off = (u_short) hp->hs_dir_off;
	*fidpp = (struct fid *)fid;
	return (0);
}


/* The following are only needed to write High Sierra filesystems */

/*ARGSUSED*/
static int
hsfs_setattr(vp, vap, cred)
	struct vnode *vp;
	struct vattr *vap;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_create(dvp, nm, vap, exclusive, mode, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *vap;
	enum vcexcl exclusive;
	int mode;
	struct vnode **vpp;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_remove(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_rename(odvp, onm, ndvp, nnm, cred)
	struct vnode *odvp;
	char *onm;
	struct vnode *ndvp;
	char *nnm;
	struct ucred *cred;
{
	return (EINVAL);
}


/*ARGSUSED*/
static int
hsfs_link(vp, tdvp, tnm, cred)
	struct vnode *vp;
	struct vnode *tdvp;
	char *tnm;
	struct ucred *cred;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_symlink(dvp, lnm, tva, tnm, cred)
	struct vnode *dvp;
	char *lnm;
	struct vattr *tva;
	char *tnm;
	struct ucred *cred;
{

	return (EINVAL);
}


/*ARGSUSED*/
static int
hsfs_mkdir(dvp, nm, va, vpp, cred)
	struct vnode *dvp;
	char *nm;
	struct vattr *va;
	struct vnode **vpp;
	struct ucred *cred;
{
	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_rmdir(dvp, nm, cred)
	struct vnode *dvp;
	char *nm;
	struct ucred *cred;
{
	return (EINVAL);
}

/* The rest of the VOPs are only called by the kernel */

#ifdef KERNEL
/*ARGSUSED*/
static int
hsfs_open(vpp, flag, cred)
	struct vnode **vpp;
	int flag;
	struct ucred *cred;
{

	return (0);
}

/*ARGSUSED*/
static int
hsfs_close(vp, flag, count, cred)
	struct vnode *vp;
	int flag;
	int count;
	struct ucred *cred;
{

	return (0);
}

/*ARGSUSED*/
static int
hsfs_ioctl(vp, com, data, flag, cred)
	struct vnode *vp;
	int com;
	caddr_t data;
	int flag;
	struct ucred *cred;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_select(vp, which, cred)
	struct vnode *vp;
	int which;
	struct ucred *cred;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_access(vp, mode, cred)
	struct vnode *vp;
	int mode;
	struct ucred *cred;
{

	return (hs_access(vp, mode, cred));
}

/*ARGSUSED*/
static int
hsfs_fsync(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{

	return (0);
}

/*ARGSUSED*/
static int
hsfs_lockctl(vp, ld, cmd, cred, clid)
	struct vnode *vp;
	struct flock *ld;
	int cmd;
	struct ucred *cred;
	int clid;
{

	return (EINVAL);
}

/*
 * the seek time of a CD-ROM is very slow, and data transfer
 * rate is even worse (max. 150K per sec).  The design
 * decision is to reduce access to cd-rom as much as possible,
 * and to transfer a sizable block (read-ahead) of data at a time.
 * UFS style of read ahead one block at a time is not appropriate,
 * and is not supported
 */

/*
 * KLUTSIZE should be a multiple of PAGESIZE and <= MAXPHYS.
 */
#define	KLUSTSIZE	(56 * 1024)
/* we don't support read ahead */
int hsfs_lostpage;	/* no. of times we lost original page */

/*ARGSUSED*/
hsfs_getapage(vp, off, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	register u_int off;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	struct hsnode *hp;
	struct hsfs *fsp;
	dev_t dev;
	int err;
	struct buf *bp, *bp2, *bp3, *bp4;
	struct page *pp, *pp2, **ppp, *pagefound;
	u_int	bof;
	struct vnode *devvp;
	u_int	blkoff;
	int	blksz;
	u_int	io_off, io_len;
	u_int	xlen;
	int	filsiz;
	int	klustsz;
	int	intlf;
	int	intlfsz = 0;
	int	nio;
	int	xarsiz;
	int	leftover;

	/*
	 * XXX since we read in 7 pages at a time, should first check
	 * if the page is already in.  Will do in next time
	 */
	hp = VTOH(vp);
	fsp = VFS_TO_HSFS(vp->v_vfsp);
	devvp = fsp->hsfs_devvp;
	dev = devvp->v_rdev;

	/* file data size */
	filsiz = hp->hs_dirent.ext_size;
	/* disk addr for start of file */
	bof = LBN_TO_BYTE(hp->hs_dirent.ext_lbn, vp->v_vfsp);
	/* xarsiz byte must be skipped for data */
	xarsiz = hp->hs_dirent.xar_len << fsp->hsfs_vol.lbn_shift;

	/* klustsz is a multiple of 8K */
	/* but in interleaving, klustsz is limited to interleaving size */
	/* to avoid the complexity of releasing unused "skip" data blocks */
	/* This may have serious performance implications */
	/* compute klutze size */
	if (intlf = hp->hs_dirent.intlf_sz + hp->hs_dirent.intlf_sk) {
		/* interleaving */
		/* convert total (sz+sk) into bytes */
		intlf = LBN_TO_BYTE(intlf, vp->v_vfsp);
		/* convert interleaving size into bytes */
		intlfsz = LBN_TO_BYTE(hp->hs_dirent.intlf_sz, vp->v_vfsp);
		/* read in only the interleaving size */
		klustsz = roundup(MIN(intlfsz, KLUSTSIZE), PAGESIZE);
	}
	/* no interleaving, set default klutsize */
	else
		klustsz = KLUSTSIZE;

reread:
	err = 0;
	bp = bp2 = bp3 = bp4 = NULL;
	pagefound = NULL;
	nio = 0;

/* convert file relative offset into absolute byte  offset */

	/* search page in buffer */
	if ((pagefound = page_find(vp, off)) == NULL) {
		/*
		 * Need to really do disk IO to get the page.
		 */
		blkoff = (off / klustsz) * klustsz;

		if (blkoff + klustsz <= filsiz)
			blksz = klustsz;
		else
#if sun4c
		/*
		 * campus driver requires min size to be 1 sector
		 * may cause problem from the last file in the cdrom
		 * if the logical block size is less than 2k
		 */
			blksz = roundup((filsiz - blkoff), HS_SECTOR_SIZE);
#else
			blksz = roundup((filsiz - blkoff), DEV_BSIZE);
#endif

		/*
		 * try to limit amount of data read in to klutsz
		 * or interleaving size or remaining unread sector
		 * from last read
		 */
		if (intlfsz) blksz = MIN(blksz, (intlfsz-(blkoff%intlfsz)));

		pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
			blkoff, (u_int) blksz, 0);

		if (pp == NULL)
			panic("spec_getapage pvn_kluster");
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
			*ppp = NULL;	    /* terminate list */
		}
		bp = pageio_setup(pp, io_len, devvp, ppp == NULL ?
		    (B_ASYNC | B_READ) : B_READ);
		bp->b_dev = dev;

		/* compute disk address for interleving */
		/* diskoff = (off / sz) * (sk + sz) + (off % sz)  */
		bp->b_blkno = intlf ? btodb(bof+  (xarsiz+io_off) /
			intlfsz * intlf + ((xarsiz+io_off) % intlfsz))
				: btodb(bof + xarsiz + io_off);

		bp->b_un.b_addr = 0;

		/*
		 * Zero part of page which we are not
		 * going to be reading from disk now.
		 */
		xlen = io_len & PAGEOFFSET;
		if (xlen != 0) {
			pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
			/*
			 * xlen must be zero for non-interleaving file
			 * except reading the last sector
			 */
			if ((leftover = filsiz-(io_off+io_len)) > 0) {
				if (intlf) {
					/*
					 * find out how many reads needed to
					 * fill a page, be careful of eof
					 */
					leftover = MIN(PAGESIZE-xlen, leftover);
					nio = howmany(leftover, intlfsz);
					pp->p_nio = nio+1;
					/*
					 * at most three io needed
					 * to fill a 8K page from 2K sector
					 */
					if (nio > 3)
						panic("bad nio\n");
				} else {
					/*
					 * our policy is to allocate
					 * a klutsz of multiple of 8K
					 * except last read
					 */
					panic("bad klustsz\n");
				}
			} else
				nio = 0;
		} else
			nio = 0;

		(*bdevsw[major(dev)].d_strategy)(bp);

		u.u_ru.ru_majflt++;
		if (seg == segkmap)
		u.u_ru.ru_inblock++;    /* count as `read' */
		cnt.v_pgin++;
		cnt.v_pgpgin += btopr(io_len);

	}

	/* for interleaving only */
	/*
	 * since interleaving size may not be multiple of PAGESIZE,
	 * we have to fill up the remaining page area with data
	 * from another interleaving data area in the CD-ROM
	 */
	if (nio) {
		if (pp != NULL) {
			/* xlen is the amount of data in a page */
			/* xlen is less than PAGESIZE */
			io_off = io_off + xlen; /* compute new io offset */
			io_len = MIN((PAGESIZE - xlen), intlfsz);
			/* adjust for eof */
			io_len = MIN(io_len, filsiz-io_off);
			bp2 = pageio_setup(pp, io_len, devvp, ppp == NULL ?
				(B_ASYNC | B_READ) : B_READ);
			bp2->b_dev = dev;

			bp2->b_blkno = btodb(bof+(xarsiz+io_off) /
				intlfsz * intlf + ((xarsiz+io_off) % intlfsz));

			bp2->b_un.b_addr = (caddr_t) xlen;

			xlen = (io_off + io_len) & PAGEOFFSET;
			if (xlen != 0) {
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
			}


			(*bdevsw[major(dev)].d_strategy)(bp2);

			u.u_ru.ru_majflt++;
			if (seg == segkmap)
				u.u_ru.ru_inblock++;    /* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
		if (nio == 1)
			goto intlf_done;
		if (pp != NULL) {
			io_off = io_off + io_len; /* compute new io offset */
			io_len = MIN((PAGESIZE - xlen), intlfsz);
			/* adjust for eof */
			io_len = MIN(io_len, filsiz-io_off);
			bp3 = pageio_setup(pp, io_len, devvp, ppp == NULL ?
				(B_ASYNC | B_READ) : B_READ);
			bp3->b_dev = dev;

			bp3->b_blkno = btodb(bof+(xarsiz+io_off) /
				intlfsz * intlf + ((xarsiz+io_off) % intlfsz));

			bp3->b_un.b_addr = (caddr_t) xlen;

			xlen = (io_off + io_len) & PAGEOFFSET;
			if (xlen != 0) {
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
			}


			(*bdevsw[major(dev)].d_strategy)(bp3);

			u.u_ru.ru_majflt++;
			if (seg == segkmap)
			u.u_ru.ru_inblock++;    /* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}
		if (nio == 2) goto intlf_done;
		if (pp != NULL) {
			io_off = io_off + io_len; /* compute new io offset */
			io_len = MIN((PAGESIZE - xlen), intlfsz);
			/* adjust for eof */
			io_len = MIN(io_len, filsiz-io_off);
			bp4 = pageio_setup(pp, io_len, devvp, ppp == NULL ?
				(B_ASYNC | B_READ) : B_READ);
			bp4->b_dev = dev;

			bp4->b_blkno = btodb(bof+(xarsiz+io_off) /
				intlfsz * intlf + ((xarsiz+io_off) % intlfsz));

			bp4->b_un.b_addr = (caddr_t) xlen;

			xlen = (io_off + io_len) & PAGEOFFSET;
			if (xlen != 0) {
				pagezero(pp->p_prev, xlen, PAGESIZE - xlen);
			}


			(*bdevsw[major(dev)].d_strategy)(bp4);

			u.u_ru.ru_majflt++;
			if (seg == segkmap)
			u.u_ru.ru_inblock++;    /* count as `read' */
			cnt.v_pgin++;
			cnt.v_pgpgin += btopr(io_len);
		}

	}

intlf_done:

	if (bp != NULL && pl != NULL) {
		if (err == 0)
			err = biowait(bp);
		else
			(void) biowait(bp);

		(void) pageio_done(bp);

		if (nio == 0) goto intlf_iodone;
		/* second read for page (nio == 1) */
		if (bp2 != NULL) {
			if (err == 0)
				err = biowait(bp2);
			else
				(void) biowait(bp2);
			(void) pageio_done(bp2);
		}
		if (nio == 1) goto intlf_iodone;

		/* third read for page (nio == 2) */
		if (bp3 != NULL) {
			if (err == 0)
				err = biowait(bp3);
			else
				(void) biowait(bp3);
			(void) pageio_done(bp3);
		}
		if (nio == 2) goto intlf_iodone;

		/* fourth read for page (nio == 3) */
		if (bp4 != NULL) {
			if (err == 0)
				err = biowait(bp4);
			else
				(void) biowait(bp4);
			(void) pageio_done(bp4);
		}
	} else if (pagefound != NULL) {
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
			if (pagefound->p_pagein || pagefound->p_free)
				pagefound = page_lookup(vp, off);
		}
		(void) splx(s);
		if (pagefound == NULL || pagefound->p_offset != off ||
		    pagefound->p_vnode != vp || pagefound->p_gone) {
			hsfs_lostpage++;
			goto reread;
		}
		if (pl != NULL) {
			PAGE_HOLD(pagefound);
			pl[0] = pagefound;
			pl[1] = NULL;
			u.u_ru.ru_minflt++;
		}
	}

intlf_iodone:
	if (err && pl != NULL) {
		for (ppp = pl; *ppp != NULL; *ppp++ = NULL)
			PAGE_RELE(*ppp);
	}

	return (err);

}

static int
hsfs_getpage(vp, off, len, protp, pl, plsz, seg, addr, rw, cred)
	struct vnode *vp;
	register u_int off, len;
	u_int *protp;
	struct page *pl[];
	u_int plsz;
	struct seg *seg;
	addr_t addr;
	enum seg_rw rw;
	struct ucred *cred;
{
	struct hsnode *hp;
	u_int filsiz;
	int err;

	/* does not support write */
	if (rw == S_WRITE) {
		panic("write attempt on READ ONLY HSFS");
	}

	hp = VTOH(vp);

	/* assume one extent only */
	/*
	 * because of the use of path table, directory entry
	 * may have ext_size == 0.  This should be taken care
	 * of by hsfs_read.  Note hsfs_map does not allow
	 * mapping of VDIR file
	 */
	filsiz = hp->hs_dirent.ext_size;

	/* off should already be adjusted for xar */

	if (off + len > filsiz + PAGEOFFSET && seg != segkmap)
		return (EFAULT);

	if (protp != NULL)
		*protp = PROT_READ | PROT_EXEC | PROT_USER;

	if (len <= PAGESIZE)
		err = hsfs_getapage(vp, off, protp, pl, plsz, seg, addr,
		    rw, cred);
	else
		err = pvn_getpages(hsfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cred);

	return (err);
}

/*ARGSUSED*/
static int
hsfs_putpage(vp, off, len, flags, cred)
	struct vnode *vp;
	u_int off;
	u_int len;
	int flags;
	struct ucred *cred;
{

	return (EINVAL);
}

#define	sunos4_1

#ifdef sunos4_0
/*ARGSUSED*/
static int
hsfs_map(vp, off, as, addr, len, prot, maxprot, flags, cred)
	struct vnode *vp;
	u_int off;
	struct as *as;
	addr_t addr;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segvn_crargs vn_a;
	int error;

	/* VFS_RECORD(vp->v_vfsp, VS_MAP, VS_CALL); */

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_type != VREG)
		return (ENODEV);

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	(void) as_unmap(as, addr, len);
	return (as_map(as, addr, len, segvn_create, (caddr_t)&vn_a));
}

#else
/*ARGSUSED*/
static int
hsfs_map(vp, off, as, addrp, len, prot, maxprot, flags, cred)
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

	/* VFS_RECORD(vp->v_vfsp, VS_MAP, VS_CALL); */

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

	if (vp->v_type != VREG) {
		return (ENODEV);
	}

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
#endif

/*ARGSUSED*/
static int
hsfs_dump(dvp, addr, bn, count)
	struct vnode *dvp;
	caddr_t addr;
	int bn;
	int count;
{

	return (EINVAL);
}

/*ARGSUSED*/
static int
hsfs_cmp(vp1, vp2)
	struct vnode *vp1;
	struct vnode *vp2;
{

	return (vp1 == vp2);
}

#else

static int
hsfs_badop()
{

	panic("hsfs_badop");
}

#define	hsfs_open	hsfs_badop
#define	hsfs_close	hsfs_badop
#define	hsfs_ioctl	hsfs_badop
#define	hsfs_select	hsfs_badop
#define	hsfs_access	hsfs_badop
#define	hsfs_fsync	hsfs_badop
#define	hsfs_lockctl	hsfs_badop
#define	hsfs_getpage	hsfs_badop
#define	hsfs_putpage	hsfs_badop
#define	hsfs_map		hsfs_badop
#define	hsfs_dump	hsfs_badop
#define	hsfs_cmp		hsfs_badop
#endif /* !KERNEL */


struct vnodeops hsfs_vnodeops = {
	hsfs_open,
	hsfs_close,
	hsfs_rdwr,
	hsfs_ioctl,
	hsfs_select,
	hsfs_getattr,
	hsfs_setattr,
	hsfs_access,
	hsfs_lookup,
	hsfs_create,
	hsfs_remove,
	hsfs_link,
	hsfs_rename,
	hsfs_mkdir,
	hsfs_rmdir,
	hsfs_readdir,
	hsfs_symlink,
	hsfs_readlink,
	hsfs_fsync,
	hsfs_inactive,
	hsfs_lockctl,
	hsfs_fid,
	hsfs_getpage,
	hsfs_putpage,
	hsfs_map,
	hsfs_dump,
	hsfs_cmp,
};
