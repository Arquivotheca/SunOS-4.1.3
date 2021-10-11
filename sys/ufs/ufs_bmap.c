#ident  "@(#)ufs_bmap.c 1.1 92/07/30 SMI" /* from UCB 7.1 6/5/86 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1988, 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/debug.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <vm/seg.h>
#include <sys/trace.h>

/*
 * Find the extent, and the matching block number.
 */

#define	DOEXTENT(fs, lbn, boff, bnp, lenp, size, tblp, n, chkfrag) {\
	register daddr_t *dp = (tblp); \
	register int _chkfrag = chkfrag; /* for lint. sigh */ \
\
	if (*dp == 0) {\
		*(bnp) = UFS_HOLE; \
	} else {\
		register int len; \
\
		len = findextent(fs, dp, (int)(n)) << (fs)->fs_bshift; \
		if (_chkfrag) { \
			register unsigned int tmp; \
\
			tmp = fragroundup((fs), size) - \
			      ((lbn) << fs->fs_bshift); \
			len = MIN(tmp, len); \
		} \
		len -= (boff); \
		if (len <= 0) { \
			*(bnp) = UFS_HOLE; \
		} else {\
			*(bnp) = *dp; \
			*(lenp) = len; \
		}\
	}\
}

/*
 * The bmap routines define the structure of the file system storage by
 * mapping logical block numbers in a file to physical block numbers
 * on the device.
 *
 * The "boff" field is the offset into the lbn that the caller is trying
 * to map.  Bmap will return UFS_HOLE if a there is data at the beginning
 * of the lbn but a hole at lbn + boff.
 *
 * XXX - this code is not known to work for bsize < pagesize.
 */
int
bmap_read(ip, lbn, boff, bnp, lenp)
	register struct inode *ip;
	daddr_t lbn;
	daddr_t *bnp;
	int *lenp;
{
	register struct fs *fs;
	register struct buf *bp;
	register int i;
	int j, sh;
	daddr_t ob, nb, llbn, tbn, *bap;
	int bsize;

	if (lbn < 0)
		return (EFBIG);
	ASSERT(bnp != NULL);

	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	llbn = lblkno(fs, ip->i_size - 1);

	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (lbn < NDADDR) {
		nb = ip->i_db[lbn];
		DOEXTENT(fs, lbn, boff, bnp, lenp,
		    ip->i_size, &ip->i_db[lbn], NDADDR - lbn, 1);
		return (0);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	sh = 1;
	tbn = lbn - NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (tbn < sh)
			break;
		tbn -= sh;
	}

	if (j == 0) {
		return (EFBIG);
	}

	/*
	 * Fetch the first indirect block.
	 */
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		*bnp = UFS_HOLE;
		return (0);
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= NIADDR; j++) {
		ob = nb;
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, ob), bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (tbn / sh) % NINDIR(fs);
		nb = bap[i];
		brelse(bp);
		if (nb == 0) {
			*bnp = UFS_HOLE;
			return (0);
		}
	}
	/*
	 * No size is needed for the block case.
	 */
	DOEXTENT(fs, lbn, boff, bnp, lenp,
	    -1, &bap[i], MIN(NINDIR(fs) - i, llbn - lbn + 1), 0);
	return (0);
}

/*
 * bmap_wr - S_WRITE
 */
