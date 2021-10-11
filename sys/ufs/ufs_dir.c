#ident	"@(#)ufs_dir.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Directory manipulation routines.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vfs_stat.h>
#include <sys/vnode.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/dnlc.h>
#include <ufs/inode.h>
#include <ufs/fs.h>
#include <ufs/mount.h>
#include <ufs/fsdir.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif

#include <vm/seg.h>

/*
 * A virgin directory.
 */
struct dirtemplate mastertemplate = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

#define	LDIRSIZ(len) \
	((sizeof (struct direct) - (MAXNAMLEN + 1)) + ((len + 1 + 3) &~ 3))

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
	struct fbuf *fbp = NULL;	/* a buffer of directory entries */
	register struct direct *ep;	/* the current directory entry */
	register struct inode *ip;
	struct vnode *vp, *dnlc_lookup();
	int entryoffsetinblock;		/* offset of ep in addr's buffer */
	int numdirpasses;		/* strategy for directory search */
	off_t endsearch;		/* offset to end directory search */
	int namlen = strlen(namep);	/* length of name */
	off_t offset;
	int err;
	register int i;

	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		return (ENOTDIR);
	}

	if (err = iaccess(dp, IEXEC))
		return (err);

	/*
	 * Check the directory name lookup cache.
	 */
	vp = dnlc_lookup(ITOV(dp), namep, NOCRED);
	if (vp) {
		VN_HOLD(vp);
		*ipp = VTOI(vp);
		ilock(*ipp);
#if defined(DEBUG_INODES)
		printinode("dirlook: cache hit", *ipp);
#endif
		return (0);
	}

	VFS_RECORD(ITOV(dp)->v_vfsp, VS_LOOKUP, VS_MISS);

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
			err = blkatoff(dp, offset, (char **)0, &fbp);
			if (err) {
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
			if (fbp != NULL)
				fbrelse(fbp, S_OTHER);
			err = blkatoff(dp, offset, (char **)0, &fbp);
			if (err) {
				goto bad;
			}
			entryoffsetinblock = 0;
		}

		/*
		 * Get pointer to next entry.
		 * Full validation checks are slow, so we only check
		 * enough to insure forward progress through the
		 * directory. Complete checks can be run by patching
		 * "dirchk" to be true.
		 */
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblock);
		if (ep->d_reclen == 0 ||
		    dirchk && dirmangled(dp, ep, entryoffsetinblock, offset)) {
			i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
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
			u_long ep_ino;

			/*
			 * We have to release the fbp early ehere to avoid
			 * a possible deadlock situation where we have the
			 * fbp and want the directory inode and someone doing
			 * a direnter has the directory inode and wants the
			 * fbp.  XXX - is this still needed?
			 */
			ep_ino = ep->d_ino;
			fbrelse(fbp, S_OTHER);
			fbp = NULL;
			dp->i_diroff = offset;
			if (namlen == 2 && namep[0] == '.' && namep[1] == '.') {
				iunlock(dp);		/* race to get inode */
				err = iget(dp->i_dev, dp->i_fs, ep_ino, ipp);
				if (err) {
					goto bad2;
				}
			} else if (dp->i_number == ep_ino) {
				VN_HOLD(ITOV(dp));	/* want ourself, "." */
				*ipp = dp;
			} else {
				err = iget(dp->i_dev, dp->i_fs, ep_ino, ipp);
				iunlock(dp);
				if (err) {
					goto bad2;
				}
			}
			ip = *ipp;
			dnlc_enter(ITOV(dp), namep, ITOV(ip), NOCRED);
#if defined(DEBUG_INODES)
			printinode("dirlook: search hit", *ipp);
#endif
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
	err = ENOENT;
bad:
	iunlock(dp);
bad2:
	if (fbp)
		fbrelse(fbp, S_OTHER);
	return (err);
}

/*
 * If "dircheckforname" fails to find an entry with the given name, this
 * structure holds state for "direnter" as to where there is space to put
 * an entry with that name.
 * If "dircheckforname" finds an entry with the given name, this structure
 * holds state for "dirrename" and "dirremove" as to where the entry is.
 * "status" indicates what "dircheckforname" found:
 *	NONE		name not found, large enough free slot not found,
 *			can't make large enough free slot by compacting entries
 *	COMPACT		name not found, large enough free slot not found,
 *			can make large enough free slot by compacting entries
 *	FOUND		name not found, large enough free slot found
 *	EXIST		name found
 * If "dircheckforname" fails due to an error, this structure is not filled in.
 *
 * After "dircheckforname" succeeds the values are:
 *	status	offset		size		fbp, ep
 *	------	------		----		-------
 *	NONE	end of dir	needed		not valid
 *	COMPACT	start of area	of area		not valid
 *	FOUND	start of entry	of ent		not valid
 *	EXIST	start if entry	of prev ent	valid
 *
 * "endoff" is set to 0 if the an entry with the given name is found, or if no
 * free slot could be found or made; this means that the directory should not
 * be truncated.  If the entry was found, the search terminates so
 * "dircheckforname" didn't find out where the last valid entry in the
 * directory was, so it doesn't know where to cut the directory off; if no free
 * slot could be found or made, the directory has to be extended to make room
 * for the new entry, so there's nothing to cut off.
 * Otherwise, "endoff" is set to the larger of the offset of the last
 * non-empty entry in the directory, or the offset at which the new entry will
 * be placed, whichever is larger.  This is used by "diraddentry"; if a new
 * entry is to be added to the directory, any complete directory blocks at the
 * end of the directory that contain no non-empty entries are lopped off the
 * end, thus shrinking the directory dynamically.
 *
 * On success, "dirprepareentry" makes "fbp" and "ep" valid.
 */
