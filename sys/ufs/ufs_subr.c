#ident	"@(#)ufs_subr.c 1.1 92/07/30 SMI"	/* from UCB 4.5 83/03/21 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <ufs/fs.h>
#ifdef KERNEL
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/vaccess.h>	/* FIOLFS */
#include <sys/lockfs.h>		/* FIOLFS */

#include <ufs/inode.h>
#include <ufs/mount.h>
#include <ufs/lockfs.h>		/* FIOLFS */

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>
#include <vm/pvn.h>
#include <vm/seg_map.h>
#include <vm/swap.h>
#include <machine/seg_kmem.h>

int syncprt = 0;

struct	inode **ifreet;

/*
 * UFS DEVICE ERROR DETECTION
 */

long ufs_deverrs	= 0;

/*
 * ufs_callback_deverr
 *	called by biodone when (b_flag & B_ERROR)
 */
ufs_callback_deverr(bp)
	register struct buf		*bp;
{
	/*
	 * ignore read errors and soft errors (EOM, page_abort(), ...)
	 */
	if ((bp->b_flags & B_READ) || (bp->b_error == EINVAL))
		return (0);

	ufs_deverrs++;

	return (1);
}

/*
 * ufs_get_deverr
 */
ufs_get_deverr(mp)
	register struct mount	*mp;
{
	register int		s;
	register long		errors;

	/*
	 * Get #errors for this file system
	 */
	s = splbio();
	errors = getbioerror(mp->m_dev, (caddr_t)0, ufs_callback_deverr);
	if (errors > 0)
		ufs_deverrs -= errors;
	(void) splx(s);
	return (errors);
}

/*
 * ufs_check_deverr
 */
ufs_check_deverr(mp, fs)
	register struct mount	*mp;
	register struct fs	*fs;
{
	register struct buf	*bp;

	/*
	 * ignore if no errors
	 */
	if ((ufs_deverrs == 0) || (ufs_get_deverr(mp) <= 0))
		return;

	/*
	 * set clean flag to FSBAD
	 */
	if (fs->fs_clean != FSBAD) {
		bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
		fs->fs_clean = FSBAD;
		ufs_sbwrite(mp, fs, bp);
	}
	/*
	 * try once to throw away the bioerror stuff
	 */
	(void) freebioerror(mp->m_dev, (caddr_t)0, ufs_callback_deverr);
}

/*
 * Update performs the ufs part of `sync'.  It goes through the disk
 * queues to initiate sandbagged IO; goes through the inodes to write
 * modified nodes; and it goes through the mount table to initiate
 * the writing of the modified super blocks.
 */
update()
{
	register struct inode *ip;
	register struct inode *lip;
	register struct inode *iq;
	register struct mount *mp;
	register struct vnode *vp;
	register union  ihead *ih;
	struct fs *fs;
	static int updlock;
	extern struct inode *ifreeh;

	/*
	 * See buid 1053582
	 */
	if (mounttab == NULL)
		return;

	if (updlock)
		return;
	updlock++;

	if (syncprt)
		bufstats();
	/*
	 * Write back modified superblocks.
	 * Consistency check that the superblock of
	 * each file system is still in the buffer cache.
	 */
	MRLOCK();
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
		if (mp->m_bufp == NULL || mp->m_dev == NODEV)
			continue;
		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_fmod == 0)
			continue;
		if (fs->fs_ronly != 0) {
			printf("fs = %s\n", fs->fs_fsmnt);
			panic("update: ro fs mod");
		}
		fs->fs_fmod = 0;
		sbupdate(mp);
	}
	MRULOCK();
	/*
	 * Write back each (modified) inode,
	 * but don't sync back pages if vnode is
	 * part of the virtual swap device.
	 */
	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		for (ip = ih->ih_chain[0], lip = NULL;
			ip && ip != (struct inode *)ih; ip = ip->i_forw) {
			vp = ITOV(ip);
			/*
			 * Skip locked & inactive inodes.
			 * Skip inodes w/ no pages and no inode changes.
			 * Skip inodes from read only vfs's.
			 */
			if ((ip->i_flag & ILOCKED) != 0 ||
			    ((vp->v_pages == NULL) &&
			    ((ip->i_flag&(IMODACC|IMOD|IACC|IUPD|ICHG))==0)) ||
			    (vp->v_vfsp == NULL) ||
			    ((vp->v_vfsp->vfs_flag & VFS_RDONLY) != 0))
				continue;
			ilock(ip);
			VN_HOLD(vp);

			/*
			 * from iget(); remove from free list
			 */
			if ((ip->i_flag & IREF) == 0) {
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
				ip->i_flag |= IREF;
			}

			if (lip != NULL)
				iput(lip);
			lip = ip;
			/*
			 * Don't sync pages for swap files, but make sure inode
			 * is up to date.  For other files, push everything out.
			 */
			if (IS_SWAPVP(vp)) {
				IUPDAT(ip, 1);
			} else {
				(void) syncip(ip, B_ASYNC, 1);
				(void) syncip(ip, 0, 1);
			}
		}
		if (lip != NULL)
			iput(lip);
	}


	/*
	 * Force stale buffer cache information to be flushed,
	 * for all devices.  This should cause any remaining control
	 * information (e.g., cg and inode info) to be flushed back.
	 */
	bflush((struct vnode *)0);

	/*
	 * For each mounted filesystem, update clean flag if warranted.
	 */
	MRLOCK();
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
		if (mp->m_bufp == NULL || mp->m_dev == NODEV)
			continue;
	        bwait(mp->m_devvp);
		ufs_checkclean(mp);
	}
	MRULOCK();

	updlock = 0;
}