int
bmap_write(ip, lbn, boff, bnp, lenp, size, alloc_only)
	register struct inode *ip;
	daddr_t lbn, *bnp;
	int *lenp;
{
	register struct fs *fs;
	register struct buf *bp;
	register int i;
	struct buf *nbp;
	int j, sh;
	daddr_t ob, nb, pref, llbn, tbn, *bap;
	int bsize, osize, nsize, isize;
	int issync, isdir;
	int err;
	struct vnode *devvp;
	struct fbuf *fbp;

	if (lbn < 0)
		return (EFBIG);

	fs = ip->i_fs;
	ASSERT(size <= fs->fs_bsize);
	bsize = fs->fs_bsize;
	llbn = lblkno(fs, ip->i_size - 1);
	issync = (ip->i_flag & ISYNC) != 0;
	isdir = (ip->i_mode & IFMT) == IFDIR;
	if (isdir || issync)
		alloc_only = 0;		/* make sure */

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	if (llbn < NDADDR && llbn < lbn && (ob = ip->i_db[llbn]) != 0) {
		osize = blksize(fs, ip, llbn);
		if (osize < bsize && osize > 0) {
			/*
			 * Make sure we have all needed pages setup correctly.
			 * LMXXX - this is disgusting code.  It's calling
			 * the getpage routines past the end of the file
			 * because it is too lazy to alloc the pages here.
			 * It isn't a problem on bsize == PAGESIZE machines.
			 */
			err = fbread(ITOV(ip), (u_int)(llbn << fs->fs_bshift),
			    (u_int)bsize, S_OTHER, &fbp);
			if (err) {
				return (err);
			}

			pref = blkpref(ip, llbn, (int)llbn, &ip->i_db[0]);
			err = realloccg(ip, ob, pref, osize, bsize, &nb);
			if (err) {
				fbrelse(fbp, S_OTHER);
				return (err);
			}

			/*
			 * Don't check isdir here, directories won't do this
			 */
			if (issync)
				(void) fbiwrite(fbp, ip, nb);
			else
				fbrelse(fbp, S_WRITE);

			ip->i_size = (llbn + 1) << fs->fs_bshift;
			ip->i_db[llbn] = nb;
			ip->i_flag |= IUPD | ICHG;
			ip->i_blocks += btodb(bsize - osize);

			if (nb != ob)
				free(ip, ob, (off_t)osize);
		}
	}

	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (lbn < NDADDR) {
		nb = ip->i_db[lbn];
		isize = ip->i_size;
		/*
		 * Maybe it's already allocated in which case we
		 * have nothing to do.
		 */
		if (nb == 0 || ip->i_size < (lbn + 1) << fs->fs_bshift) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;

				/* need to allocate a block or frag */
				ob = nb;
				pref = blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = realloccg(ip, ob, pref, osize, nsize,
				    &nb);
				if (err) {
					return (err);
				}
			} else {
				/* need to allocate a block or frag */
				osize = 0;
				if (ip->i_size < (lbn + 1) << fs->fs_bshift)
					nsize = fragroundup(fs, size);
				else
					nsize = bsize;
				pref = blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = alloc(ip, pref, nsize, &nb);
				if (err) {
					return (err);
				}
				ob = nb;
			}

			/*
			 * Read old/create new zero pages
			 */
			fbp = NULL;
			if (osize == 0) {
				if (!alloc_only)
					fbzero(ITOV(ip),
					    (u_int)(lbn << fs->fs_bshift),
					    (u_int)nsize, &fbp);
			} else {
				err = fbread(ITOV(ip),
				    (u_int)(lbn << fs->fs_bshift),
				    (u_int)nsize, S_OTHER, &fbp);
				if (err) {
					if (nb != ob) {
						free(ip, nb, (off_t)nsize);
					} else {
						free(ip,
						    ob + numfrags(fs, osize),
						    (off_t)(nsize - osize));
					}
#ifdef	QUOTA
					(void) chkdq(ip,
					    -(long)btodb(nsize - osize), 0);
#endif	QUOTA
					return (err);
				}
			}

			if (isdir) {
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				(void) fbiwrite(fbp, ip, nb);
			} else {
				if (fbp != NULL)
					fbrelse(fbp, S_WRITE);
			}

			ip->i_db[lbn] = nb;
			ip->i_blocks += btodb(nsize - osize);
			isize += (nsize - osize);
			ip->i_flag |= IUPD | ICHG;

			if (nb != ob)
				free(ip, ob, (off_t)osize);
		}