struct slot {
	enum	{NONE, COMPACT, FOUND, EXIST} status;
	off_t	offset;		/* offset of area with free space */
	int	size;		/* size of area at slotoffset */
	struct	fbuf *fbp;	/* dir buf where slot is */
	struct direct *ep;	/* pointer to slot */
	off_t	endoff;		/* last useful location found in search */
};

/*
 * Write a new directory entry.
 * The directory must not have been removed and must be writeable.
 * There are three operations in building the new entry: creating a file
 * or directory (DE_CREATE), renaming (DE_RENAME) or linking (DE_LINK).
 * There are five possible cases to consider:
 *	Name
 *	found	op			action
 *	----	---			-------------------------------
 *	no	DE_CREATE		create file according to vap and enter
 *	no	DE_LINK | DE_RENAME	enter the file sip
 *	yes	DE_CREATE		error EEXIST *ipp = found file
 *	yes	DE_LINK			error EEXIST
 *	yes	DE_RENAME		remove existing file, enter new file
 */
direnter(tdp, namep, op, sdp, sip, vap, ipp)
	register struct inode *tdp;	/* target directory to make entry in */
	register char *namep;		/* name of entry */
	enum de_op op;			/* entry operation */
	register struct inode *sdp;	/* source inode parent if rename */
	struct inode *sip;		/* source inode if link/rename */
	struct vattr *vap;		/* attributes if new inode needed */
	struct inode **ipp;		/* return entered inode (locked) here */
{
	struct inode *tip;		/* inode of (existing) target file */
	struct slot slot;		/* slot info to pass around */
	register int namlen;		/* length of name */
	register int err;		/* error number */
	register char *s;

	/*
	 *	Don't allow '/' characters in pathname components
	 */
	for (s = namep, namlen = 0; *s; s++, namlen++)
		if (*s == '/')
			return (EACCES);
	if (namlen == 0)
		panic("direnter");

	/*
	 * If name is "." or ".." then if this is a create look it up
	 * and return EEXIST.  Rename or link TO "." or ".." is forbidden.
	 */
	if (namep[0] == '.' &&
	    (namlen == 1 || (namlen == 2 && namep[1] == '.'))) {
		if (op == DE_RENAME) {
			return (ENOTEMPTY);
		}
		if (ipp) {
			if (err = dirlook(tdp, namep, ipp))
				return (err);
		}
		return (EEXIST);
	}
	slot.status = NONE;
	slot.fbp = NULL;
	/*
	 * Lock the directory in which we are trying to make the new entry.
	 */
	ilock(tdp);
	/*
	 * For link and rename lock the source entry and check the link count
	 * to see if it has been removed while it was unlocked.  If not, we
	 * increment the link count and force the inode to disk to make sure
	 * that it is there before any directory entry that points to it.
	 */
	if (op != DE_CREATE) {
		ilock(sip);
		if (sip->i_nlink == 0) {
			iunlock(sip);
			iunlock(tdp);
			return (ENOENT);
		}
		if (sip->i_nlink == MAXLINK) {
			iunlock(sip);
			iunlock(tdp);
			return (EMLINK);
		}
		sip->i_nlink++;
		sip->i_flag |= ICHG;
		iupdat(sip, 1);
		iunlock(sip);
	}
	/*
	 * Check accessiblity of directory.
	 */
	if ((tdp->i_mode & IFMT) != IFDIR) {
		err = ENOTDIR;
		goto out;
	}
	/*
	 * If target directory has not been removed, then we can consider
	 * allowing file to be created.
	 */
	if (tdp->i_nlink == 0) {
		err = ENOENT;
		goto out;
	}
	/*
	 * Execute access is required to search the directory.
	 */
	if (err = iaccess(tdp, IEXEC)) {
		goto out;
	}
	/*
	 * If this is a rename and we are doing a directory and the parent
	 * is different (".." must be changed), then the source directory must
	 * not be in the directory heirarchy above the target, as this would
	 * orphan everything below the source directory.  Also the user must
	 * have write permission in the source so as to be able to change "..".
	 */
	if ((op == DE_RENAME) &&
	    ((sip->i_mode & IFMT) == IFDIR) && (sdp != tdp)) {
		if (err = iaccess(sip, IWRITE))
			goto out;
		if (err = dircheckpath(sip, tdp))
			goto out;
	}
	/*
	 * Search for the entry.
	 */
	err = dircheckforname(tdp, namep, namlen, &slot, &tip);
	if (err) {
		goto out;
	}

	if (tip) {
		switch (op) {

		case DE_CREATE:
			if (ipp) {
				*ipp = tip;
				err = EEXIST;
			} else {
				iput(tip);
			}
			break;

		case DE_RENAME:
			err = dirrename(sdp, sip, tdp, namep,
			    namlen, tip, &slot);
			iput(tip);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file
			 */
			iput(tip);
			err = EEXIST;
			break;
		}
	} else {
		/*
		 * The entry does not exist. Check write permission in
		 * directory to see if entry can be created.
		 */
		if (err = iaccess(tdp, IWRITE)) {
			goto out;
		}
		if (op == DE_CREATE) {
			/*
			 * make a new inode and directory as required
			 */
			err = dirmakeinode(tdp, &sip, vap);
			if (err) {
				goto out;
			}
		}
		err = diraddentry(tdp, namep, namlen, &slot, sip, sdp);
		if (err) {
			if (op == DE_CREATE) {
				/*
				 * Unmake the inode we just made.
				 */
				if ((sip->i_mode & IFMT) == IFDIR) {
					tdp->i_nlink--;
				}
				sip->i_nlink = 0;
				sip->i_flag |= ICHG;
				irele(sip);
				sip = NULL;
			}
		} else if (ipp) {
			ilock(sip);
			*ipp = sip;
		} else if (op == DE_CREATE) {
			irele(sip);
		}
	}

out:
	if (slot.fbp)
		fbrelse(slot.fbp, S_OTHER);
	if (err && (op != DE_CREATE)) {
		/*
		 * Undo bumped link count
		 */
		sip->i_nlink--;
		sip->i_flag |= ICHG;
	}
	iunlock(tdp);
	return (err);
}

