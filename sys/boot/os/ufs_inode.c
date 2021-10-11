#ifndef lint
static        char sccsid[] = "@(#)ufs_inode.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */



#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/vfs.h>
#include "boot/vnode.h"
#include <sys/buf.h>
#include <sys/kernel.h>
#include "boot/cmap.h"
#include <ufs/mount.h>
#include "boot/inode.h"
#include <ufs/fs.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif

#ifdef	 NFS_BOOT
#undef u
extern struct user u;
/*
 * Small hash list for /boot.
 */
#define	INOHSZ  1
/*
 * Small inode list also.  Note: the kernel one is set up
 * by config; we just set ours up statically.
 */
#define	NINODES  10
struct inode inodes[NINODES];
struct inode *inode = &inodes[0];
struct inode *inodeNINODE = &inodes[NINODES-1];
int	ninode = NINODES;
#else 
#define	INOHSZ	64
#endif	 /* NFS_BOOT */
#define	INOHASH(dev,ino)	(((unsigned)((dev)+(ino)))%INOHSZ)

union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
} ihead[INOHSZ];

struct inode *ifreeh, **ifreet;

/*
 * Convert inode formats to vnode types
 */
enum vtype iftovt_tab[] = {
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
	register struct inode *ip = inode;
	register union  ihead *ih = ihead;

	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}
	ifreeh = ip;
	ifreet = &ip->i_freef;
	ip->i_freeb = &ifreeh;
	ip->i_forw = ip;
	ip->i_back = ip;
	ip->i_vnode.v_data = (caddr_t)ip;
	ip->i_vnode.v_op = &ufs_vnodeops;
	for (i = ninode; --i > 0; ) {
		++ip;
		ip->i_forw = ip;
		ip->i_back = ip;
		*ifreet = ip;
		ip->i_freeb = ifreet;
		ifreet = &ip->i_freef;
		ip->i_vnode.v_data = (caddr_t)ip;
		ip->i_vnode.v_op = &ufs_vnodeops;
	}
	ip->i_freef = NULL;
}



/*
 * Look up an inode by device,inumber.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: no imt -- if the mounted file
 *	system is not in the mount table.
 *	"cannot happen"
 */