/*
 * Flush all the pages associated with an inode using the given flags
 * then force inode information to be written back using the given flags.
 */
syncip(ip, flags, waitfor)
	register struct inode *ip;
{
	int err;
	int	fastsymflag;		/* fast symbolic link is active */
	long	fastsymlnk[FSL_SIZE/4]; /* for sun4 alignment */
	ino_t	ino;			/* old ino number*/
	long	igen;			/* old generation number */

	if (ip->i_fs == NULL)
		return (0);			/* not active */
	fastsymflag = ip->i_flag & IFASTSYMLNK;
	if (fastsymflag) {
		/* remove fast symbolic links from inode */
		int i;

		ino = ip->i_number;
		igen = ip->i_gen;
		(void) bcopy((caddr_t) &ip->i_db[1],
			(caddr_t) fastsymlnk, (u_int)ip->i_size);
		ip->i_flag &= ~IFASTSYMLNK;
		for (i = 1; i < NDADDR && ip->i_db[i]; i++)
			ip->i_db[i] = 0;
		for (i = 0; i < NIADDR && ip->i_ib[i]; i++)
			ip->i_ib[i] = 0;
	}
	if (ITOV(ip)->v_pages == NULL || ITOV(ip)->v_type == VCHR ||
	    ITOV(ip)->v_type == VSOCK)
		err = 0;
	else
		err = VOP_PUTPAGE(ITOV(ip), 0, 0, flags, (struct ucred *)0);
	IUPDAT(ip, waitfor);

	/* restore fast symbolic links back to incore inode*/
	if (fastsymflag) {
		if (ip->i_number == ino && ip->i_gen == igen) {
			(void) bcopy((caddr_t) fastsymlnk,
				(caddr_t) &ip->i_db[1], (u_int)ip->i_size);
			ip->i_flag |= IFASTSYMLNK;
		}
	}
	return (err);
}

/*
 * Flush all indirect blocks related to an inode.
 *
 * N.B. No support for three levels of indirect blocks.
 */
int
sync_indir(ip)
	register struct inode *ip;
{
	register int i;
	register daddr_t blkno;
	register daddr_t lbn;		/* logical blkno of last blk in file */
	register daddr_t clbn;		/* current logical blk */
	register daddr_t *bap;
	struct fs *fs;
	struct buf *bp;
	int bsize;

	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	lbn = lblkno(fs, ip->i_size - 1);
	if (lbn < NDADDR)
		return (0);		/* No indirect blocks used */
	if (lbn < NDADDR + NINDIR(fs)) {
		/* File has one indirect block. */
		blkflush(ip->i_devvp, (daddr_t)fsbtodb(fs, ip->i_ib[0]));
		return (0);
	}

	/* Write out all the first level indirect blocks */
	for (i = 0; i <= NIADDR; i++) {
		if ((blkno = ip->i_ib[i]) == 0)
			continue;
		blkflush(ip->i_devvp, (daddr_t)fsbtodb(fs, blkno));
	}
	/* Write out second level of indirect blocks */
	if ((blkno = ip->i_ib[1]) == 0)
		return (0);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, blkno), bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (EIO);
	}
	bap = bp->b_un.b_daddr;
	clbn = NDADDR + NINDIR(fs);
	for (i = 0; i < NINDIR(fs); i++) {
		if (clbn > lbn)
			break;
		clbn += NINDIR(fs);
		if ((blkno = bap[i]) == 0)
			continue;
		blkflush(ip->i_devvp, (daddr_t)fsbtodb(fs, blkno));
	}
	brelse(bp);
	return (0);
}
/*
 * Check that a specified block number is in range.
 */
