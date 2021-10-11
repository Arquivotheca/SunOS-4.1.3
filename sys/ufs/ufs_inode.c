#ident  "@(#)ufs_inode.c 1.1 92/07/30 SMI" /* from UCB 7.1 6/5/86 */

/*LINTLIBRARY*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1988, 1990 by Sun Microsystem, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/trace.h>
#include <sys/dnlc.h>
#include <sys/vaccess.h>	/* ULOCKFS */
#include <sys/lockfs.h>		/* ULOCKFS */

#include <ufs/mount.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#ifdef	QUOTA
#include <ufs/quota.h>
#endif
#include <ufs/lockfs.h>		/* ULOCKFS */
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/pvn.h>
#include <vm/seg.h>
#include <vm/swap.h>
#include <vm/page.h>

extern int freemem, lotsfree, pages_before_pager;

struct	instats ins;

int ino_new;			/* Current # of inodes kmem_allocated */
int ino_free_at_front;		/* # inodes freed at front of free list */

struct	inode *ifreeh, **ifreet;

union ihead ihead[INOHSZ];

/*
 * Variables for maintaining the free list of inode structures.
 */
static struct inode *in_free;

#if !(PAGESIZE - 4096)
static u_int nin_incr = (2*PAGESIZE / (sizeof(struct inode)));
#else
static u_int nin_incr = (PAGESIZE / (sizeof(struct inode)));
#endif

/*
 * Convert inode formats to vnode types
 */
enum	vtype iftovt_tab[] = {
	VFIFO, VCHR, VDIR, VBLK, VREG, VLNK, VSOCK, VBAD
};

int vttoif_tab[] = {
	0, IFREG, IFDIR, IFBLK, IFCHR, IFLNK, IFSOCK, IFMT, IFIFO
};

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
ihinit()
{
	register int i;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = NULL;
	ifreet = NULL;
}

zeroperms()
{
}

#if defined(DEBUG_INODES)
printinode(s, i)
	char *s;
	struct inode *i;
{
    printf("%s inode %d/%d:%d m=%o u=%d g=%d\n",
	   s, major(i->i_dev), minor(i->i_dev), i->i_number,
	   i->i_ic.ic_mode, i->i_ic.ic_uid, i->i_ic.ic_gid);
    if (i->i_ic.ic_mode == 0) {
	printf("strange, perm of zero .. complete contents:\n");

	printf("\tchain: %x %x\n", i->i_chain[0], i->i_chain[1]);
	printf("\tvnode: %x devvp: %x\n", i->i_vnode, i->i_devvp);
	printf("\tflag: %x dev: %d/%d number: %x\n", i->i_flag, major(i->i_dev), minor(i->i_dev), i->i_number);
	printf("\tdiroff: %x fs: %x\n", i->i_diroff, i->i_fs);
	printf("\tdquot: %x owner: %x count: %x\n", i->i_dquot, i->i_owner, i->i_count);
	printf("\tfreef: %x freeb: %x\n", i->i_fr.if_freef, i->i_fr.if_freeb);

	printf("\tmode: %x\tnlink: %x\n", i->i_ic.ic_mode, i->i_ic.ic_nlink);
	printf("\tuid: %x\tgid: %x\n", i->i_ic.ic_uid, i->i_ic.ic_gid);
	printf("\tsize: %x\n", i->i_ic.ic_size);
	printf("\tatime: %x\tmtime: %x\tctime: %x\n", i->i_ic.ic_atime.tv_sec, i->i_ic.ic_mtime.tv_sec, i->i_ic.ic_ctime.tv_sec);
	printf("\tflags: %x\tblocks: %x\tgen: %x\n", i->i_ic.ic_flags, i->i_ic.ic_blocks, i->i_ic.ic_gen);
	zeroperms();
    }
}
#endif

/*
 * Look up an inode by device, inumber.  If it is in core (in the
 * inode structure), honor the locking protocol.  If it is not in
 * core, read it in from the specified device after freeing any pages.
 * In all cases, a pointer to a locked inode structure is returned.
 */
iget(dev, fs, ino, ipp)
	dev_t dev;
	register struct fs *fs;
	ino_t ino;
	struct inode **ipp;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct buf *bp;
	register struct dinode *dp;
	register struct inode *iq;
	struct mount *mp;
	int inode_dnlc_purge();

	/*
	 * Lookup inode in cache.
	 */
