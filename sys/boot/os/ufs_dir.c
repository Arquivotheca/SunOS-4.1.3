#ifndef lint
static        char sccsid[] = "@(#)ufs_dir.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Directory manipulation routines.
 * From outside this file, only dirlook, direnter and dirremove
 * should be called.
 */

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/dnlc.h>
#include "boot/inode.h"
#include <ufs/fs.h>
#include <ufs/mount.h>
#include <ufs/fsdir.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif

#ifdef	 NFS_BOOT
#include <mon/sunromvec.h>
#undef u
extern struct user u;
static int dump_debug = 20;
#endif	 /* NFS_BOOT */

/*
 * A virgin directory.
 */
struct dirtemplate mastertemplate = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

#define LDIRSIZ(len) \
    ((sizeof (struct direct) - (MAXNAMLEN+1)) + ((len+1 + 3) &~ 3))

struct buf *blkatoff();

int dirchk = 0;
/*
 * Look for a certain name in a directory
 * On successful return, *ipp will point to the (locked) inode.
 */
dirlook(dp, namep, ipp)
	register struct inode *dp;
	register char *namep;		/* name */
	register struct inode **ipp;
{
	register struct buf *bp = 0;	/* a buffer of directory entries */
	register struct direct *ep;	/* the current directory entry */
	register struct inode *ip;
	int entryoffsetinblock;		/* offset of ep in bp's buffer */
	int numdirpasses;		/* strategy for directory search */
	int endsearch;			/* offset to end directory search */
	int namlen = strlen(namep);	/* length of name */
	int offset;
	int error;
	register int i;

	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode&IFMT) != IFDIR) {
		dprint(dump_debug, 6, "dirlook: not a directory 0x%x\n", dp);
		return (ENOTDIR);
	}

	if (error = iaccess(dp, IEXEC))		{
		dprint(dump_debug, 6, "dirlook: no access 0x%x\n", dp);
	        return (error);
	}

	ilock(dp);
	if (dp->i_diroff > dp->i_size) {
		dp->i_diroff = 0;
	}
	if (dp->i_diroff == 0) {
		offset = 0;
		numdirpasses = 1;
	} else {
		offset = dp->i_diroff;
		entryoffsetinblock = blkoff(dp->i_fs, offset);
		if (entryoffsetinblock != 0) {
			bp = blkatoff(dp, (off_t)offset, (char **)0);
			if (bp == 0) {
				error = u.u_error;
				goto bad;
			}
		}
		numdirpasses = 2;
	}
	endsearch = roundup(dp->i_size, DIRBLKSIZ);

searchloop:
	while (offset < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(dp->i_fs, offset) == 0) {
			if (bp != NULL)
				brelse(bp);
			bp = blkatoff(dp, (off_t)offset, (char **)0);
			if (bp == 0) {
				error = u.u_error;	/* XXX */
				goto bad;
			}
			entryoffsetinblock = 0;
		}

		ep = (struct direct *)(bp->b_un.b_addr + entryoffsetinblock);

		/*
		 * Inline expansion of dirmangled for speed.
		 */
		i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
		if ((ep->d_reclen & 0x3) || ep->d_reclen == 0 ||
		    ep->d_reclen > i || DIRSIZ(ep) > ep->d_reclen ||
		    dirchk &&
		    (ep->d_namlen > MAXNAMLEN ||
		    dirbadname(ep->d_name, (int)ep->d_namlen))) {
			dirbad(dp, "mangled entry", entryoffsetinblock);
			offset += i;
			entryoffsetinblock += i;
			continue;
		}

		/*
		 * Check for a name match.
		 * We must get the target inode before unlocking
		 * the directory to insure that the inode will not be removed
		 * before we get it.  We prevent deadlock by always fetching
		 * inodes from the root, moving down the directory tree. Thus
		 * when following backward pointers ".." we must unlock the
		 * parent directory before getting the requested directory.
		 * There is a potential race condition here if both the current
		 * and parent directories are removed before the `iget' for the
		 * inode associated with ".." returns.  We hope that this
		 * occurs infrequently since we can't avoid this race condition
		 * without implementing a sophisticated deadlock detection
		 * algorithm. Note also that this simple deadlock detection
		 * scheme will not work if the file system has any hard links
		 * other than ".." that point backwards in the directory
		 * structure.
		 * See comments at head of file about deadlocks.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st chr */
		    bcmp(namep, ep->d_name, (int)ep->d_namlen) == 0) {
			dp->i_diroff = offset;
			if (namlen == 2 && namep[0] == '.' && namep[1] == '.') {
				iunlock(dp);	/* race to get the inode */
				ip = iget(dp->i_dev, dp->i_fs, ep->d_ino);
				if (ip == NULL) {
					error = u.u_error;
					goto bad2;
				}
			} else if (dp->i_number == ep->d_ino) {
				VN_HOLD(ITOV(dp));	/* want ourself, "." */
				ip = dp;
			} else {
				ip = iget(dp->i_dev, dp->i_fs, ep->d_ino);
				iunlock(dp);
				if (ip == NULL) {
					error = u.u_error;
					goto bad2;
				}
			}
			*ipp = ip;
			dp->i_diroff = 0;
			brelse(bp);
			return (0);
		}
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		offset = 0;
		endsearch = dp->i_diroff;
		goto searchloop;
	}
	error = ENOENT;
bad:
	iunlock(dp);
bad2:
	if (bp) {
		dp->i_diroff = 0;
		brelse(bp);
	}
	return (error);
}


/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
struct buf *
blkatoff(ip, offset, res)
	struct inode *ip;
	off_t offset;
	char **res;
{
	register struct fs *fs;
	daddr_t lbn;
	int bsize;
	daddr_t bn;
	register struct buf *bp;

	fs = ip->i_fs;
	lbn = lblkno(fs, offset);
	bsize = blksize(fs, ip, lbn);
	bn = fsbtodb(fs, bmap(ip, lbn, B_READ));
	if (bn < 0) {
		dirbad(ip, "nonexixtent directory block", (int)offset);
		u.u_error = ENOENT;
	} 
	if (u.u_error) {
		return (0);
	}
	bp = bread(ip->i_devvp, bn, bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (0);
	}
	if (res)
		*res = bp->b_un.b_addr + blkoff(fs, offset);
	return (bp);
}


dirbad(ip, how, offset)
	struct inode *ip;
	char *how;
	int offset;
{

	printf("%s: bad dir ino %d at offset %d: %s\n",
	    ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
}

dirbadname(sp, l)
	register char *sp;
	register int l;
{
	register char c;

	while (l--) {			/* check for nulls or high bit */
		c = *sp++;
		if ((c == 0) || (c & 0200)) {
			return (1);
		}
	}
	return (*sp);			/* check for terminating null */
}