badblock(fs, bn)
	register struct fs *fs;
	daddr_t bn;
{

	if ((unsigned)bn >= fs->fs_size) {
		printf("bad block %d, ", bn);
		fserr(fs, "bad block");
		return (1);
	}
	return (0);
}

struct mount *
getmp(dev)
	dev_t dev;
{
	register struct mount *mp;
	register struct fs *fs;

	MRLOCK();
	for (mp = mounttab; mp != NULL; mp = mp->m_nxt) {
		if (mp->m_bufp == NULL || mp->m_dev != dev)
			continue;
		fs = mp->m_bufp->b_un.b_fs;
		if (fs->fs_magic != FS_MAGIC) {
			printf("dev = 0x%x, fs = %s\n", dev, fs->fs_fsmnt);
			panic("getmp: bad magic");
		}
		break;
	}
	MRULOCK();
	return (mp);
}

/*
 * Print out statistics on the current allocation of the buffer pool.
 * Can be enabled to print out on every ``sync'' by setting "syncprt".
 */
bufstats()
{
	int s, i, j, count;
	register struct buf *bp, *dp;
	int counts[MAXBSIZE/CLBYTES+1];
	static char *bname[BQUEUES] = { "LRU", "AGE", "EMPTY" };

	for (bp = bfreelist, i = 0; bp < &bfreelist[BQUEUES]; bp++, i++) {
		count = 0;
		for (j = 0; j <= MAXBSIZE/CLBYTES; j++)
			counts[j] = 0;
		s = splbio();
		for (dp = bp->av_forw; dp != bp; dp = dp->av_forw) {
			counts[dp->b_bufsize/CLBYTES]++;
			count++;
		}
		(void) splx(s);
		printf("%s: total-%d", bname[i], count);
		for (j = 0; j <= MAXBSIZE/CLBYTES; j++)
			if (counts[j] != 0)
				printf(", %d-%d", j * CLBYTES, counts[j]);
		printf("\n");
	}
}

/*
 * Variables for maintaining the free list of fbuf structures.
 */
static struct fbuf *fb_free;
static int nfb_incr = 0x10;

/*
 * Return a pointer to locked kernel virtual address for
 * the given <vp, off> for len bytes.  It is not allowed to
 * have the offset cross a MAXBSIZE boundary over len bytes.
 */
int
fbread(vp, off, len, rw, fbpp)
	struct vnode *vp;
	register u_int off;
	u_int len;
	enum seg_rw rw;
	struct fbuf **fbpp;
{
	register addr_t addr;
	register u_int o;
	register struct fbuf *fb;
	faultcode_t err;

	o = off & MAXBOFFSET;
	if (o + len > MAXBSIZE)
		panic("fbread");
	addr = segmap_getmap(segkmap, vp, off & MAXBMASK);
	err = as_fault(&kas, addr + o, len, F_SOFTLOCK, rw);
	if (err) {
		(void) segmap_release(segkmap, addr, 0);
		return (FC_CODE(err) == FC_OBJERR ? FC_ERRNO(err) : EIO);
	}
	fb = (struct fbuf *)new_kmem_fast_alloc((caddr_t *)&fb_free,
			sizeof (*fb_free), nfb_incr, KMEM_SLEEP);
	fb->fb_addr = addr + o;
	fb->fb_count = len;
	*fbpp = fb;
	return (0);
}

/*
 * Similar to fbread() but we call segmap_pagecreate instead of using
 * as_fault for SOFTLOCK to create the pages without using VOP_GETPAGE
 * and then we zero up to the length rounded to a page boundary.
 * XXX - this won't work right when bsize < PAGESIZE!!!
 */
void
fbzero(vp, off, len, fbpp)
	struct vnode *vp;
	u_int off;
	u_int len;
	struct fbuf **fbpp;
{
	addr_t addr;
	register u_int o, zlen;

	o = off & MAXBOFFSET;
	ASSERT(o + len <= MAXBSIZE);
	addr = segmap_getmap(segkmap, vp, off & MAXBMASK) + o;

	*fbpp = (struct fbuf *)new_kmem_fast_alloc((caddr_t *)&fb_free,
			sizeof (*fb_free), nfb_incr, KMEM_SLEEP);
	(*fbpp)->fb_addr = addr;
	(*fbpp)->fb_count = len;
	segmap_pagecreate(segkmap, addr, len, 1);

	/*
	 * Now we zero all the memory in the mapping we are interested in.
	 */
	zlen = (addr_t)ptob(btopr(len + addr)) - addr;
	ASSERT(zlen >= len && o + zlen <= MAXBSIZE);
	bzero(addr, zlen);
}