loop:
	mp = getmp(dev);
	if (mp == NULL)
		return (ENOENT);
	if (mp->m_bufp->b_un.b_fs != fs)
		return (ENOENT);
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw) {
		if (ino == ip->i_number && dev == ip->i_dev) {
			/*
			 * Found it - check for locks.
			 */
			if (((ip->i_flag & ILOCKED) != 0) &&
			    ip->i_owner != uniqpid()) {
				ip->i_flag |= IWANT;
				(void) sleep((caddr_t)ip, PINOD);
				goto loop;
			}
			/*
			 * If inode is on free list, remove it.
			 */
			if ((ip->i_flag & IREF) == 0) {
				if (iq = ip->i_freef)
					iq->i_freeb = ip->i_freeb;
				else
					ifreet = ip->i_freeb;
				*ip->i_freeb = iq;
				ip->i_freef = NULL;
				ip->i_freeb = NULL;
			}
			/*
			 * Lock the inode and mark it referenced and return it.
			 */
			ip->i_flag |= IREF;
			ilock(ip);
			VN_HOLD(ITOV(ip));
			*ipp = ip;
			ins.in_hits++;
			trace6(TR_UFS_INSTATS, ip, ip->i_dev, ip->i_number,
				TRC_INSTATS_HIT, ins.in_misses, ins.in_hits);
#if defined(DEBUG_INODES)
			printinode("iget: cached", ip);
#endif
			return (0);
		}
	}

	/*
	 * Inode was not in cache.
	 */

	/*
	 * If over high-water mark, and no inodes available on freelist
	 * without attached pages, try to free one up from dnlc.
	 */
	if (ino_new >= ninode)  {
		while (ifreeh == NULL || ITOV(ifreeh)->v_pages) {
			if (dnlc_iter(inode_dnlc_purge, 0) == NULL)
				break;
		}
	}

	/*
	 * If there's a free one available and it has no pages attached
	 * take it. If we're over the high water mark, take it even if
	 * it has attached pages. Otherwise, make a new one.
	 */
	if (ifreeh && (ITOV(ifreeh)->v_pages == NULL || ino_new >= ninode))  {
		ip = ifreeh;
		if (iq = ip->i_freef)
			iq->i_freeb = &ifreeh;
		ifreeh = iq;
		ip->i_freef = NULL;
		ip->i_freeb = NULL;
		/*
		 * When the inode was put on the free list in iinactive,
		 * we did an async syncip() there.  Here we call syncip()
		 * to synchronously wait for any pages that are still
		 * in transit and to invalidate all the pages on the vp
		 * and finally to write back the inode to disk.
		 */
		if (ip->i_flag & IFASTSYMLNK) {
			/* clean up the symbolic link cache content first */
			int i;
			for (i = 1; i < NDADDR && ip->i_db[i]; i++)
				ip->i_db[i] = 0;
			for (i = 0; i < NIADDR && ip->i_ib[i]; i++)
				ip->i_ib[i]=0;
		}
		ILOCK(ip);
		ip->i_flag = (ip->i_flag & (IMODTIME|ILOCKED|IWANT)) | IREF;
#ifdef	TRACE
		trace_vn_reuse(ITOV(ip));
#endif	TRACE
		if (ITOV(ip)->v_pages == NULL) {
			if (ino_free_at_front > 0)
				ino_free_at_front--;
		}
		if (syncip(ip, B_INVAL, 1) != 0 || (ip->i_flag & IWANT) != 0) {
			VN_HOLD(ITOV(ip));
			idrop(ip);
			goto loop;
		}
		ip->i_flag &= ~IMODTIME;
	} else {
		ip = (struct inode *)new_kmem_fast_zalloc((caddr_t *)
			&in_free, sizeof(*in_free), (int)nin_incr, KMEM_SLEEP);
		ip->i_forw = ip;
		ip->i_back = ip;
		ip->i_vnode.v_data = (caddr_t)ip;
		ip->i_vnode.v_op = &ufs_vnodeops;
		ip->i_flag = IREF;
		ILOCK(ip);
		ins.in_malloc++;
		ino_new++;
		ins.in_maxsize = MAX(ins.in_maxsize, ino_new);
	}

	/*
	 * We have to check the inode table again to make sure no
	 * inode with the same (dev, ino) has been created since last
	 * time we checked the inode table. This case can happen
	 * because there are a few places where we can go to sleep.
	 * If another process comes by also trying to iget the same
	 * (dev, ino) while we are sleeping, failing to find one, it
	 * will create an in-core inode with the same (dev, ino). Then
	 * we'll end up with two in-core inodes representing the same
	 * disk inode. This bug has caused a lot of panics due to file
	 * system corruption of various kinds.
	 */
	for (iq = ih->ih_chain[0]; iq != (struct inode *)ih; iq = iq->i_forw) {
		if (ino == iq->i_number && dev == iq->i_dev) {
			VN_HOLD(ITOV(ip));
			idrop(ip);
			goto loop;
		}
	}

	if (ITOV(ip)->v_count != 0)
		panic("free inode isn't");

	/*
	 * Move the inode on the chain for its new (ino, dev) pair
	 */
	remque(ip);
	insque(ip, ih);
	ip->i_dev = dev;
	ip->i_devvp = mp->m_devvp;
	ip->i_number = ino;
	trace3(TR_MP_INODE, ITOV(ip), dev, ino);
	ip->i_diroff = 0;
	ip->i_fs = fs;
	ip->i_nextr = 0;