gotit:
		if (bnp != NULL) {
			DOEXTENT(fs, lbn, boff, bnp, lenp,
			    isize, &ip->i_db[lbn], NDADDR - lbn, 1);
			ASSERT(*bnp != UFS_HOLE);
		}
		return (0);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	tbn = lbn - NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (tbn < sh)
			break;
		tbn -= sh;
	}

	if (j == 0) {
		return (EFBIG);
	}

	/*
	 * Fetch the first indirect block.
	 */
	devvp = ip->i_devvp;
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		/*
		 * Need to allocate an indirect block.
		 */
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
		err = alloc(ip, pref, bsize, &nb);
		if (err) {
			return (err);
		}
		/*
		 * Write zero block synchronously so that
		 * indirect blocks never point at garbage.
		 */
		bp = getblk(devvp, fsbtodb(fs, nb), bsize);
		clrbuf(bp);
		bwrite(bp);

		ip->i_ib[NIADDR - j] = nb;
		ip->i_blocks += btodb(bsize);
		ip->i_flag |= IUPD | ICHG;

		/*
		 * In the ISYNC case, rwip will notice that the block
		 * count on the inode has changed and will be sure to
		 * iupdat the inode at the end of rwip.
		 */
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= NIADDR; j++) {
		ob = nb;
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, ob), bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (tbn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (pref == 0) {
				if (j < NIADDR) {
					/* Indirect block */
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				} else {
					/* Data block */
					pref = blkpref(ip, lbn, i, &bap[0]);
				}
			}

			err = alloc(ip, pref, bsize, &nb);
			if (err) {
				brelse(bp);
				return (err);
			}

			if (j < NIADDR) {
				/*
				 * Write synchronously so indirect
				 * blocks never point at garbage.
				 */
				nbp = getblk(devvp, fsbtodb(fs, nb), bsize);
				clrbuf(nbp);
				bwrite(nbp);
			} else if (!alloc_only ||
			    roundup(size, PAGESIZE) < bsize) {
				/*
				 * To avoid deadlocking if the pageout
				 * daemon decides to push a page for this
				 * inode while we are sleeping holding the
				 * bp but waiting more pages for fbzero,
				 * we give up the bp now.
				 *
				 * XXX - need to avoid having the pageout
				 * daemon get in this situation to begin with!
				 */
				brelse(bp);
				fbzero(ITOV(ip),
				    (u_int)(lbn << fs->fs_bshift),
				    (u_int)bsize, &fbp);

				/*
				 * Cases which we need to do a synchronous
				 * write of the zeroed data pages:
				 *
				 * 1) If we are writing a directory then we
				 * want to write synchronously so blocks in
				 * directories never contain garbage.
				 *
				 * 2) If we are filling in a hole and the
				 * indirect block is going to be synchronously
				 * written back below we need to make sure
				 * that the zeroes are written here before
				 * the indirect block is updated so that if
				 * we crash before the real data is pushed
				 * we will not end up with random data is
				 * the middle of the file.
				 */
				if (isdir || (issync && lbn < llbn))
					(void) fbiwrite(fbp, ip, nb);
				else
					fbrelse(fbp, S_WRITE);

				/*
				 * Now get the bp back
				 */
				bp = bread(ip->i_devvp,
				    (daddr_t)fsbtodb(fs, ob), bsize);
				err = geterror(bp);
				if (err) {
					free(ip, nb, (off_t)bsize);
#ifdef	QUOTA
					(void) chkdq(ip, -(long)btodb(bsize),
					    0);
#endif	QUOTA
					brelse(bp);
					return (err);
				}
				bap = bp->b_un.b_daddr;
			}

			bap[i] = nb;
			ip->i_blocks += btodb(bsize);
			ip->i_flag |= IUPD | ICHG;

			if (issync)
				bwrite(bp);
			else
				bdwrite(bp);
		} else {
			brelse(bp);
		}
	}
	if (bnp != NULL) {
		/*
		 * No size is needed for the block case.
		 */
		DOEXTENT(fs, lbn, boff, bnp, lenp,
		    -1, &bap[i], MIN(NINDIR(fs) - i, llbn - lbn + 1), 0);
		ASSERT(*bnp != UFS_HOLE);
	}
	return (0);
}

/*
 * find some contig blocks starting at *sbp and going for min(n, max_contig)
 * return the number of blocks (not frags) found.
 * The array passed in must be at least [0..n-1].
 */