/*
 * Release the fb using the rw mode specified
 */
void
fbrelse(fb, rw)
	register struct fbuf *fb;
	enum seg_rw rw;
{
	addr_t addr;

	(void) as_fault(&kas, fb->fb_addr, fb->fb_count, F_SOFTUNLOCK, rw);
	addr = (addr_t)((u_int)fb->fb_addr & MAXBMASK);
	(void) segmap_release(segkmap, addr, 0);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fb);
}

/*
 * Perform a direct write using the segmap_release and the mapping
 * information contained in the inode.  Upon return the fb is invalid.
 */
int
fbwrite(fb, ip)
	register struct fbuf *fb;
	register struct inode *ip;
{
	int err;
	addr_t addr;
	struct mount *mp	= ITOM(ip);

	ufs_notclean(mp, ITOF(ip));

	(void) as_fault(&kas, fb->fb_addr, fb->fb_count, F_SOFTUNLOCK, S_WRITE);
	addr = (addr_t)((u_int)fb->fb_addr & MAXBMASK);
	err = segmap_release(segkmap, addr,
		(mp->m_dio & MDIO_ON) ? (u_int)0 : SM_WRITE);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fb);
	return (err);
}

/*
 * Perform an indirect write using the given fbuf to the given
 * file system block number.  Upon return the fb is invalid.
 */
int
fbiwrite(fb, ip, bn)
	register struct fbuf *fb;
	register struct inode *ip;
	daddr_t bn;
{
	register struct buf *bp;
	int err;
	addr_t addr;

	ufs_notclean(ITOM(ip), ITOF(ip));

	/*
	 * Allocate a temp bp using pageio_setup, but then use it
	 * for physio to the area mapped by fbuf which is currently
	 * all locked down in place.
	 *
	 * XXX - need to have a generalized bp header facility
	 * which we build up pageio_setup on top of.  Other places
	 * (like here and in device drivers for the raw io case)
	 * could then use these new facilities in a more straight
	 * forward fashion instead of playing all theses games.
	 */
	bp = pageio_setup((struct page *)NULL, fb->fb_count, ip->i_devvp,
	    B_WRITE);
	bp->b_flags &= ~B_PAGEIO;		/* XXX */
	bp->b_flags |= B_PHYS;
	bp->b_un.b_addr = fb->fb_addr;
	bp->b_blkno = fsbtodb(ip->i_fs, bn);
	bp->b_dev = ip->i_dev;
	bp->b_proc = NULL;			/* i.e. the kernel */

	(*bdevsw[major(ip->i_dev)].d_strategy)(bp);

	err = biowait(bp);
	pageio_done(bp);

	(void) as_fault(&kas, fb->fb_addr, fb->fb_count, F_SOFTUNLOCK, S_OTHER);
	addr = (addr_t)((u_int)fb->fb_addr & MAXBMASK);
	if (err == 0)
		err = segmap_release(segkmap, addr, 0);
	else
		(void) segmap_release(segkmap, addr, 0);
	kmem_fast_free((caddr_t *)&fb_free, (caddr_t)fb);

	return (err);
}
#endif KERNEL

extern	int around[9];
extern	int inside[9];
extern	u_char *fragtbl[];

/*
 * Update the frsum fields to reflect addition or deletion
 * of some frags.
 */
fragacct(fs, fragmap, fraglist, cnt)
	struct fs *fs;
	int fragmap;
	long fraglist[];
	int cnt;
{
	int inblk;
	register int field, subfield;
	register int siz, pos;

	inblk = (int)(fragtbl[fs->fs_frag][fragmap]) << 1;
	fragmap <<= 1;
	for (siz = 1; siz < fs->fs_frag; siz++) {
		if ((inblk & (1 << (siz + (fs->fs_frag % NBBY)))) == 0)
			continue;
		field = around[siz];
		subfield = inside[siz];
		for (pos = siz; pos <= fs->fs_frag; pos++) {
			if ((fragmap & field) == subfield) {
				fraglist[siz] += cnt;
				pos += siz;
				field <<= siz;
				subfield <<= siz;
			}
			field <<= 1;
			subfield <<= 1;
		}
	}
}

/*
 * Block operations
 */

/*
 * Check if a block is available
 */
isblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{
	unsigned char mask;

	switch ((int)fs->fs_frag) {
	case 8:
		return (cp[h] == 0xff);
	case 4:
		mask = 0x0f << ((h & 0x1) << 2);
		return ((cp[h >> 1] & mask) == mask);
	case 2:
		mask = 0x03 << ((h & 0x3) << 1);
		return ((cp[h >> 2] & mask) == mask);
	case 1:
		mask = 0x01 << (h & 0x7);
		return ((cp[h >> 3] & mask) == mask);
	default:
		panic("isblock");
		return (NULL);
	}
}

/*
 * Take a block out of the map
 */
clrblock(fs, cp, h)
	struct fs *fs;
	u_char *cp;
	daddr_t h;
{

	switch ((int)fs->fs_frag) {
	case 8:
		cp[h] = 0;
		return;
	case 4:
		cp[h >> 1] &= ~(0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] &= ~(0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] &= ~(0x01 << (h & 0x7));
		return;
	default:
		panic("clrblock");
	}
}

/*
 * Put a block into the map
 */
setblock(fs, cp, h)
	struct fs *fs;
	unsigned char *cp;
	daddr_t h;
{

	switch ((int)fs->fs_frag) {

	case 8:
		cp[h] = 0xff;
		return;
	case 4:
		cp[h >> 1] |= (0x0f << ((h & 0x1) << 2));
		return;
	case 2:
		cp[h >> 2] |= (0x03 << ((h & 0x3) << 1));
		return;
	case 1:
		cp[h >> 3] |= (0x01 << (h & 0x7));
		return;
	default:
		panic("setblock");
	}
}

#if !(defined(vax) || defined(sun)) || defined(VAX630)
/*
 * C definitions of special vax instructions.
 */
scanc(size, cp, table, mask)
	u_int size;
	register u_char *cp, table[];
	register u_char mask;
{
	register u_char *end = &cp[size];

	while (cp < end && (table[*cp] & mask) == 0)
		cp++;
	return (end - cp);
}
#endif !(defined(vax) || defined(sun)) || defined(VAX630)

#if !defined(vax)

skpc(c, len, cp)
	register char c;
	register u_int len;
	register char *cp;
{

	if (len == 0)
		return (0);
	while (*cp++ == c && --len)
		;
	return (len);
}

#ifdef notdef
locc(c, len, cp)
	register char c;
	register u_int len;
	register char *cp;
{

	if (len == 0)
		return (0);
	while (*cp++ != c && --len)
		;
	return (len);
}
#endif notdef

#endif !defined(vax)

#ifdef KERNEL
/*
 * ufs_checkclean
 */
ufs_checkclean(mp)
	register struct mount	*mp;
{
	register struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	register struct buf	*bp;

	/*
	 * check for device errors
	 */
	if (ufs_deverrs)
		ufs_check_deverr(mp, fs);

	/*
	 * ignore if already stable
	 */
	if (fs->fs_clean == FSSTABLE)
		return;

	/*
	 * ignore if clean flag processing is disabled or suspended
	 */
	if ((fs->fs_clean == FSSUSPEND) || (fs->fs_clean == FSBAD))
		return;

	/*
	 * ignore if modified or readonly
	 */
	if ((fs->fs_fmod) || (fs->fs_ronly))
		return;

	/*
	 * get superblock buffer
	 */
	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);

	/*
	 * ignore if clean flag processing is disabled or suspended
	 */
	if ((fs->fs_clean == FSSUSPEND) || (fs->fs_clean == FSBAD)) {
		brelse(bp);
		return;
	}

	/*
	 * ignore if already stable or modified
	 */
	if ((fs->fs_clean == FSSTABLE) || (fs->fs_fmod)) {
		brelse(bp);
		return;
	}

	/*
	 * ignore if busy
	 */
	if ((ufs_bcheck(mp, bp)) || (ufs_icheck(mp))) {
		brelse(bp);
		return;
	}

	/*
	 * set clean flag to appropriate value
	 */
	fs->fs_clean = FSSTABLE;

	/*
	 * write superblock
	 */
	ufs_sbwrite(mp, fs, bp);
}

/*
 * ufs_bcheck
 */
ufs_bcheck(mp, sbp)
	register struct mount	*mp;
	register struct buf	*sbp;
{
	register struct vnode	*vp	= mp->m_devvp;
	register struct buf	*bp;
	register struct bufhd	*dp;
	register int		 s;

	/*
	 * check for busy bufs for this filesystem
	 */
	for (dp = bufhash; dp < &bufhash[BUFHSZ]; ++dp) {
		s = splbio();
		for (bp = dp->b_forw; bp != (struct buf *)dp; bp = bp->b_forw) {
			/*
			 * if buf is busy or dirty, then filesystem is busy
			 */
			if ((bp->b_vp == vp) &&
			    ((bp->b_flags & B_INVAL) == 0) &&
			    (bp->b_flags & (B_DELWRI|B_BUSY)) &&
			    (bp != sbp)) {
				(void) splx(s);
				return (1);
			}
		}
		(void) splx(s);
	}
	return (0);
}