#ifdef	QUOTA
	dqrele(ip->i_dquot);
	ip->i_dquot = NULL;
#endif
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, itod(fs, ino)),
	    (int)fs->fs_bsize);
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags & B_ERROR) != 0) {
		brelse(bp);
		/*
		 * The inode doesn't contain anything useful, so it
		 * would be misleading to leave it on its hash chain.
		 */
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * We also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		iunlock(ip);
		ip->i_flag = 0;
		/*
		 * Put the inode on the end of the free list.
		 * Maybe we should put it on the beginning of
		 * the free list.
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
#if defined(DEBUG_INODES)
		printf("iget: I/O error reading inode %d/%d:%d\n",
		       major(dev), minor(dev), ino);
#endif
		return (EIO);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;			/* structure assignment */
	/*
	 * These are unneeded when we go to the next major release and
	 * get these fields out of icommon.
	 */
	ip->i_ic.ic_delayoff = 0;		/* XXX */
	ip->i_ic.ic_delaylen = 0;		/* XXX */
	ip->i_ic.ic_nextrio = 0;		/* XXX */
	ip->i_ic.ic_writes = 0;			/* XXX */
	VN_INIT(ITOV(ip), mp->m_vfsp, IFTOVT(ip->i_mode), ip->i_rdev);
	if (ino == (ino_t)ROOTINO) {
		ITOV(ip)->v_flag |= VROOT;
	}
	brelse(bp);
#ifdef	QUOTA
	if (ip->i_mode != 0)
		ip->i_dquot = getinoquota(ip);
#endif
	*ipp = ip;
	ins.in_misses++;
	trace6(TR_UFS_INSTATS, ip, ip->i_dev, ip->i_number, TRC_INSTATS_MISS,
		ins.in_misses, ins.in_hits);
#if defined(DEBUG_INODES)
	printinode("iget: read", ip);
#endif
	return (0);
}

/*
 * Unlock inode and vrele associated vnode
 */
iput(ip)
	register struct inode *ip;
{

	if ((ip->i_flag & ILOCKED) == 0)
		panic("iput");
	iunlock(ip);
	ITIMES(ip);
	VN_RELE(ITOV(ip));
}

/*
 * Check that inode is not locked and release associated vnode.
 */
irele(ip)
	register struct inode *ip;
{

	if (ip->i_flag & ILOCKED)
		panic("irele");
	ITIMES(ip);
	VN_RELE(ITOV(ip));
}

/*
 * Drop inode without going through the normal
 * chain of unlocking and releasing.
 */
idrop(ip)
	register struct inode *ip;
{
	register struct vnode *vp = &ip->i_vnode;

	if ((ip->i_flag & ILOCKED) == 0)
		panic("idrop");
	iunlock(ip);
	if (--vp->v_count == 0) {
		/* retain the fast symlnk flag and mtime-okay flag */
		ip->i_flag &= (IFASTSYMLNK|IMODTIME);
		/*
		 * Put the inode back on the end of the free list.
		 */
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
	}
}

/*
 * Vnode is no longer referenced, write the inode out
 * and if necessary, truncate and deallocate the file.
 */