static int
findextent(fs, sbp, n)
	struct fs *fs;
	daddr_t *sbp;
	register int n;
{
	register daddr_t bn, nextbn;
	register daddr_t *bp;
	register int diff;

	if (n <= 0)
		return (0);
	bn = *sbp;
	if (bn == 0)
		return (0);
	diff = fs->fs_frag;
	n = MIN(n, fs->fs_maxcontig);
	bp = sbp;
	while (--n > 0) {
		nextbn = *(bp + 1);
		if (nextbn == 0 || bn + diff != nextbn)
			break;
		bn = nextbn;
		bp++;
	}
	return ((int)(bp - sbp) + 1);
}

#ifndef	REMOVE_OLD_UFS
/*
 * XXX - obsolete - only for 4K fs support.
 *
 * bmap defines the structure of the file system storage by mapping logical
 * block numbers in a file to physical block numbers on the device.  bmap
 * should be called with a locked inode when allocation is to be done.
 *
 * bmap translates logical block number lbn to a physical block and
 * returns it in *bnp, possibly along with a read-ahead block number in
 * *rabnp.  bnp and rabnp can be NULL if the information is not required.
 * rw specifies whether the mapping is for read or write.  If for write,
 * the block must be at least size bytes and will be extended or allocated
 * as needed.  If alloc_only is set, bmap may not create any in-core pages
 * that correspond to the new disk allocation.  Otherwise, the in-core
 * pages will be created and initialized as needed.
 */
int
bmap(ip, lbn, bnp, rabnp, size, rw, alloc_only)
	register struct inode *ip;
	daddr_t lbn;
	daddr_t *bnp, *rabnp;
	int size;
	enum seg_rw rw;
{
	register struct fs *fs;
	register struct buf *bp;
	register int i;
	struct buf *nbp;
	int j, sh;
	daddr_t ob, nb, pref, llbn, tbn, *bap;
	int bsize, osize, nsize;
	int issync, isdir;
	int err;
	struct vnode *devvp;
	struct fbuf *fbp;

	if (lbn < 0)
		return (EFBIG);

	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	llbn = lblkno(fs, ip->i_size - 1);
	issync = (ip->i_flag & ISYNC) != 0;
	isdir = (ip->i_mode & IFMT) == IFDIR;
	if (isdir || issync)
		alloc_only = 0;		/* make sure */

	/*
	 * If the next write will extend the file into a new block,
	 * and the file is currently composed of a fragment
	 * this fragment has to be extended to be a full block.
	 */
	if (rw == S_WRITE && llbn < NDADDR && llbn < lbn &&
	    (ob = ip->i_db[llbn]) != 0) {
		osize = blksize(fs, ip, llbn);
		if (osize < bsize && osize > 0) {
			/*
			 * Make sure we have all needed pages setup correctly.
			 */
			err = fbread(ITOV(ip), (u_int)(llbn << fs->fs_bshift),
			    (u_int)bsize, S_OTHER, &fbp);
			if (err)
				return (err);

			pref = blkpref(ip, llbn, (int)llbn, &ip->i_db[0]);
			err = realloccg(ip, ob, pref, osize, bsize, &nb);
			if (err) {
				fbrelse(fbp, S_OTHER);
				return (err);
			}

			/*
			 * Don't check isdir here, directories won't do this
			 */
			if (issync)
				(void) fbiwrite(fbp, ip, nb);
			else
				fbrelse(fbp, S_WRITE);

			ip->i_size = (llbn + 1) << fs->fs_bshift;
			ip->i_db[llbn] = nb;
			ip->i_flag |= IUPD | ICHG;
			ip->i_blocks += btodb(bsize - osize);

			if (nb != ob)
				free(ip, ob, (off_t)osize);
		}
	}