/*
 * ufs_icheck
 */
ufs_icheck(mp)
	register struct mount	*mp;
{
	register struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	register union  ihead	*ih;
	register struct  inode	*ip;

	/*
	 * Check for busy inodes for this filesystem
	 */
	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		for (ip = ih->ih_chain[0];
		    ((ip != NULL) && (ip != (struct inode *)ih));
		    ip = ip->i_forw)
		{
			/*
			 * if inode is busy/modified/deleted, filesystem is busy
			 */
			if ((ip->i_fs == fs) &&
			    ((ip->i_flag & (ILOCKED|IMOD|IUPD|ICHG)) ||
			    ((ip->i_nlink <= 0) && (ip->i_flag & IREF))))
			{
				return (1);
			}
		}

	}
	return (0);
}

/*
 * ufs_notclean
 */
ufs_notclean(mp, fs)
	register struct mount	*mp;
	register struct fs	*fs;
{
	register struct buf	*bp;

	LOCKFS_SET_MOD(UTOL(mp->m_ul));

	/*
	 * check for device errors
	 */
	if (ufs_deverrs)
		ufs_check_deverr(mp, fs);

	/*
	 * ignore if already active
	 */
	if (fs->fs_clean == FSACTIVE)
		return;

	/*
	 * ignore if clean flag processing is disabled or suspended
	 */
	if ((fs->fs_clean == FSSUSPEND) || (fs->fs_clean == FSBAD))
		return;

	/*
	 * ignore if readonly
	 */
	if (fs->fs_ronly)
		return;

	/*
	 * read superblock, set fs_clean to active, and write it
	 */
	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);

	if (fs->fs_clean == FSTOACTIVE)
		fs->fs_clean = FSACTIVE;

	if (fs->fs_clean == FSACTIVE)
		brelse(bp);
	else if ((fs->fs_clean == FSSUSPEND) || (fs->fs_clean == FSBAD))
		brelse(bp);
	else {
		fs->fs_clean = FSTOACTIVE;
		ufs_sbwrite(mp, fs, bp);
	}
}

/*
 * ufs_cdwrite
 */
ufs_cdwrite(bp, ip, fs)
	register struct buf	*bp;
	register struct inode	*ip;
	register struct fs	*fs;
{

	ufs_notclean(ITOM(ip), fs);
	fs->fs_fmod = 1;
	bdwrite(bp);
}

/*
 * ufs_checkmount
 */
ufs_checkmount(mp)
	register struct mount	 *mp;
{
	register struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	register struct buf	*bp;

	/*
	 * ignore if readonly
	 */
	if (fs->fs_ronly)
		return;

	/*
	 * allocate a device error struct for the device of this mount
	 */
	(void) allocbioerror(mp->m_dev, (caddr_t)0, ufs_callback_deverr);

	/*
	 * read superblock, set fs_clean appropriately, and write it
	 */
	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
	ufs_setclean(mp, fs, fs, bp);
}

/*
 * ufs_checkremount
 */
ufs_checkremount(mp, sbp)
	register struct mount	*mp;
	register struct buf	*sbp;
{
	register struct fs	*fs	= mp->m_bufp->b_un.b_fs;
	register struct fs	*sfs	= sbp->b_un.b_fs;

	if (fs->fs_ronly) {

		/*
		 * allocate a device error struct for the device of this mount
		 */
		(void) allocbioerror(mp->m_dev,
		    (caddr_t)0, ufs_callback_deverr);
		/*
		 * update the clean state and write the superblock
		 */
		fs->fs_ronly = 0;
		ufs_setclean(mp, sfs, fs, sbp);
	} else
		brelse(sbp);
}

/*
 * ufs_checkunmount
 */