iinactive(ip)
	register struct inode *ip;
{
	int mode;

	if ((ip->i_flag & (IREF|ILOCKED)) != IREF || ip->i_freeb || ip->i_freef)
		panic("iinactive");
	if (ip->i_fs && ip->i_mode && ip->i_fs->fs_ronly == 0) {
		ilock(ip);
		if (ip->i_nlink <= 0) {
			if (ULOCKFS_IS_NOIDEL(ITOU(ip))) {
				iunlock(ip);
				return;
			}
			ip->i_gen++;
			(void) itrunc(ip, (u_long)0);
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			ip->i_flag |= IUPD|ICHG;
			ifree(ip, ip->i_number, mode);
#ifdef	QUOTA
			(void) chkiq(VFSTOM(ip->i_vnode.v_vfsp),
			    ip, (int)ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
#endif
			IUPDAT(ip, 0)
		} else if (!IS_SWAPVP(ITOV(ip))) {
			/*
			 * Do an async write (B_ASYNC) of the pages
			 * on the vnode and put the pages on the free
			 * list when we are done (B_FREE).  This action
			 * will cause all the pages to be written back
			 * for the file now and will allow update() to
			 * skip over inodes that are on the free list.
			 *
			 * NOTE:  The pages associated with this vnode are
			 * freed only if the system is low on memory.
			 */
			if (freemem < lotsfree + pages_before_pager)
				(void) syncip(ip, B_FREE | B_ASYNC, 0);
			else {
				IUPDAT(ip, 0);
			}
				
		} else {
			IUPDAT(ip, 0);
		}
		iunlock(ip);
	}

	/* retain the fast symlnk flag */
	ip->i_flag &= (IFASTSYMLNK|IMODTIME);
	ITOV(ip)->v_op = &ufs_vnodeops;
	/*
	 * If the inode has associated pages put it on the back of the
	 * free list. If it has none, put it on the front.
	 * Also, fast symbolic inode always put at end of free list
	 */
	if (ITOV(ip)->v_pages || ip->i_flag & IFASTSYMLNK) {
		if (ifreeh) {
			*ifreet = ip;
			ip->i_freeb = ifreet;
		} else {
			ifreeh = ip;
			ip->i_freeb = &ifreeh;
		}
		ip->i_freef = NULL;
		ifreet = &ip->i_freef;
		ins.in_frback++;
	} else {
		if (ifreeh) {
			ip->i_freef = ifreeh;
			ifreeh->i_freeb = &ip->i_freef;
		} else {
			ip->i_freef = NULL;
			ifreet = &ip->i_freef;
		}
		ifreeh = ip;
		ip->i_freeb = &ifreeh;
		ino_free_at_front++;
		ins.in_frfront++;
	}
}

/*
 * Check accessed and update flags on an inode structure.
 * If any is on, update the inode with the (unique) current time.
 * If waitfor is given insure i/o order so wait for write to complete.
 */
iupdat(ip, waitfor)
	register struct inode *ip;
	int waitfor;
{
	register struct buf *bp;
	register struct fs *fp;
	struct dinode *dp;
	int fastsymflag;		/* fast symbolic link is active */
	long fastsymlnk[FSL_SIZE / sizeof (long)]; /* for sun4 alignment */

	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG|IMOD|IMODACC)) != 0) {
		if (fp == NULL || fp->fs_ronly)
			return;
		ufs_notclean(ITOM(ip), fp);
		fastsymflag = (ip->i_flag & IFASTSYMLNK);
		if (fastsymflag) {
			/* save fast sym link */
			int i;

			(void) bcopy((caddr_t) &ip->i_db[1],
				(caddr_t) fastsymlnk, (u_int)ip->i_size);
			ip->i_flag &= ~IFASTSYMLNK;
			for (i = 1; i < NDADDR && ip->i_db[i]; i++)
				ip->i_db[i] = 0;
			for (i = 0; i < NIADDR && ip->i_ib[i]; i++)
				ip->i_ib[i] = 0;
		}
		bp = bread(ip->i_devvp,
		    (daddr_t)fsbtodb(fp, itod(fp, ip->i_number)),
		    (int)fp->fs_bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		if (ip->i_flag & (IUPD|IACC|ICHG))
			IMARK(ip);
		ip->i_flag &= ~(IUPD|IACC|ICHG|IMOD|IMODACC);
		dp = bp->b_un.b_dino + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;	/* structure assignment */
		/*
		 * These are unneeded when we go to the next major release and
		 * get these fields out of icoomon.
		 */
		dp->di_un.di_icom.ic_delayoff = 0;	/* XXX */
		dp->di_un.di_icom.ic_delaylen = 0;	/* XXX */
		dp->di_un.di_icom.ic_nextrio = 0;	/* XXX */
		dp->di_un.di_icom.ic_writes = 0;		/* XXX */
		if (waitfor && ((ITOM(ip)->m_dio & MDIO_ON) == 0))
			bwrite(bp);
		else
			bdwrite(bp);

		/* restore inode */
		if (fastsymflag) {
			bcopy((caddr_t) fastsymlnk, (caddr_t) &ip->i_db[1],
				(u_int)ip->i_size);
			ip->i_flag |= IFASTSYMLNK;
		}
	}
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */

/*
 * Release blocks associated with the inode ip and
 * stored in the indirect block bn.  Blocks are free'd
 * in LIFO order up to (but not including) lastbn.  If
 * level is greater than SINGLE, the block is an indirect
 * block and recursive calls to indirtrunc must be used to
 * cleanse other indirect blocks.
 *
 * N.B.: triple indirect blocks are untested.
 */
static long
indirtrunc(ip, bn, lastbn, level)
	register struct inode *ip;
	daddr_t bn, lastbn;
	int level;
{
	register int i;
	struct buf *bp, *copy;
	register daddr_t *bap;
	register struct fs *fs = ip->i_fs;
	daddr_t nb, last;
	long factor;
	int blocksreleased = 0, nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++)
		factor *= NINDIR(fs);
	last = lastbn;
	if (lastbn > 0)
		last /= factor;
	nblocks = btodb(fs->fs_bsize);
	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = geteblk((int)fs->fs_bsize);
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
		(u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy, bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0)
			continue;
		if (level > SINGLE)
			blocksreleased +=
			    indirtrunc(ip, nb, (daddr_t)-1, level - 1);
		free(ip, nb, (off_t)fs->fs_bsize);
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0)
			blocksreleased += indirtrunc(ip, nb, last, level - 1);
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * Truncate the inode ip to at most length size.
 * Free affected disk blocks -- the blocks of the
 * file are removed in reverse order.
 *
 * N.B.: triple indirect blocks are untested.
 */