	/*
	 * The first NDADDR blocks are direct blocks
	 */
	if (lbn < NDADDR) {
		nb = ip->i_db[lbn];
		if (rw != S_WRITE)
			goto gotit;

		if (nb == 0 || ip->i_size < (lbn + 1) << fs->fs_bshift) {
			if (nb != 0) {
				/* consider need to reallocate a frag */
				osize = fragroundup(fs, blkoff(fs, ip->i_size));
				nsize = fragroundup(fs, size);
				if (nsize <= osize)
					goto gotit;

				/* need to allocate a block or frag */
				ob = nb;
				pref = blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = realloccg(ip, ob, pref, osize, nsize,
				    &nb);
				if (err)
					return (err);
			} else {
				/* need to allocate a block or frag */
				osize = 0;
				if (ip->i_size < (lbn + 1) << fs->fs_bshift)
					nsize = fragroundup(fs, size);
				else
					nsize = bsize;
				pref = blkpref(ip, lbn, (int)lbn, &ip->i_db[0]);
				err = alloc(ip, pref, nsize, &nb);
				if (err)
					return (err);
				ob = nb;
			}

			/*
			 * Read old/create new zero pages
			 */
			fbp = NULL;
			if (osize == 0) {
				if (!alloc_only)
					fbzero(ITOV(ip),
					    (u_int)(lbn << fs->fs_bshift),
					    (u_int)nsize, &fbp);
			} else {
				err = fbread(ITOV(ip),
				    (u_int)(lbn << fs->fs_bshift),
				    (u_int)nsize, S_OTHER, &fbp);
				if (err) {
					if (nb != ob) {
						free(ip, nb, (off_t)nsize);
					} else {
						free(ip,
						    ob + numfrags(fs, osize),
						    (off_t)(nsize - osize));
					}
#ifdef QUOTA
					(void) chkdq(ip,
					    -(long)btodb(nsize - osize), 0);
#endif QUOTA
					return (err);
				}
			}

			if (isdir) {
				/*
				 * Write directory blocks synchronously
				 * so they never appear with garbage in
				 * them on the disk.
				 */
				(void) fbiwrite(fbp, ip, nb);
			} else {
				if (fbp != NULL)
					fbrelse(fbp, S_WRITE);
			}

			ip->i_db[lbn] = nb;
			ip->i_blocks += btodb(nsize - osize);
			ip->i_flag |= IUPD | ICHG;

			if (nb != ob)
				free(ip, ob, (off_t)osize);
		}
gotit:
		if (bnp != NULL)
			*bnp = (nb == 0)? UFS_HOLE : nb;
		if (rabnp != NULL) {
			nb = ip->i_db[lbn + 1];
			*rabnp = (nb == 0 || lbn >= NDADDR - 1) ? UFS_HOLE : nb;
		}
		return (0);
	}

	/*
	 * Determine how many levels of indirection.
	 */
	pref = 0;
	sh = 1;
	tbn = lbn - NDADDR;
	for (j = NIADDR; j > 0; j--) {
		sh *= NINDIR(fs);
		if (tbn < sh)
			break;
		tbn -= sh;
	}

	if (j == 0)
		return (EFBIG);

	/*
	 * Fetch the first indirect block.
	 */
	devvp = ip->i_devvp;
	nb = ip->i_ib[NIADDR - j];
	if (nb == 0) {
		if (rw != S_WRITE) {
			if (bnp != NULL)
				*bnp = UFS_HOLE;
			if (rabnp != NULL)
				*rabnp = UFS_HOLE;
			return (0);
		}
		/*
		 * Need to allocate an indirect block.
		 */
		pref = blkpref(ip, lbn, 0, (daddr_t *)0);
		err = alloc(ip, pref, bsize, &nb);
		if (err)
			return (err);
		/*
		 * Write zero block synchronously so that
		 * indirect blocks never point at garbage.
		 */
		bp = getblk(devvp, fsbtodb(fs, nb), bsize);
		clrbuf(bp);
		bwrite(bp);

		ip->i_ib[NIADDR - j] = nb;
		ip->i_blocks += btodb(bsize);
		ip->i_flag |= IUPD | ICHG;

		/*
		 * In the ISYNC case, rwip will notice that the block
		 * count on the inode has changed and will be sure to
		 * iupdat the inode at the end of rwip.
		 */
	}