ufs_checkunmount(mp)
	register struct mount	 *mp;
{
	register struct fs	*fs		= mp->m_bufp->b_un.b_fs;
	register struct buf	*bp;

	/*
	 * ignore if readonly
	 */
	if (fs->fs_ronly)
		return;
	/*
	 * ignore if clean flag processing is disabled or suspended
	 */
	if ((fs->fs_clean == FSSUSPEND) || (fs->fs_clean == FSBAD))
		goto out;
	/*
	 * try once to make it stable
	 */
	ufs_checkclean(mp);
	if (fs->fs_clean != FSSTABLE)
		goto out;
	/*
	 * read superblock, set fs_clean, and write superblock
	 */
	bp = getblk(mp->m_devvp, SBLOCK, (int)fs->fs_sbsize);
	if (fs->fs_clean == FSSTABLE) {
		fs->fs_clean = FSCLEAN;
		ufs_sbwrite(mp, fs, bp);
	} else
		brelse(bp);

	/*
	 * check for device errors
	 */
	if (ufs_deverrs)
		ufs_check_deverr(mp, fs);
out:
	/*
	 * release device error structure
	 */
	while (freebioerror(mp->m_dev, (caddr_t)0, ufs_callback_deverr) < 0)
		(void) ufs_get_deverr(mp);

}

/*
 * ufs_sbwrite
 */
ufs_sbwrite(mp, fs, bp)
	register struct mount	*mp;
	register struct fs	*fs;
	register struct buf	*bp;
{
	/*
	 * for ulockfs processing, limit the superblock writes
	 */
	if ((mp->m_ul->ul_sbowner) && (u.u_procp != mp->m_ul->ul_sbowner)) {
		brelse(bp);
		return;
	}

	/*
	 * update superblock timestamp and fs_clean checksum
	 */
	fs->fs_time = time.tv_sec;
	fs_set_state(fs, FSOKAY - fs->fs_time);
	/*
	 * copy superblock into buffer and restore compatibility
	 */
	bcopy((caddr_t)fs, bp->b_un.b_addr, (u_int)fs->fs_sbsize);
	if (fs->fs_postblformat == FS_42POSTBLFMT) {		/* XXX */
		bp->b_un.b_fs->fs_nrpos = -1;			/* XXX */
		bp->b_un.b_fs->fs_npsect = 0;			/* XXX */
		bp->b_un.b_fs->fs_interleave = 0;		/* XXX */
	}
	switch (bp->b_un.b_fs->fs_clean) {
	case FSCLEAN:
	case FSSTABLE:
	case FSACTIVE:
		break;
	default:
		bp->b_un.b_fs->fs_clean = FSACTIVE;
		break;
	}
	bp->b_un.b_fs->fs_fmod = 0;	/* fs_fmod must always be 0 */
	bwrite(bp);			/* update superblock */
}

/*
 * bwait
 */
bwait(vp)
	struct vnode *vp;
{
	struct bufhd	*hp;		/* hash anchor */
	daddr_t		*bnop;		/* array of daddr_t */
	daddr_t		*cbnop;		/* pointer into array */
	long		*sizep;		/* array of long */
	long		*csizep;	/* pointer into array */
	u_long		tbno;		/* total entires in array */

	/*
	 * for each buf hash
	 */
	for (hp = bufhash; hp < &bufhash[BUFHSZ]; hp++) {
		/*
		 * get an array of (blkno, size) for processing below
		 */
		bgetbno(vp, hp, &bnop, &sizep, &tbno);
		for (cbnop = bnop, csizep = sizep; *cbnop; ++cbnop, ++csizep) {
			/*
			 * wait for the buf to go idle and then release it
			 */
			brelse(getblk(vp, *cbnop, (int)*csizep));
		}
		/*
		 * free array's of (blkno, size)
		 */
		if (bnop != NULL) {
			kmem_free((caddr_t)bnop,
				(u_int)(tbno * sizeof (daddr_t)));
			kmem_free((caddr_t)sizep,
				(u_int)(tbno * sizeof (long)));
		}
	}
}


/*
 * bsinval
 */
bsinval(vp)
	struct vnode *vp;
{
	struct bufhd	*hp;		/* hash anchor */
	struct buf	*bp;		/* buf */
	daddr_t		*bnop;		/* array of daddr_t */
	daddr_t		*cbnop;		/* pointer into array */
	long		*sizep;		/* array of long */
	long		*csizep;	/* pointer into array */
	u_long		tbno;		/* total entires in array */

	/*
	 * for each buf hash
	 */
	for (hp = bufhash; hp < &bufhash[BUFHSZ]; hp++) {
		/*
		 * get an array of (blkno, size) for processing below
		 */
		bgetbno(vp, hp, &bnop, &sizep, &tbno);
		for (cbnop = bnop, csizep = sizep; *cbnop; ++cbnop, ++csizep) {
loop:
			/*
			 * synchronously push and then invalidate
			 */
			bp = getblk(vp, *cbnop, (int)*csizep);
			while (bp->b_flags & B_DELWRI) {
				bwrite(bp);
				goto loop;
			}
			bp->b_flags |= B_INVAL;
			brelse(bp);
		}
		/*
		 * free array's of (blkno, size)
		 */
		if (bnop != NULL) {
			kmem_free((caddr_t)bnop,
				(u_int)(tbno * sizeof (daddr_t)));
			kmem_free((caddr_t)sizep,
				(u_int)(tbno * sizeof (long)));
		}
		/*
		 * release vnode associations
		 */
again:
		for (bp = hp->b_forw; bp != (struct buf *)hp; bp = bp->b_forw)
			if ((bp->b_vp == vp) && (bp->b_flags & B_INVAL)) {
				brelvp(bp);
				goto again;
			}

	}
}