/*
 * Check for the existence of a slot to make a directory entry.
 * On successful return *ipp points at the (locked) inode found.
 * The target directory inode (tdp) is supplied locked.
 * This may not be used on "." or "..", but aliases of "." are ok.
 */
static int
dircheckforname(tdp, namep, namlen, slotp, ipp)
	register struct inode *tdp;	/* inode of directory being checked */
	char *namep;			/* name we're checking for */
	register int namlen;		/* length of name */
	register struct slot *slotp;	/* slot structure */
	struct inode **ipp;		/* return inode if we find one */
{
	int dirsize;			/* size of the directory */
	struct fbuf *fbp;		/* pointer to directory block */
	register int entryoffsetinblk;	/* offset of ep in fbp's buffer */
	int slotfreespace;		/* free space in block */
	register struct direct *ep;	/* directory entry */
	register off_t offset;		/* offset in the directory */
	register off_t last_offset;	/* last offset */
	off_t enduseful;		/* pointer past last used dir slot */
	int i;				/* length of mangled entry */
	int needed;
	int err;

	fbp = NULL;
	entryoffsetinblk = 0;
	needed = LDIRSIZ(namlen);
	/*
	 * No point in using i_diroff since we must search whole directory
	 */
	dirsize = roundup(tdp->i_size, DIRBLKSIZ);
	enduseful = 0;
	offset = last_offset = 0;
	while (offset < dirsize) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(tdp->i_fs, offset) == 0) {
			if (fbp != NULL)
				fbrelse(fbp, S_OTHER);
			err = blkatoff(tdp, offset, (char **)0, &fbp);
			if (err) {
				return (err);
			}
			entryoffsetinblk = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZ
		 * boundary, have to start looking for free space
		 * again.
		 */
		if (slotp->status == NONE &&
		    (entryoffsetinblk&(DIRBLKSIZ-1)) == 0) {
			slotp->offset = -1;
			slotfreespace = 0;
		}
		/*
		 * Get pointer to next entry.
		 * Since we are going to do some entry manipulation
		 * we call dirmangled to do more thorough checks.
		 */
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblk);
		if (ep->d_reclen == 0 ||
		    dirmangled(tdp, ep, entryoffsetinblk, offset)) {
			i = DIRBLKSIZ - (entryoffsetinblk & (DIRBLKSIZ - 1));
			offset += i;
			entryoffsetinblk += i;
			continue;
		}
		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slotp->status != FOUND) {
			int size = ep->d_reclen;

			if (ep->d_ino != 0)
				size -= DIRSIZ(ep);
			if (size > 0) {
				if (size >= needed) {
					slotp->status = FOUND;
					slotp->offset = offset;
					slotp->size = ep->d_reclen;
				} else if (slotp->status == NONE) {
					slotfreespace += size;
					if (slotp->offset == -1)
						slotp->offset = offset;
					if (slotfreespace >= needed) {
						slotp->status = COMPACT;
						slotp->size =
						    offset + ep->d_reclen -
						    slotp->offset;
					}
				}
			}
		}
		/*
		 * Check for a name match.
		 */
		if (ep->d_ino && ep->d_namlen == namlen &&
		    *namep == *ep->d_name &&	/* fast chk 1st char */
		    bcmp(namep, ep->d_name, namlen) == 0) {
			tdp->i_diroff = offset;
			if (tdp->i_number == ep->d_ino) {
				*ipp = tdp;	/* we want ourself, ie "." */
				VN_HOLD(ITOV(tdp));
			} else {
				err = iget(tdp->i_dev, tdp->i_fs,
				    ep->d_ino, ipp);
				if (err) {
					fbrelse(fbp, S_OTHER);
					return (err);
				}
			}
			slotp->status = EXIST;
			slotp->offset = offset;
			slotp->size = offset - last_offset;
			slotp->fbp = fbp;
			slotp->ep = ep;
			slotp->endoff = 0;
			return (0);
		}
		last_offset = offset;
		offset += ep->d_reclen;
		entryoffsetinblk += ep->d_reclen;
		if (ep->d_ino)
			enduseful = offset;
	}
	if (fbp) {
		fbrelse(fbp, S_OTHER);
	}
	if (slotp->status == NONE) {
		/*
		 * We didn't find a slot; the new directory entry should be put
		 * at the end of the directory.  Return an indication of where
		 * this is, and set "endoff" to zero; since we're going to have
		 * to extend the directory, we're certainly not going to
		 * trucate it.
		 */
		slotp->offset = dirsize;
		slotp->size = DIRBLKSIZ;
		slotp->endoff = 0;
	} else {
		/*
		 * We found a slot, and will return an indication of where that
		 * slot is, as any new directory entry will be put there.
		 * Since that slot will become a useful entry, if the last
		 * useful entry we found was before this one, update the offset
		 * of the last useful entry.
		 */
		if (enduseful < slotp->offset + slotp->size)
			enduseful = slotp->offset + slotp->size;
		slotp->endoff = roundup(enduseful, DIRBLKSIZ);
	}
	*ipp = (struct inode *)NULL;
	return (0);
}