	/*
	 * Fetch through the indirect blocks.
	 */
	for (; j <= NIADDR; j++) {
		ob = nb;
		bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, ob), bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return (EIO);
		}
		bap = bp->b_un.b_daddr;
		sh /= NINDIR(fs);
		i = (tbn / sh) % NINDIR(fs);
		nb = bap[i];
		if (nb == 0) {
			if (rw != S_WRITE) {
				brelse(bp);
				if (bnp != NULL)
					*bnp = UFS_HOLE;
				if (rabnp != NULL)
					*rabnp = UFS_HOLE;
				return (0);
			}
			if (pref == 0) {
				if (j < NIADDR) {
					/* Indirect block */
					pref = blkpref(ip, lbn, 0,
						(daddr_t *)0);
				} else {
					/* Data block */
					pref = blkpref(ip, lbn, i, &bap[0]);
				}
			}

			err = alloc(ip, pref, bsize, &nb);
			if (err) {
				brelse(bp);
				return (err);
			}

			if (j < NIADDR) {
				/*
				 * Write synchronously so indirect
				 * blocks never point at garbage.
				 */
				nbp = getblk(devvp, fsbtodb(fs, nb), bsize);
				clrbuf(nbp);
				bwrite(nbp);
			} else if (!alloc_only ||
			    roundup(size, PAGESIZE) < bsize) {
				/*
				 * To avoid deadlocking if the pageout
				 * daemon decides to push a page for this
				 * inode while we are sleeping holding the
				 * bp but waiting more pages for fbzero,
				 * we give up the bp now.
				 *
				 * XXX - need to avoid having the pageout
				 * daemon get in this situation to begin with!
				 */
				brelse(bp);
				fbzero(ITOV(ip),
				    (u_int)(lbn << fs->fs_bshift),
				    (u_int)bsize, &fbp);

				/*
				 * Cases which we need to do a synchronous
				 * write of the zeroed data pages:
				 *
				 * 1) If we are writing a directory then we
				 * want to write synchronously so blocks in
				 * directories never contain garbage.
				 *
				 * 2) If we are filling in a hole and the
				 * indirect block is going to be synchronously
				 * written back below we need to make sure
				 * that the zeroes are written here before
				 * the indirect block is updated so that if
				 * we crash before the real data is pushed
				 * we will not end up with random data is
				 * the middle of the file.
				 *
				 * 3) If the size of the request rounded up
				 * to the system page size is smaller than
				 * the file system block size, we want to
				 * write out all the pages now so that
				 * they are not aborted before they actually
				 * make it to ufs_putpage since the length
				 * of the inode will not include the pages.
				 */
				if (isdir || (issync && lbn < llbn) ||
				    roundup(size, PAGESIZE) < bsize)
					(void) fbiwrite(fbp, ip, nb);
				else
					fbrelse(fbp, S_WRITE);

				/*
				 * Now get the bp back
				 */
				bp = bread(ip->i_devvp,
				    (daddr_t)fsbtodb(fs, ob), bsize);
				err = geterror(bp);
				if (err) {
					free(ip, nb, (off_t)bsize);
#ifdef QUOTA
					(void) chkdq(ip, -(long)btodb(bsize),
					    0);
#endif QUOTA
					brelse(bp);
					return (err);
				}
				bap = bp->b_un.b_daddr;
			}

			bap[i] = nb;
			ip->i_blocks += btodb(bsize);
			ip->i_flag |= IUPD | ICHG;

			if (issync)
				bwrite(bp);
			else
				bdwrite(bp);
		} else {
			brelse(bp);
		}
	}
	if (bnp != NULL)
		*bnp = nb;
	if (rabnp != NULL) {
		if (i < NINDIR(fs) - 1) {
			nb = bap[i + 1];
			*rabnp = (nb == 0) ? UFS_HOLE : nb;
		} else {
			*rabnp = UFS_HOLE;
		}
	}
	return (0);
}
#endif	/* REMOVE_OLD_UFS */