itrunc(oip, length)
	register struct inode *oip;
	u_long length;
{
	register struct fs *fs = oip->i_fs;
	register struct inode *ip;
	register daddr_t lastblock;
	register off_t bsize;
	register int offset;
	daddr_t bn, lastiblock[NIADDR];
	int level;
	long nblocks, blocksreleased = 0;
	register int i;
	struct inode tip;
	daddr_t llbn;

	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here.  In addition, we allow symbolic
	 * links to be truncated only to zero length.  Other inode
	 * types cannot have their length set here since disk blocks
	 * are being dealt with - especially device inodes where
	 * ip->i_rdev is actually being stored in ip->i_db[0]!
	 */
	i = oip->i_mode & IFMT;
	if (i != IFREG && i != IFDIR && i != IFLNK)
		return (0);
	else if (i == IFLNK && length != 0)
		return (EINVAL);
/*
 * POSIX requires modification time be updated on truncation
 */
	if (length == oip->i_size) {
		oip->i_flag |= ICHG|IUPD;
		return (0);
	}

	if (oip->i_flag & IFASTSYMLNK) {
		int j;
		oip->i_flag &= ~IFASTSYMLNK;
		for (j = 1; j < NDADDR && oip->i_db[j]; j++)
			oip->i_db[j] = 0;
		for (j = 0; j < NIADDR && oip->i_ib[j]; j++)
			oip->i_ib[j] = 0;
	}

	offset = blkoff(fs, length);
	llbn = lblkno(fs, length - 1);
	if (length > oip->i_size) {
		int err;

		/*
		 * Trunc up case.  We need to call bmap_write() because of
		 * frags since the number of frags is calculated from the size.
		 */
		if (offset == 0)
			err = BMAPALLOC(oip, llbn, (int)fs->fs_bsize);
		else
			err = BMAPALLOC(oip, llbn, offset);
		if (err == 0) {
			oip->i_size = length;
			oip->i_flag |= ICHG;
			ITIMES(oip);
		}
		return (err);
	}

	/*
	 * Forget about delayed pages that will be past the end of the file.
	 */
	if (oip->i_delaylen && oip->i_delayoff + oip->i_delaylen > length) {
		oip->i_delaylen -= oip->i_delayoff + oip->i_delaylen - length;
		if (oip->i_delaylen <= 0)
			oip->i_delaylen = oip->i_delayoff = 0;
	}

	/*
	 * Update the pages of the file.  If the file is not being
	 * truncated to a block boundary, the contents of the
	 * pages following the end of the file must be zero'ed
	 * in case it ever become accessable again because
	 * of subsequent file growth.
	 */
	trace3(TR_MP_TRUNC, ITOV(oip), length, oip->i_size);
	if (offset == 0) {
		pvn_vptrunc(ITOV(oip), (u_int)length, (u_int)0);
	} else {
		int err;

		/*
		 * Trunc down case.
		 * Make sure that the last block is properly allocated.
		 * We only really have to do this if the last block is
		 * actually allocated since bmap will now handle the case
		 * of a fragment which is has no block allocated.  Just to
		 * be sure, we do it now independent of current allocation.
		 */
		if (err = BMAPALLOC(oip, llbn, offset))
			return (err);

		/*
		 * Caclulate how much to zero past EOF.
		 */
		bsize = llbn >= NDADDR? fs->fs_bsize : fragroundup(fs, offset);
		pvn_vptrunc(ITOV(oip), (u_int)length, (u_int)(bsize - offset));
	}

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);

	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to indirtrunc below.
	 */
	tip = *oip;			/* structure copy */
	ip = &tip;

	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	for (i = NDADDR - 1; i > lastblock; i--)
		oip->i_db[i] = 0;

	oip->i_size = length;
	oip->i_flag |= ICHG|IUPD;
	iupdat(oip, 1);			/* do sync inode update */

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    indirtrunc(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
				ip->i_ib[level] = 0;
				free(ip, bn, (off_t)fs->fs_bsize);
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		ip->i_db[i] = 0;
		bsize = (off_t)blksize(fs, ip, i);
		free(ip, bn, bsize);
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0)
		goto done;

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		off_t oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip, lastblock);
		if (newspace == 0)
			panic("itrunc: newspace");
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			free(ip, bn, oldspace - newspace);
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
/* BEGIN PARANOIA */
	for (level = SINGLE; level <= TRIPLE; level++)
		if (ip->i_ib[level] != oip->i_ib[level])
			panic("itrunc1");
	for (i = 0; i < NDADDR; i++)
		if (ip->i_db[i] != oip->i_db[i])
			panic("itrunc2");