/*
 * Rename the entry in the directory tdp so that
 * it points to sip instead of tip.
 */
/*ARGSUSED*/
static
dirrename(sdp, sip, tdp, namep, namlen, tip, slotp)
	register struct inode *sdp;	/* parent directory of source */
	register struct inode *sip;	/* source inode */
	register struct inode *tdp;	/* parent directory of target */
	char *namep;			/* entry we are trying to change */
	int namlen;			/* length of entry string */
	struct inode *tip;		/* locked target inode */
	struct slot *slotp;		/* slot for entry */
{
	int err;
	int doingdirectory;

	/*
	 * Check that everything is on the same filesystem.
	 */
	if ((tip->i_vnode.v_vfsp != tdp->i_vnode.v_vfsp) ||
	    (tip->i_vnode.v_vfsp != sip->i_vnode.v_vfsp))
		return (EXDEV);		/* XXX archaic */
	/*
	 * Short circuit rename (foo, foo).
	 */
	if (sip->i_number == tip->i_number)
		return (ESAME);		/* special error code */
	/*
	 * Must have write permission to rewrite target entry.
	 */
	if (err = iaccess(tdp, IWRITE)) {
		return (err);
	}
	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory, or the destination of the rename,
	 * otherwise the destination may not be changed (except by
	 * root).  This implements append-only directories.
	 */
	if ((tdp->i_mode & ISVTX) && u.u_uid != 0 &&
	    u.u_uid != tdp->i_uid && tip->i_uid != u.u_uid)
		return (EPERM);

	/*
	 * Ensure source and target are compatible
	 * (both directories or both not directories).
	 * If target is a directory it must be empty
	 * and have no links to it.
	 */
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);
	if ((tip->i_mode & IFMT) == IFDIR) {
		if (!doingdirectory)
			return (ENOTDIR);
		if (!dirempty(tip, tdp->i_number) || (tip->i_nlink > 2))
			return (ENOTEMPTY);
	} else if (doingdirectory) {
		return (ENOTDIR);
	}

	/*
	 * Rewrite the inode pointer for target name entry
	 * from the target inode (ip) to the source inode (sip).
	 * This prevents the target entry from disappearing
	 * during a crash. Mark the directory inode to reflect the changes.
	 */
	dnlc_remove(ITOV(tdp), namep);
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);
	err = fbwrite(slotp->fbp, tdp);
	slotp->fbp = NULL;
	if (err)
		return (err);
	tdp->i_flag |= IUPD|ICHG;
	/*
	 * Decrement the link count of the target inode.
	 * Fix the ".." entry in sip to point to dp.
	 * This is done after the new entry is on the disk.
	 */
	tip->i_nlink--;
	tip->i_flag |= ICHG;
	if (doingdirectory) {
		/*
		 * Decrement target link count once more if it was a directory.
		 */
		if (--tip->i_nlink != 0) {
			panic("direnter: target directory link count");
		}
		(void) itrunc(tip, (u_long)0);
		/*
		 * Renaming a directory with the parent different requires
		 * ".." to be rewritten. The window is still there for ".."
		 * to be inconsistent, but this is unavoidable, and a lot
		 * shorter than when it was done in a user process.
		 * Unconditionally decrement the link count in the new parent;
		 * If the parent is the same, we decrement the link count,
		 * since the original directory is going away.  If the
		 * new parent is different, dirfixdotdot() will bump the
		 * link count back.
		 */
		tdp->i_nlink--;
		tdp->i_flag |= ICHG;
		if (sdp != tdp) {
			err = dirfixdotdot(sip, sdp, tdp);
			if (err) {
				return (err);
			}
		}
	}
	return (0);
}

/*
 * Fix the ".." entry of the child directory from the old parent to the
 * new parent directory.
 * Assumes dp is a directory and that all the inodes are on the same
 * file system.
 */
static
dirfixdotdot(dp, opdp, npdp)
	register struct inode *dp;	/* child directory */
	register struct inode *opdp;	/* old parent directory */
	register struct inode *npdp;	/* new parent directory */
{
	struct fbuf *fbp;
	struct dirtemplate *dirp;
	int err;

	ilock(dp);
	/*
	 * check whether this is an ex-directory
	 */
	if ((dp->i_nlink == 0) || (dp->i_size < sizeof (struct dirtemplate))) {
		iunlock(dp);
		return (0);
	}
	err = blkatoff(dp, (off_t)0, (char **)&dirp, &fbp);
	if (err) {
		goto bad;
	}
	if (dirp->dotdot_ino == npdp->i_number) {   /* just a no-op */
		goto bad;
	}
	if (dirp->dotdot_namlen != 2 ||
	    dirp->dotdot_name[0] != '.' ||
	    dirp->dotdot_name[1] != '.') {
		dirbad(dp, "mangled .. entry", (off_t)0);
		err = EINVAL;
		goto bad;
	}

	/*
	 * Increment the link count in the new parent inode and force it out.
	 */
	npdp->i_nlink++;
	npdp->i_flag |= ICHG;
	iupdat(npdp, 1);

	/*
	 * Rewrite the child ".." entry and force it out.
	 */
	dnlc_remove(ITOV(dp), "..");
	dirp->dotdot_ino = npdp->i_number;
	dnlc_enter(ITOV(dp), "..", ITOV(npdp), NOCRED);
	err = fbwrite(fbp, dp);
	fbp = NULL;
	if (err)
		goto bad;
	iunlock(dp);

	/*
	 * Decrement the link count of the old parent inode and force it out.
	 * If opdp is NULL, then this is a new directory link; it has no
	 * parent, so we need not do anything.
	 */
	if (opdp != NULL) {
		ilock(opdp);
		if (opdp->i_nlink != 0) {
			opdp->i_nlink--;
			opdp->i_flag |= ICHG;
			iupdat(opdp, 1);
		}
		iunlock(opdp);
	}
	return (0);
bad:
	if (fbp)
		fbrelse(fbp, S_OTHER);
	iunlock(dp);
	return (err);
}