/*
 * bgetbno
 *	return array of (blkno, size) for given buf hash
 */
bgetbno(vp, dp, bnopp, sizepp, tbnop)
	struct vnode	*vp;
	struct bufhd	*dp;
	daddr_t		**bnopp;
	long		**sizepp;
	u_long		*tbnop;
{
	struct buf	*bp;
	struct buf	*abp	= (struct buf *)dp;
	daddr_t		*bnop	= NULL;
	int		tbno	= 16;
	int		nbno;
	long		*sizep	= NULL;

	/*
	 * allocate an array of disk addr (null terminated)
	 */
again:
	if (bnop) {
		kmem_free((caddr_t)bnop, (u_int)(tbno * sizeof (daddr_t)));
		kmem_free((caddr_t)sizep, (u_int)(tbno * sizeof (long)));
	}
	tbno <<= 1;
	bnop  = (daddr_t *)kmem_zalloc((u_int)(tbno * sizeof (daddr_t)));
	sizep = (daddr_t *)kmem_zalloc((u_int)(tbno * sizeof (long)));

	/*
	 * fill in the array from the bufs for vp on hash dp
	 */
	for (bp = abp->b_forw, nbno = 0; bp && bp != abp; bp = bp->b_forw) {
		if (vp && bp->b_vp != vp) continue;
		if (nbno == (tbno-1))
			goto again;
		*(bnop  + nbno) = bp->b_blkno;
		*(sizep + nbno) = bp->b_bcount;
		nbno++;
	}

	/*
	 * return the array
	 */
	*bnopp  = bnop;
	*sizepp = sizep;
	*tbnop  = tbno;
}

/*
 * ufs_getsummaryinfo(mp, fs)
 */
ufs_getsummaryinfo(mp, fs)
	struct mount	*mp;
	struct fs	*fs;
{
	int		i;		/* `for' loop counter */
	int		size;		/* bytes of summary info to read */
	daddr_t		frags;		/* frags of summary info to read */
	caddr_t		sip;		/* summary info */
	struct buf	*tp;		/* tmp buf */

	/*
	 * Compute #frags and allocate space for summary info
	 */
	frags = howmany(fs->fs_cssize, fs->fs_fsize);
	sip = new_kmem_alloc((u_int)fs->fs_cssize, KMEM_SLEEP);

	/*
	 * Read summary info a fs block at a time and fill in array of pointers
	 */
	for (i = 0; i < frags; i += fs->fs_frag) {
		size = fs->fs_bsize;
		if (i + fs->fs_frag > frags)
			size = (frags - i) * fs->fs_fsize;
		tp = bread(mp->m_devvp, fsbtodb(fs, fs->fs_csaddr+i), size);
		if (tp->b_flags & B_ERROR) {
			kmem_free(sip, (u_int)fs->fs_cssize);
			brelse(tp);
			return (EIO);
		}
		bcopy((caddr_t)tp->b_un.b_addr, sip, (u_int)size);
		fs->fs_csp[fragstoblks(fs, i)] = (struct csum *)sip;
		sip += size;
		brelse(tp);
	}
	return (0);
}
/*
 * ufs_setclean
 */
ufs_setclean(mp, cfs, sfs, bp)
	struct mount	*mp;
	struct fs	*cfs;	/* check state in this fs */
	struct fs	*sfs;	/* set   state in this fs */
	struct buf	*bp;	/* buf to be updated sfs */
{
	/*
	 * set the clean state in sfs based on state in cfs
	 */
	if (FSOKAY != (fs_get_state(cfs) + cfs->fs_time))
		sfs->fs_clean = FSBAD;
	else
		switch (cfs->fs_clean) {
		case FSCLEAN:
		case FSSTABLE:
			if (mp->m_dio & MDIO_ON)
				sfs->fs_clean = FSSUSPEND;
			else
				sfs->fs_clean = FSSTABLE;
			break;
		case FSACTIVE:
		default:
			sfs->fs_clean = FSBAD;
			break;
		}
	ufs_sbwrite(mp, sfs, bp);
}

#endif KERNEL