/* END PARANOIA */
	oip->i_blocks -= blocksreleased;
	if (length == 0 && oip->i_blocks != 0) {	/* sanity */
		printf("itrunc: %s/%d new size = %d, blocks = %d\n",
		    fs->fs_fsmnt, oip->i_number, oip->i_size, oip->i_blocks);
		oip->i_blocks = 0;
	}
	oip->i_flag |= ICHG;
#ifdef	QUOTA
	(void) chkdq(oip, -blocksreleased, 0);
#endif
	return (0);
}


/*
 * Check mode permission on inode.  Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the read-only status of the file system
 * is checked.  The mode is shifted to select the owner/group/other
 * fields.  The super user is granted all permissions except
 * writing to read-only file systems.
 */
iaccess(ip, m)
	register struct inode *ip;
	register int m;
{
	register int *gp;

	if (m & IWRITE) {
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system, or a fifo.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK &&
			    (ip->i_mode & IFMT) != IFIFO) {
				return (EROFS);
			}
		}
	}
	/*
	 * If you're the super-user,
	 * you always get access.
	 */
	if (u.u_uid == 0)
		return (0);
	/*
	 * Access check is based on only
	 * one of owner, group, public.
	 * If not owner, then check group.
	 * If not a member of the group, then
	 * check public access.
	 */
	if (u.u_uid != ip->i_uid) {
		m >>= 3;
		if (u.u_gid == ip->i_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if ((ip->i_mode & m) == m)
		return (0);
	return (EACCES);
}

ilock(ip)
	register struct inode *ip;
{

	ILOCK(ip);
}

iunlock(ip)
	register struct inode *ip;
{

	IUNLOCK(ip);
}

/*
 *	Check a dnlc entry to see if it's suitable for purging from the
 *	dnlc and if so purge it. Suitable is: must be type ufs,
 *	and if pages_ok == 0, have no associated pages. Returns 1 if
 *	purged an entry, else 0. This function is intended to be called by
 *	the dnlc_iter() function to search the cache and purge a qualified
 *	entry.
 */
static int
inode_dnlc_purge(ncp, pages_ok)
	register struct ncache *ncp;
	register int pages_ok;
{
	register struct vnode *vp = dnlc_vp(ncp);

	ins.in_dnlclook++;

	if ((vp->v_op == &ufs_vnodeops) &&
	    (pages_ok || (!pages_ok && !vp->v_pages))) {
		ins.in_dnlcpurge++;
		dnlc_rm(ncp);
		return (1);
	} else
		return (0);
}
/*
 * Remove any inodes in the inode cache belonging to dev
 *
 * If not forced unmount:
 * 	There should not be any active ones, return error if any are
 *	found but still invalidate others (N.B.: this is a user error,
 *	not a system error).
 * If forced unmount
 *	unhash relevant inodes, and NULL ip->i_fs and vp->v_vfsp because
 *	fs and vfs will be freed even while the inode remains.
 *	vp->v_ops is set to ufs_forcedops, which returns EIO to every
 *	access except close and inactive.
 */

extern struct vnodeops	ufs_forcedops;
extern struct vfsops	ufs_vfsops;

struct vfs		forcedvfs;

iflush(dev, forced, iq)
	dev_t dev;
	int forced;
	struct inode *iq;
{
	struct inode *ip;
	union  ihead *ih;

loop:
	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		for (ip = ih->ih_chain[0];
			ip != (struct inode *)ih; ip = ip->i_forw) {

			if (ip->i_dev != dev)
				continue;
			/*
			 * quota inode will be handled in a later iflush call
			 */
			if (ip == iq)
				continue;
			/*
			 * stop the flush if an inode is referenced
			 */
			if ((!forced) && (ip->i_flag & IREF))
				return (1);
			/*
			 * flush the inode
			 */
			if (iget(ip->i_dev, ip->i_fs, ip->i_number, &ip))
				return (1);
			(void) syncip(ip, B_INVAL, 1);

			/*
			 * unhash
			 */
			remque(ip);
			ip->i_forw = ip;
			ip->i_back = ip;
#ifdef	QUOTA
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
#endif
			/*
			 * if forced:
			 *	new vnodeops vector (returns EIO)
			 *	new vfs struct (prevent data fault panics)
			 *	NULL i_fs
			 *	call iinactive on hidden, deleted files
			 */
			if (forced) {
				forcedvfs.vfs_flag = VFS_RDONLY;
				forcedvfs.vfs_op = &ufs_vfsops;
				ITOV(ip)->v_op = &ufs_forcedops;
				ITOV(ip)->v_vfsp = &forcedvfs;
				ITOV(ip)->v_flag &= ~VROOT;
				ip->i_fs = NULL;
			}
			iput(ip);
			goto loop;
		}
	}
	return (0);
}

#ifdef	TRACE
/*
 * Dump the contents of the inode cache.
 */
trace_inode()
{
	register struct inode *ip;
	register union  ihead *ih;

	/*
	 * Purge the cache so we can find out who's REALLY in use
	 */
	dnlc_purge();

	for (ih = ihead; ih < &ihead[INOHSZ]; ih++) {
		for (ip = ih->ih_chain[0];
		    ip != (struct inode *)ih; ip = ip->i_forw) {
			trace6(TR_UFS_INODE, ip,
				ip->i_flag, ip->i_dev, ip->i_number,
				(ip->i_mode << 16) | (ip->i_rdev & 0xffff), 0);
		}
	}
	trace6(TR_UFS_INODE, 0, 0, 0, 0, 0, 1);  /* Signals last entry */
}

/*
 * Dump the inode statistics counters.
 */
trace_instats()
{
	trace6(TR_UFS_INSTATS, 0, 0, 0, 0, ins.in_misses, ins.in_hits);
}

/*
 * Reset the inode statistics counters.
 */
trace_instats_reset()
{
	bzero((caddr_t) &ins, sizeof (ins));
}
#endif	TRACE