/*
 * Enter the file sip in the directory tdp with name namep.
 */
static
diraddentry(tdp, namep, namlen, slotp, sip, sdp)
	struct inode *tdp;
	char *namep;
	int namlen;
	struct slot *slotp;
	struct inode *sip;
	struct inode *sdp;
{
	int err;
	char *strncpy();

	/*
	 * Prepare a new entry.  If the caller has not supplied an
	 * existing inode, make a new one.
	 */
	err = dirprepareentry(tdp, slotp);
	if (err) {
		return (err);
	}
	/*
	 * Check inode to be linked to see if it is in the
	 * same filesystem.
	 */
	if (tdp->i_vnode.v_vfsp != sip->i_vnode.v_vfsp) {
		err = EXDEV;
		goto bad;
	}
	if ((sip->i_mode & IFMT) == IFDIR) {
		err = dirfixdotdot(sip, sdp, tdp);
		if (err) {
			goto bad;
		}
	}

	/*
	 * Fill in entry data
	 */
	slotp->ep->d_namlen = namlen;
	(void) strncpy(slotp->ep->d_name, namep, (namlen + 4) & ~3);
	slotp->ep->d_ino = sip->i_number;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NOCRED);

	/*
	 * Write out the directory entry.
	 */
	err = fbwrite(slotp->fbp, tdp);
	slotp->fbp = NULL;
	if (err)
		return (err);		/* XXX - already fixed dotdot? */

	/*
	 * Mark the directory inode to reflect the changes.
	 * Truncate the directory to chop off blocks of empty entries.
	 */
	tdp->i_flag |= IUPD|ICHG;
	tdp->i_diroff = 0;
	if (slotp->endoff && slotp->endoff < tdp->i_size)
		(void) itrunc(tdp, (u_long)slotp->endoff);
	return (0);

bad:
	/*
	 * Clear out entry prepared by dirprepareent.
	 */
	slotp->ep->d_ino = 0;
	(void) fbwrite(slotp->fbp, tdp);	/* XXX - is this right? */
	slotp->fbp = NULL;
	return (err);
}

/*
 * Prepare a directory slot to receive an entry.
 */
static
dirprepareentry(dp, slotp)
	register struct inode *dp;	/* directory we are working in */
	register struct slot *slotp;	/* available slot info */
{
	register int slotfreespace;
	register int dsize;
	register int loc;
	register struct direct *ep, *nep;
	char *dirbuf;
	off_t entryend;
	int err;

	/*
	 * If we didn't find a slot, then indicate that the
	 * new slot belongs at the end of the directory.
	 * If we found a slot, then the new entry can be
	 * put at slotp->offset.
	 */
	entryend = slotp->offset + slotp->size;
	if (slotp->status == NONE) {
		if (slotp->offset & (DIRBLKSIZ - 1))
			panic("dirprepareentry: new block");
		if (DIRBLKSIZ > dp->i_fs->fs_fsize)
			panic("DIRBLKSIZ > fsize");
		/*
		 * Allocate the new block.
		 */
		err = BMAPALLOC(dp, (daddr_t)lblkno(dp->i_fs, slotp->offset),
		    (int)(blkoff(dp->i_fs, slotp->offset) + DIRBLKSIZ));
		if (err)
			return (err);
		dp->i_size = entryend;
		dp->i_flag |= IUPD|ICHG;
	} else if (entryend > dp->i_size) {
		/*
		 * Adjust directory size, if needed. This should never
		 * push the size past a new multiple of DIRBLKSIZ.
		 * This is an artifact of the old (4.2BSD) way of initializing
		 * directory sizes to be less than DIRBLKSIZ.
		 */
		dp->i_size = roundup(entryend, DIRBLKSIZ);
		dp->i_flag |= IUPD|ICHG;
	}

	/*
	 * Get the block containing the space for the new directory entry.
	 */
	err = blkatoff(dp, slotp->offset, (char **)&slotp->ep, &slotp->fbp);
	if (err) {
		return (err);
	}

	ep = slotp->ep;
	switch (slotp->status) {
	case NONE:
		/*
		 * No space in the directory. slotp->offset will be on a
		 * directory block boundary and we will write the new entry
		 * into a fresh block.
		 */
		ep->d_reclen = DIRBLKSIZ;
		break;

	case FOUND:
	case COMPACT:
		/*
		 * Found space for the new entry
		 * in the range slotp->offset to slotp->offset + slotp->size
		 * in the directory.  To use this space, we have to compact
		 * the entries located there, by copying them together towards
		 * the beginning of the block, leaving the free space in
		 * one usable chunk at the end.
		 */
		dirbuf = (char *)ep;
		dsize = DIRSIZ(ep);
		slotfreespace = ep->d_reclen - dsize;
		for (loc = ep->d_reclen; loc < slotp->size; ) {
			nep = (struct direct *)(dirbuf + loc);
			if (ep->d_ino) {
				/* trim the existing slot */
				ep->d_reclen = dsize;
				ep = (struct direct *)((char *)ep + dsize);
			} else {
				/* overwrite; nothing there; header is ours */
				slotfreespace += dsize;
			}
			dsize = DIRSIZ(nep);
			slotfreespace += nep->d_reclen - dsize;
			loc += nep->d_reclen;
			bcopy((caddr_t)nep, (caddr_t)ep, (unsigned)dsize);
		}
		/*
		 * Update the pointer fields in the previous entry (if any).
		 * At this point, ep is the last entry in the range
		 * slotp->offset to slotp->offset + slotp->size.
		 * Slotfreespace is the now unallocated space after the
		 * ep entry that resulted from copying entries above.
		 */
		if (ep->d_ino == 0) {
			ep->d_reclen = slotfreespace + dsize;
		} else {
			ep->d_reclen = dsize;
			ep = (struct direct *)((char *)ep + dsize);
			ep->d_reclen = slotfreespace;
		}
		break;

	default:
		panic("dirprepareentry: invalid slot status");
	}
	slotp->ep = ep;
	return (0);
}