struct inode *
iget(dev, fs, ino)
	dev_t dev;
	register struct fs *fs;
	ino_t ino;
{
	register struct inode *ip;
	register union  ihead *ih;
	register struct buf *bp;
	register struct dinode *dp;
	register struct inode *iq;
	register struct vnode *vp;
	struct mount *mp;

	/*
	 * lookup inode in cache
	 */
loop:
	mp = getmp(dev);
	if (mp == NULL) {
		panic("iget: bad dev\n");
	}
	if (mp->m_bufp->b_un.b_fs != fs)
		panic("iget: bad fs");
	ih = &ihead[INOHASH(dev, ino)];
	for (ip = ih->ih_chain[0]; ip != (struct inode *)ih; ip = ip->i_forw)
		if (ino == ip->i_number && dev == ip->i_dev) {
			/*
			 * found it. check for locks
			 */
			if ((ip->i_flag & ILOCKED) != 0) {
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
			 * mark inode locked and referenced and return it.
			 */
			ip->i_flag |= ILOCKED | IREF;
			VN_HOLD(ITOV(ip));
			return(ip);
		}

	/*
	 * Inode was not in cache. Get free inode slot for new inode.
	 */
	while (ifreeh == NULL) {
	}
	if ((ip = ifreeh) == NULL) {
		tablefull("inode");
		u.u_error = ENFILE;
		return(NULL);
	}
	if (iq = ip->i_freef)
		iq->i_freeb = &ifreeh;
	ifreeh = iq;
	ip->i_freef = NULL;
	ip->i_freeb = NULL;
	/*
	 * Now to take inode off the hash chain it was on
	 * (initially, or after an iflush, it is on a "hash chain"
	 * consisting entirely of itself, and pointed to by no-one,
	 * but that doesn't matter), and put it on the chain for
	 * its new (ino, dev) pair
	 */
	remque(ip);
	insque(ip, ih);
#ifdef QUOTA
	dqrele(ip->i_dquot);
	ip->i_dquot = NULL;
#endif
	ip->i_flag = ILOCKED | IREF;
	ip->i_dev = dev;
	ip->i_devvp = mp->m_devvp;
	ip->i_number = ino;
	ip->i_diroff = 0;
	ip->i_fs = fs;
	ip->i_lastr = 0;
	bp = bread(ip->i_devvp, (daddr_t)fsbtodb(fs, itod(fs, ino)),
	    (int)fs->fs_bsize);
	/*
	 * Check I/O errors
	 */
	if ((bp->b_flags & B_ERROR) != 0) {
		brelse(bp);
		/*
		 * the inode doesn't contain anything useful, so it would
		 * be misleading to leave it on its hash chain.
		 * 'iput' will take care of putting it back on the free list.
		 */
		remque(ip);
		ip->i_forw = ip;
		ip->i_back = ip;
		/*
		 * we also loose its inumber, just in case (as iput
		 * doesn't do that any more) - but as it isn't on its
		 * hash chain, I doubt if this is really necessary .. kre
		 * (probably the two methods are interchangable)
		 */
		ip->i_number = 0;
		iunlock(ip);
		iinactive(ip);
		return(NULL);
	}
	dp = bp->b_un.b_dino;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;
	vp = ITOV(ip);
	VN_INIT(vp, mp->m_vfsp, IFTOVT(ip->i_mode), ip->i_rdev);
	if (ino == (ino_t)ROOTINO) {
		vp->v_flag |= VROOT;
	}
	brelse(bp);
#ifdef QUOTA
	if (ip->i_mode != 0)
		ip->i_dquot = getinoquota(ip);
#endif
	return (ip);
}


/*
 * Vnode is no loger referenced, write the inode out and if necessary,
 * truncate and deallocate the file.
 */
iinactive(ip)
	register struct inode *ip;
{

	if (ip->i_flag & ILOCKED)
		panic("ufs_inactive");
	if (ip->i_fs->fs_ronly == 0) {
#ifdef	 NFS_BOOT
		/*
		 * When booting all inodes are read only.
		 */
#else 
		ip->i_flag |= ILOCKED;
		if (ip->i_nlink <= 0) {
			ip->i_gen++;
			itrunc(ip, (u_long)0);
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_rdev = 0;
			imark(ip, IUPD|ICHG);
			ifree(ip, ip->i_number, mode);
#ifdef QUOTA
			(void)chkiq(VFSTOM(ip->i_vnode.v_vfsp),
			    ip, ip->i_uid, 0);
			dqrele(ip->i_dquot);
			ip->i_dquot = NULL;
#endif
		}
		if (ip->i_flag & (IUPD|IACC|ICHG))
			iupdat(ip, 0);
		iunlock(ip);
#endif	 /* NFS_BOOT */
	}
	ip->i_flag = 0;
	/*
	 * Put the inode on the end of the free list.
	 * Possibly in some cases it would be better to
	 * put the inode at the head of the free list,
	 * (eg: where i_mode == 0 || i_number == 0)
	 * but I will think about that later .. kre
	 * (i_number is rarely 0 - only after an i/o error in iget,
	 * where i_mode == 0, the inode will probably be wanted
	 * again soon for an ialloc, so possibly we should keep it)
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


/*
 * Mark the accessed, updated, or changed times in an inode
 * with the current (unique) time
 */
imark(ip, flag)
	register struct inode *ip;
	register int flag;
{
	struct timeval ut;

#ifdef	NFS_BOOT
	ut.tv_sec = 0;
	ut.tv_usec = 0;
#else
	uniqtime(&ut);
#endif	/* NFS_BOOT */
	ip->i_flag |= flag;
	if (flag & IACC)
		ip->i_atime = ut;
	if (flag & IUPD)
		ip->i_mtime = ut;
	if (flag & ICHG) {
		ip->i_diroff = 0;
		ip->i_ctime = ut;
	}
}


/*
 * Lock an inode. If its already locked, set the WANT bit and sleep.
 */
ilock(ip)
	register struct inode *ip;
{

	ILOCK(ip);
}

/*
 * Unlock an inode.  If WANT bit is on, wakeup.
 */
iunlock(ip)
	register struct inode *ip;
{

	if (!(ip->i_flag & ILOCKED)) {
		panic("iunlock");
	}
	IUNLOCK(ip);
}


/*
 * Check mode permission on inode.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
iaccess(ip, m)
	register struct inode *ip;
	register int m;
{
	register int *gp;

	if (m & IWRITE) {
		register struct vnode *vp;

		vp = ITOV(ip);
		/*
		 * Disallow write attempts on read-only
		 * file systems; unless the file is a block
		 * or character device resident on the
		 * file system.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK) {
				u.u_error = EROFS;
				return (EROFS);
			}
		}
		/*
		 * If there's shared text associated with
		 * the inode, try to free it up once.  If
		 * we fail, we can't allow writing.
		 */
		if (vp->v_flag & VTEXT)
			xrele(vp);
		if (vp->v_flag & VTEXT) {
			u.u_error = ETXTBSY;
			return (ETXTBSY);
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
	u.u_error = EACCES;
	return (EACCES);
}