/*
 * Allocate and initialize a new inode that will go
 * into directory tdp.
 */
static
dirmakeinode(tdp, ipp, vap)
	struct inode *tdp;
	struct inode **ipp;
	register struct vattr *vap;
{
	register enum vtype type;
	struct inode *ip;
	int imode;			/* mode and format as in inode */
	ino_t ipref;
	int err;

	if (vap == (struct vattr *)0) {
		panic("dirmakeinode: no attributes");
	}
	/*
	 * Allocate a new inode.
	 */
	type = vap->va_type;
	if (type == VDIR) {
		ipref = dirpref(tdp->i_fs);
	} else {
		ipref = tdp->i_number;
	}
	imode = MAKEIMODE(type, vap->va_mode);
	err = ialloc(tdp, ipref, imode, &ip);
	if (err)
		return (err);
#ifdef QUOTA
	if (ip->i_dquot != NULL)
		panic("direnter: dquot");
#endif
	ip->i_flag |= IACC|IUPD|ICHG;
	ip->i_mode = imode;
	if (type == VBLK || type == VCHR) {
		ip->i_vnode.v_rdev = ip->i_rdev = vap->va_rdev;
	}
	ip->i_vnode.v_type = type;
	if (type == VDIR) {
		ip->i_nlink = 2;    /* anticipating a call to dirmakedirect */
	} else {
		ip->i_nlink = 1;
	}
	ip->i_uid = u.u_uid;
	/*
	 * To determine the group-id of the created file:
	 *  1) If the gid is set in the attribute list (non-Sun & pre-4.0
	 *	clients are not likely to set the gid), then use it if
	 *	the process is super-user, belongs to the target group,
	 *	or the group is the same as the parent directory.
	 *  2) If the filesystem was not mounted with the Old-BSD-compatible
	 *	GRPID option, and the directory's set-gid bit is clear,
	 *	then use the process's gid.
	 *  3) Otherwise, set the group-id to the gid of the parent directory.
	 */
	if ((vap->va_gid != (gid_t)-1) &&
	    ((u.u_uid == 0) || (vap->va_gid == tdp->i_gid) ||
	    groupmember((int)vap->va_gid))) {
		ip->i_gid = vap->va_gid;
	} else if (!(tdp->i_vnode.v_vfsp->vfs_flag & VFS_GRPID) &&
	    !(tdp->i_mode & ISGID)) {
		ip->i_gid = u.u_gid;
	} else
		ip->i_gid = tdp->i_gid;

	/*
	 * If the user is neither super-user nor a member of the file's
	 * new group, clear the file's set-gid bit.
	 */
	if ((u.u_uid != 0) &&
	    (ip->i_mode & ISGID) && !groupmember((int)ip->i_gid)) {
		ip->i_mode &= ~ISGID;
	}
#ifdef QUOTA
	ip->i_dquot = getinoquota(ip);
#endif
	/*
	 * Make sure inode goes to disk before directory data and entries
	 * pointing to it.
	 * Then unlock it, since nothing points to it yet.
	 */
	if (type == VDIR) {
		err = dirmakedirect(ip, tdp);
	}
	iupdat(ip, 1);
	if (err) {
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
		iput(ip);
	} else {
		iunlock(ip);
		*ipp = ip;
	}
	return (err);
}

/*
 * Make an empty directory for inode ip in dp.
 */
static
dirmakedirect(ip, dp)
	register struct inode *ip;		/* new directory */
	register struct inode *dp;		/* parent directory */
{
	register struct dirtemplate *dirp;
	struct fbuf *fbp;
	int err;

	/*
	 * Allocate space for the directory we're creating.
	 */
	err = BMAPALLOC(ip, (daddr_t)0, DIRBLKSIZ);
	if (err)
		return (err);
	if (DIRBLKSIZ > ip->i_fs->fs_fsize)
		panic("DIRBLKSIZ > fsize");
	ip->i_size = DIRBLKSIZ;
	ip->i_flag |= IUPD|ICHG;
	/*
	 * Update the tdp link count and write out the change.
	 * This reflects the ".." entry we'll soon write.
	 */
	dp->i_nlink++;
	dp->i_flag |= ICHG;
	iupdat(dp, 1);
	/*
	 * Initialize directory with "."
	 * and ".." from static template.
	 */
	err = fbread(ITOV(ip), 0, (u_int)ip->i_fs->fs_fsize, S_OTHER, &fbp);
	if (err)
		return (err);
	dirp = (struct dirtemplate *)fbp->fb_addr;
	/*
	 * Now initialize the directory we're creating
	 * with the "." and ".." entries.
	 */
	*dirp = mastertemplate;			/* structure assignment */
	dirp->dot_ino = ip->i_number;
	dirp->dotdot_ino = dp->i_number;
	err = fbwrite(fbp, ip);
	return (err);
}

/*
 * Delete a directory entry, if oip is nonzero the
 * entry is checked to make sure it still reflects oip.
 */
dirremove(dp, namep, oip, rmdir)
	register struct inode *dp;
	char *namep;
	struct inode *oip;
	int rmdir;
{
	register struct direct *ep;
	struct direct *pep;
	struct inode *ip;
	int namlen;
	struct slot slot;
	int err = 0;

	namlen = strlen(namep);
	if (namlen == 0)
		panic("dirremove");
	/*
	 * return error when removing . and ..
	 */
	if (namep[0] == '.') {
		if (namlen == 1)
			return (EINVAL);
		else if (namlen == 2 && namep[1] == '.')
			return (ENOTEMPTY);
	}

	ip = NULL;
	slot.fbp = NULL;
	ilock(dp);
	/*
	 * Check accessiblity of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		err = ENOTDIR;
		goto out;
	}

	/*
	 * Execute access is required to search the directory.
	 * Access for write is interpreted as allowing
	 * deletion of files in the directory.
	 */
	if (err = iaccess(dp, IEXEC|IWRITE)) {
		goto out;
	}

	slot.status = FOUND;	/* don't need to look for empty slot */
	err = dircheckforname(dp, namep, namlen, &slot, &ip);
	if (err) {
		goto out;
	}
	if (ip == (struct inode *)0) {
		err = ENOENT;
		goto out;
	}
	if (oip && oip != ip) {
		err = ENOENT;
		goto out;
	}
	/*
	 * There used to be a check here to make sure you are not removing a
	 * a mounted on dir.  This was no longer correct because iget() does
	 * not cross mount points anymore so the the i_dev fields in the inodes
	 * pointed to by ip and dp will never be different.  There does need
	 * to be a check here though, to eliminate the race between mount and
	 * rmdir (It can also be a race between mount and unlink, if your
	 * kernel allows you to unlink a directory.)
	 */
	if (ITOV(ip)->v_vfsmountedhere != (struct vfs *)0) {
		err = EBUSY;
		goto out;
	}
	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory, or the file in it, otherwise
	 * he may not delete it (unless he's root).  This implements
	 * append-only directories.
	 */
	if ((dp->i_mode & ISVTX) && u.u_uid != 0 &&
	    u.u_uid != dp->i_uid && ip->i_uid != u.u_uid) {
		err = EPERM;
		goto out;
	}
	/*
	 * If the inode being removed is a directory, we must be
	 * sure it only has entries "." and "..".
	 */
	if (rmdir && (ip->i_mode & IFMT) == IFDIR) {
		if ((ip->i_nlink != 2) || !dirempty(ip, dp->i_number)) {
			err = ENOTEMPTY;
			goto out;
		}
	}
	/*
	 * Remove the cache'd entry, if any.
	 */
	dnlc_remove(ITOV(dp), namep);
	/*
	 * If the entry isn't the first in the directory, we must reclaim
	 * the space of the now empty record by adding the record size
	 * to the size of the previous entry.
	 */
	ep = slot.ep;
	if ((slot.offset & (DIRBLKSIZ - 1)) == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ep->d_ino = 0;
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		pep = (struct direct *)((char *)ep - slot.size);
		pep->d_reclen += ep->d_reclen;
	}
	err = fbwrite(slot.fbp, dp);
	slot.fbp = NULL;
	dp->i_flag |= IUPD|ICHG;
	ip->i_flag |= ICHG;
	if (err)
		goto out;
	/*
	 * Now dereference the inode.
	 */
	if (ip->i_nlink > 0) {
		if (rmdir && (ip->i_mode & IFMT) == IFDIR) {
			/*
			 * Decrement by 2 because we're trashing the "."
			 * entry as well as removing the entry in dp.
			 * Clear the inode, but there may be other hard
			 * links so don't free the inode.
			 * Decrement the dp linkcount because we're
			 * trashing the ".." entry.
			 */
			ip->i_nlink -= 2;
			dp->i_nlink--;
			dnlc_remove(ITOV(ip), ".");
			dnlc_remove(ITOV(ip), "..");
			(void) itrunc(ip, (u_long)0);
		} else {
			ip->i_nlink--;
		}
	}
out:
	if (ip)
		iput(ip);
	if (slot.fbp)
		fbrelse(slot.fbp, S_OTHER);
	iunlock(dp);
	return (err);
}

/*
 * Return buffer with contents of block "offset"
 * from the beginning of directory "ip".  If "res"
 * is non-zero, fill it in with a pointer to the
 * remaining space in the directory.
 */
static
blkatoff(ip, offset, res, fbpp)
	struct inode *ip;
	off_t offset;
	char **res;
	struct fbuf **fbpp;
{
	register struct fs *fs;
	struct fbuf *fbp;
	daddr_t lbn;
	u_int bsize;
	int err;

	fs = ip->i_fs;
	lbn = lblkno(fs, offset);
	bsize = blksize(fs, ip, lbn);
	err = fbread(ITOV(ip), (u_int)(offset & fs->fs_bmask), bsize, S_OTHER,
	    &fbp);
	if (err) {
		*fbpp = (struct fbuf *)NULL;
		return (err);
	}
	if (res)
		*res = fbp->fb_addr + blkoff(fs, offset);
	*fbpp = fbp;
	return (0);
}

/*
 * Do consistency checking:
 *	record length must be multiple of 4
 *	entry must fit in rest of its DIRBLKSIZ block
 *	record must be large enough to contain entry
 *	name is not longer than MAXNAMLEN
 * if dirchk is on:
 *	name must be as long as advertised, and null terminated
 * NOTE: record length must not be zero (should be checked previously).
 */
static
dirmangled(dp, ep, entryoffsetinblock, offset)
	register struct inode *dp;
	register struct direct *ep;
	int entryoffsetinblock;
	off_t offset;
{
	register int i;

	i = DIRBLKSIZ - (entryoffsetinblock & (DIRBLKSIZ - 1));
	if ((ep->d_reclen & 0x3) != 0 || ep->d_reclen > i ||
	    ep->d_reclen < DIRSIZ(ep) || ep->d_namlen > MAXNAMLEN ||
	    dirchk && dirbadname(ep->d_name, (int)ep->d_namlen)) {
		dirbad(dp, "mangled entry", offset);
		return (1);
	}
	return (0);
}

static
dirbad(ip, how, offset)
	struct inode *ip;
	char *how;
	off_t offset;
{

	printf("%s: bad dir ino %d at offset %d: %s\n",
	    ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
}

static
dirbadname(sp, l)
	register char *sp;
	register int l;
{

	while (l--) {			/* check for nulls */
		if (*sp++ == '\0') {
			return (1);
		}
	}
	return (*sp);			/* check for terminating null */
}

/*
 * Check if a directory is empty or not.
 *
 * Using a struct dirtemplate here is not precisely
 * what we want, but better than using a struct direct.
 *
 * N.B.: does not handle corrupted directories.
 */
static
dirempty(ip, parentino)
	register struct inode *ip;
	ino_t parentino;
{
	register off_t off;
	struct dirtemplate dbuf;
	register struct direct *dp = (struct direct *)&dbuf;
	int err, count;
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		err = rdwri(UIO_READ, ip, (caddr_t)dp, MINDIRSIZ,
		    off, 1, &count);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (err || count != 0 || dp->d_reclen == 0)
			return (0);
		/* skip empty entries */
		if (dp->d_ino == 0)
			continue;
		/* accept only "." and ".." */
		if (dp->d_namlen > 2)
			return (0);
		if (dp->d_name[0] != '.')
			return (0);
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (dp->d_namlen == 1)
			continue;
		if (dp->d_name[1] == '.' && dp->d_ino == parentino)
			continue;
		return (0);
	}
	return (1);
}

#define	RENAME_IN_PROGRESS	0x01
#define	RENAME_WAITING		0x02

/*
 * Check if source directory is in the path of the target directory.
 * Target is supplied locked, source is unlocked.
 * The target is always relocked before returning.
 */
static
dircheckpath(source, target)
	struct inode *source, *target;
{
	struct fbuf *fbp;
	struct dirtemplate *dirp;
	register struct inode *ip;
	struct inode *tip;
	static char serialize_flag = 0;
	ino_t dotdotino;
	int err = 0;

	/*
	 * If two renames of directories were in progress at once, the partially
	 * completed work of one dircheckpath could be invalidated by the other
	 * rename.  To avoid this, all directory renames in the system are
	 * serialized.
	 */
	while (serialize_flag & RENAME_IN_PROGRESS) {
		serialize_flag |= RENAME_WAITING;
		(void) sleep((caddr_t)&serialize_flag, PINOD);
	}
	serialize_flag = RENAME_IN_PROGRESS;
	ip = target;
	if (ip->i_number == source->i_number) {
		err = EINVAL;
		goto out;
	}
	if (ip->i_number == ROOTINO) {
		goto out;
	}
	fbp = NULL;
	for (;;) {
		if (((ip->i_mode & IFMT) != IFDIR) || (ip->i_nlink == 0) ||
		    (ip->i_size < sizeof (struct dirtemplate))) {
			dirbad(ip, "bad size, unlinked or not dir", (off_t)0);
			err = ENOTDIR;
			break;
		}
		err = blkatoff(ip, (off_t)0, (char **)&dirp, &fbp);
		if (err) {
			break;
		}
		if (dirp->dotdot_namlen != 2 ||
		    dirp->dotdot_name[0] != '.' ||
		    dirp->dotdot_name[1] != '.') {
			dirbad(ip, "mangled .. entry", (off_t)0);
			err = ENOTDIR;
			break;
		}
		dotdotino = dirp->dotdot_ino;
		if (dotdotino == source->i_number) {
			err = EINVAL;
			break;
		}
		if (dotdotino == ROOTINO) {
			break;
		}
		if (fbp) {
			fbrelse(fbp, S_OTHER);
			fbp = NULL;
		}
		if (ip != target) {
			iput(ip);
		} else {
			iunlock(ip);
		}
		/*
		 * i_dev and i_fs are still valid after iput
		 * This is a race to get ".." just like dirlook.
		 */
		err = iget(ip->i_dev, ip->i_fs, dotdotino, &tip);
		if (err) {
			ip = NULL;
			break;
		}
		ip = tip;
	}
	if (fbp) {
		fbrelse(fbp, S_OTHER);
	}
out:
	/*
	 * unserialize before relocking target to avoid a race
	 */
	if (serialize_flag & RENAME_WAITING) {
		wakeup((caddr_t)&serialize_flag);
	}
	serialize_flag = 0;

	if (ip) {
		if (ip != target) {
			iput(ip);
			/*
			 * Relock target and make sure it has not gone away
			 * while it was unlocked.
			 */
			ilock(target);
			if ((err == 0) && (target->i_nlink == 0)) {
				err = ENOENT;
			}
		}
	}
	return (err);
}
