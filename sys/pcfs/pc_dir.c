#ifndef lint
static	char sccsid[] = "@(#)pc_dir.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_dir.h>
#include <pcfs/pc_node.h>

/*
 * slot structure is used by the directory search routine to return
 * the results of the search.  If the search is successful sl_blkno and
 * sl_offset reflect the disk address of the entry and sl_ep points to
 * the actual entry data in buffer sl_bp. sl_flags is set to whether the
 * entry is dot or dotdot. If the search is unsuccessful sl_blkno and
 * sl_offset points to an empty directory slot if there are any. Otherwise
 * it is set to -1.
 */
struct slot {
	enum {SL_NONE, SL_FOUND, SL_EXTEND}
		sl_status;	/* slot status */
	daddr_t sl_blkno;	/* disk block number which has entry */
	int sl_offset;		/* offset of entry within block */
	struct buf *sl_bp;	/* buffer containing entry data */
	struct pcdir *sl_ep;	/* pointer to entry data */
	int sl_flags;		/* flags (see below) */
};
#define	SL_DOT		1	/* entry point to self */
#define	SL_DOTDOT	2	/* entry points to parent */

/*
 * Lookup a name in a directory. Return a pointer to the pc_node
 * which represents the entry.
 */
int
pc_dirlook(dp, namep, pcpp)
	register struct pcnode *dp;	/* parent directory */
	char *namep;			/* name to lookup */
	struct pcnode **pcpp;		/* result */
{
	struct vnode * vp;
	struct slot slot;
	int error;

PCFSDEBUG(7)
printf("pc_dirlook (dp %x name %s pcpp %x)\n", dp, namep, pcpp);

	if (!(dp->pc_entry.pcd_attr & PCA_DIR)) {
		return (ENOTDIR);
	}
	/*
	 * The root directory does not have "." and ".." entries,
	 * so they are faked here.
	 */
	vp = PCTOV(dp);
	if (vp->v_flag & VROOT) {
		if (bcmp(namep, ".", 2) == 0 || bcmp(namep, "..", 3) == 0) {
			*pcpp = pc_getnode(VFSTOPCFS(vp->v_vfsp),
				(daddr_t)0, 0, (struct pcdir *)0);
			return (0);
		}
	}
	error = pc_findentry(dp, namep, &slot);
	if (error == 0) {
		*pcpp = pc_getnode(VFSTOPCFS(vp->v_vfsp),
			slot.sl_blkno, slot.sl_offset, slot.sl_ep);
		brelse(slot.sl_bp);
	} else if (error == EINVAL) {
		error = ENOENT;
	}
	return (error);
}

/*
 * Enter a name in a directory.
 */
pc_direnter(dp, namep, vap, pcpp)
	register struct pcnode *dp;	/* directory to make entry in */
	register char *namep;		/* name of entry */
	struct vattr *vap;		/* attributes of new entry */
	struct pcnode **pcpp;
{
	register struct pcfs *fsp;
	register int error;
	struct slot slot;
	struct vnode * vp;

PCFSDEBUG(7)
printf("pcdirentry(dp %x, name %s, vap %x, pcpp %x\n", dp, namep, vap, pcpp);

	if (pcpp != NULL)
		*pcpp = NULL;

	if (!(dp->pc_entry.pcd_attr & PCA_DIR)) {
		return (ENOTDIR);
	}
	/*
	 * If name is "." or "..", just look it up.
	 */
	if (*namep == '.') {
		if (pcpp) {
			error = pc_dirlook(dp, namep, pcpp);
			if (error)
				return (error);
		}
		return (EEXIST);
	}
	if (dp->pc_entry.pcd_attr & (PCA_HIDDEN | PCA_SYSTEM)) {
		return (EPERM);
	}
	/*
	 * Make sure directory has not been removed while fs was unlocked.
	 */
	if (dp->pc_entry.pcd_filename[0] == PCD_ERASED) {
		return (ENOENT);
	}
	vp = PCTOV(dp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	error = pc_findentry(dp, namep, &slot);
	if (error == 0) {
		if (pcpp) {
			*pcpp =
			    pc_getnode(fsp, slot.sl_blkno, slot.sl_offset,
				slot.sl_ep);
			error = EEXIST;
		}
		brelse(slot.sl_bp);
	} else if (error == ENOENT) {
		struct pcdir direntry;

		/*
		 * The entry does not exist. Check write permission in
		 * directory to see if entry can be created.
		 */
		if (dp->pc_entry.pcd_attr & PCA_RDONLY) {
			return (EPERM);
		}
		error = 0;
		/*
		 * Make sure there is a slot.
		 */
		if (slot.sl_status == SL_NONE)
			panic("pc_direnter: no slot\n");
		if (slot.sl_status == SL_EXTEND) {
			/*
			 * There is no slot in the directory, so try to
			 * extend it.
			 */
			error =
			    pc_balloc(dp,
				pc_lblkno(fsp, slot.sl_offset),
				&slot.sl_blkno);
			if (error)
				return (error);
			slot.sl_offset = 0;
		}
		/*
		 * Make an entry from the supplied attributes.
		 */
		error = pc_makedirentry(dp, namep, vap, &direntry);
		if (error)
			return (error);
		/*
		 * Get a pcnode for the new entry.
		 */
		*pcpp =
		    pc_getnode(fsp, slot.sl_blkno, slot.sl_offset, &direntry);

		/*
		 * Write out the new entry in the parent directory.
		 */
		error = pc_syncfat(fsp);
		if (! error) {
			error = pc_nodeupdate(*pcpp);
		}

		/*
		 * %% why not update the mtime in dp? because '..' update
		 * problem.  (dp could be pointing to the '..' of a child
		 * of the real directory, instead of the real directory itself.
		 * Then we end up updating '..' entry.
		 */
	}
	return (error);
}

/*
 * Template for "." and ".." directory entries.
 */
static struct {
	struct pcdir t_dot;		/* dot entry */
	struct pcdir t_dotdot;		/* dotdot entry */
} dirtemplate = {
	{
		".       ",
		"   ",
		PCA_DIR,
		0,
		"",
		{0, 0},
		0,
		0
	},
	{
		"..      ",
		"   ",
		PCA_DIR,
		0,
		"",
		{0, 0},
		0,
		0
	}
};

/*
 * Convert an attributes structure into a pc directory entry.
 */
int
pc_makedirentry(dp, namep, vap, ep)
	struct pcnode *dp;		/* parent directory */
	char *namep;			/* name of new node */
	register struct vattr *vap;	/* attributes of new node */
	register struct pcdir *ep;	/* new directory entry */
{
	struct vnode * vp;
	int error;

	bzero((caddr_t)ep, sizeof (struct pcdir));
	error = pc_parsename(namep, ep->pcd_filename, ep->pcd_ext);
	if (error)
		return (error);
	pc_tvtopct(&time, &ep->pcd_mtime);
	ep->pcd_size = 0;
	ep->pcd_attr = 0;
	if ((vap->va_mode & 0222) == 0)
		ep->pcd_attr |=  PCA_RDONLY;
	if (vap->va_type == VDIR) {
		register struct buf *bp;
		register struct pcfs *fsp;
		short cn;

		vp = PCTOV(dp);
		fsp = VFSTOPCFS(vp->v_vfsp);
		ep->pcd_attr |= PCA_DIR;
		/*
		 * Make dot and dotdot entries for a new directory.
		 */
		switch (cn = pc_alloccluster(fsp)) {
		case PCF_FREECLUSTER:
			return (ENOSPC);
		case PCF_ERRORCLUSTER:
			return (EIO);
		}
		bp = bread(fsp->pcfs_devvp, pc_cldaddr(fsp, cn),
			fsp->pcfs_clsize);
		if (bp->b_flags & (B_ERROR | B_INVAL)) {
			pc_setcluster(fsp, cn, PCF_FREECLUSTER);
			if (bp->b_flags & B_ERROR) {
PCFSDEBUG(1)
printf("pc_makedirentry ");
				pc_diskchanged(fsp);
				return (EIO);
			}
			brelse(bp);
			return (EIO);
		}
		bp->b_flags |= B_NOCACHE;		/* don't cache */
		dirtemplate.t_dot.pcd_scluster = ep->pcd_scluster = htols(cn);
		dirtemplate.t_dotdot.pcd_scluster = dp->pc_entry.pcd_scluster;
		dirtemplate.t_dot.pcd_mtime =
		    dirtemplate.t_dotdot.pcd_mtime = ep->pcd_mtime;
		bcopy((caddr_t)&dirtemplate,
		    bp->b_un.b_addr, sizeof (dirtemplate));
		bwrite(bp);
		if (bp->b_flags & B_ERROR) {
			pc_diskchanged(fsp);
			return (EIO);
		}
	} else {
		ep->pcd_scluster = 0;
	}
	return (0);
}

/*
 * Remove a name from a directory.
 */
pc_dirremove(dp, namep, type)
	register struct pcnode *dp;
	char *namep;
	enum vtype type;
{
	struct slot slot;
	register struct pcnode *pcp;
	register int error;
	struct vnode * vp;

	vp = PCTOV(dp);
	if (!(dp->pc_entry.pcd_attr & PCA_DIR)) {
		return (ENOTDIR);
	}
	if (dp->pc_entry.pcd_attr & (PCA_RDONLY | PCA_HIDDEN | PCA_SYSTEM)) {
		return (EPERM);
	}
	error = pc_findentry(dp, namep, &slot);
	if (error)
		return (error);
	if (slot.sl_flags == SL_DOT) {
		error = EINVAL;
	} else if (slot.sl_flags == SL_DOTDOT) {
		error = ENOTEMPTY;
	} else {
		pcp =
		    pc_getnode(VFSTOPCFS(vp->v_vfsp),
			slot.sl_blkno, slot.sl_offset, slot.sl_ep);
	}
	if (error) {
		brelse(slot.sl_bp);
		return (error);
	}
	if (type == VDIR) {
		if (pcp->pc_entry.pcd_attr & PCA_DIR) {
			if (!pc_dirempty(pcp)) {
				error = ENOTEMPTY;
			}
		} else {
			error = ENOTDIR;
		}
	} else {
		if (pcp->pc_entry.pcd_attr & PCA_DIR)
			error = EISDIR;
	}
	if (error == 0) {
		/*
		 * Mark the in core node and on disk entry
		 * as removed. The slot may then be reused.
		 * The files clusters will be deallocated
		 * when the last reference goes away.
		 */
		slot.sl_ep->pcd_filename[0] =
		    pcp->pc_entry.pcd_filename[0] = PCD_ERASED;
		pcp->pc_eblkno = -1;
		bwrite(slot.sl_bp);
		if (slot.sl_bp->b_flags & B_ERROR) {
			pc_diskchanged(VFSTOPCFS(vp->v_vfsp));
			return (EIO);
		} else if (type == VDIR) { /* %% delete '.', '..' now so that */
			(void) pc_truncate(pcp, 0L);
			if (pc_syncfat(VFSTOPCFS(vp->v_vfsp))) {
				return (EIO);
			}
		}

	} else {
		brelse(slot.sl_bp);
	}
	/*
	 * If this is the only reference release it now.
	 */
#ifdef notdef
	if (PCTOV(pcp)->v_count == 1) {
		PCTOV(pcp)->v_count = 0;
		if (!(pcp->pc_flags & PC_RELE_PEND))
			pc_rele(pcp);
	} else
#endif notdef
		VN_RELE(PCTOV(pcp));

	return (error);
}

/*
 * Ascertain whether a directory is empty.
 */
int
pc_dirempty(pcp)
	register struct pcnode *pcp;
{
	struct buf *bp;
	struct pcdir *ep;
	register long offset;
	register int boff;
	register char c;
	int error;
	struct vnode * vp;

	vp = PCTOV(pcp);
	bp = NULL;

	for (offset = 0; /* */; offset += sizeof (struct pcdir)) {

		/*
		 * If offset is on a block boundary,
		 * read in the next directory block.
		 * Release previous if it exists.
		 */
		boff = pc_blkoff(VFSTOPCFS(vp->v_vfsp), offset);
		if (boff == 0 || bp == NULL || boff >= bp->b_bcount) {
			if (bp != NULL)
				brelse(bp);
			if (error = pc_blkatoff(pcp, offset, &bp, &ep)) {
				return (error);
			}
		}
		c = ep->pcd_filename[0];
		if (c == PCD_UNUSED)
			break;
		if ((c != '.') && (c != PCD_ERASED)) {
			brelse(bp);
			return (0);
		}
		ep++;
	}
	if (bp != NULL)
		brelse(bp);
	return (1);
}

/*
 * Rename a file within a directory.
 * Target cannot exist (for now).
 */
pc_rename(dp, snm, tnm)
	register struct pcnode *dp;	/* parent directory */
	char *snm;			/* source file name */
	char *tnm;			/* target file name */
{
	register struct pcnode *pcp;	/* pcnode we are trying to rename */
	struct slot slot;
	char tfname[PCFNAMESIZE];
	char tfext[PCFEXTSIZE];
	register int error;
	struct vnode * vp;

	vp = PCTOV(dp);
PCFSDEBUG(6)
printf("pc_rename(0x%x, %s, %s)\n", dp, snm, tnm);
	if (!(dp->pc_entry.pcd_attr & PCA_DIR)) {
		return (ENOTDIR);
	}
	/*
	 * No dot or dotdot.
	 */
	if (*snm == '.' || *tnm == '.')
		return (EINVAL);
	/*
	 * Get the source node.
	 */
	error = pc_findentry(dp, snm, &slot);
	if (error) {
		return (error);
	}
	pcp = pc_getnode(VFSTOPCFS(vp->v_vfsp),
	    slot.sl_blkno, slot.sl_offset, slot.sl_ep);

	brelse(slot.sl_bp);
	/*
	 * Parse the target name.
	 */
	error = pc_parsename(tnm, tfname, tfext);
	if (error)
		goto out;
	/*
	 * See if source and target names are different.
	 */
	if (bcmp(tfname, pcp->pc_entry.pcd_filename, PCFNAMESIZE) == 0 &&
	    bcmp(tfext, pcp->pc_entry.pcd_ext, PCFEXTSIZE) == 0) {
		goto out;
	}
	/*
	 * see if the target exists
	 */
	error = pc_findentry(dp, tnm, &slot);
	if (error == 0) {
		/*
		 * Target exists.
		 */
		brelse(slot.sl_bp);
		error = EEXIST;
	} else if (error == ENOENT) {
		/*
		 * Rename the source.
		 */
		bcopy(tfname, pcp->pc_entry.pcd_filename, PCFNAMESIZE);
		bcopy(tfext, pcp->pc_entry.pcd_ext, PCFEXTSIZE);
		if (error = pc_nodeupdate(pcp)) {
			return (error);
		}
	}
out:
	/*
	 * If this is the only reference release it now.
	 */
#ifdef notdef
	if (PCTOV(pcp)->v_count == 1) {
		PCTOV(pcp)->v_count = 0;
		if (!(pcp->pc_flags & PC_RELE_PEND)) {
			pc_rele(pcp);
		}
	} else
#endif notdef
		VN_RELE(PCTOV(pcp));

	return (error);
}

/*
 * Search a directory for an entry.
 * The directory should be locked as this routine
 * will sleep on I/O while searching.
 */
static int
pc_findentry(dp, namep, slotp)
	register struct pcnode *dp;	/* parent directory */
	char *namep;			/* name to lookup */
	struct slot *slotp;
{
	register long offset;
	struct pcdir *ep;
	register int boff;
	int error;
	char fname[PCFNAMESIZE];
	char fext[PCFEXTSIZE];
	struct vnode * vp;

	vp = PCTOV(dp);
PCFSDEBUG(7)
printf("pc_findentry: looking for %s in dir 0x%x\n", namep, dp);
	slotp->sl_status = SL_NONE;
	if (!(dp->pc_entry.pcd_attr & PCA_DIR)) {
		return (ENOTDIR);
	}
	/*
	 * Verify that the dp is still valid on the disk
	 */
	error = pc_verify(dp);
	if (error)
		return (error);
	error = pc_parsename(namep, fname, fext);
	if (error)
		return (error);
	slotp->sl_bp = NULL;
	for (offset = 0; /* */; ep++, offset += sizeof (struct pcdir)) {
		/*
		 * If offset is on a block boundary,
		 * read in the next directory block.
		 * Release previous if it exists.
		 */
		boff = pc_blkoff(VFSTOPCFS(vp->v_vfsp), offset);
		if (boff == 0 || slotp->sl_bp == NULL ||
		    boff >= slotp->sl_bp->b_bcount) {
			if (slotp->sl_bp != NULL) {
				brelse(slotp->sl_bp);
				slotp->sl_bp = NULL;
			}
			error = pc_blkatoff(dp, offset, &slotp->sl_bp, &ep);
			if (error == ENOENT && slotp->sl_status == SL_NONE) {
				slotp->sl_status = SL_EXTEND;
				slotp->sl_offset = offset;
			}
			if (error)
				return (error);
		}
		if ((ep->pcd_filename[0] == PCD_UNUSED) ||
		    (ep->pcd_filename[0] == PCD_ERASED)) {
			/*
			 * note empty slots, in case name is not found
			 */
			if (slotp->sl_status == SL_NONE) {
				slotp->sl_status = SL_FOUND;
				slotp->sl_blkno = slotp->sl_bp->b_blkno;
				slotp->sl_offset = boff;
			}
			/*
			 * If unused we've hit the end of the directory
			 */
			if (ep->pcd_filename[0] == PCD_UNUSED)
				break;
			else
				continue;
		}
		/*
		 * Hidden files do not participate in the search
		 */
		if (ep->pcd_attr & (PCA_HIDDEN | PCA_SYSTEM))
			continue;
		if ((bcmp(fname, ep->pcd_filename, PCFNAMESIZE) == 0) &&
		    (bcmp(fext, ep->pcd_ext, PCFEXTSIZE) == 0)) {
			/*
			 * found the file
			 */
			if (fname[0] == '.') {
				if (fname[1] == '.')
					slotp->sl_flags = SL_DOTDOT;
				else
					slotp->sl_flags = SL_DOT;
			} else {
				slotp->sl_flags = 0;
			}
			slotp->sl_blkno = slotp->sl_bp->b_blkno;
			slotp->sl_offset = boff;
			slotp->sl_ep = ep;
			return (0);
		}
	}
	if (slotp->sl_bp != NULL) {
		brelse(slotp->sl_bp);
		slotp->sl_bp = NULL;
	}
	return (ENOENT);
}

/*
 * Obtain the block at offset "offset" in file pcp.
 */
int
pc_blkatoff(pcp, offset, bpp, epp)
	register struct pcnode *pcp;
	register long offset;
	struct buf **bpp;
	struct pcdir **epp;
{
	register struct pcfs *fsp;
	register struct buf *bp;
	int size;
	int error;
	daddr_t bn;

	fsp = VFSTOPCFS(PCTOV(pcp)->v_vfsp);
	size = pc_blksize(fsp, pcp, offset);
	if (pc_blkoff(fsp, offset) >= size) {
		return (ENOENT);
	}
	error = pc_bmap(pcp, pc_lblkno(fsp, offset), &bn, (daddr_t *)0);
	if (error)
		return (error);
	bp = bread(fsp->pcfs_devvp, bn, size);
	if (bp->b_flags & (B_ERROR | B_INVAL)) {
PCFSDEBUG(1)
printf("pc_blkatoff ");
		if (bp->b_flags & B_ERROR)
			pc_diskchanged(fsp);
		else
			brelse(bp);
		return (EIO);
	}
	bp->b_flags |= B_NOCACHE;		/* don't cache */
	if (epp) {
		*epp =
		    (struct pcdir *)(bp->b_un.b_addr + pc_blkoff(fsp, offset));
	}
	*bpp = bp;
	return (0);
}

/*
 * Parse user filename into the pc form of "filename.extension".
 * If names are too long for the format they are truncated silently.
 * Tests for characters that are invalid in PCDOS and converts to upper case.
 */
static int
pc_parsename(namep, fnamep, fextp)
	register char *namep;
	register char *fnamep;
	register char *fextp;
{
	register int n;
	register char c;

	n = PCFNAMESIZE;
	c = *namep++;
	if (c == 0)
		return (EINVAL);
	if (c == '.') {
		/*
		 * check for "." and "..".
		 */
		*fnamep++ = c;
		n--;
		if (c = *namep++) {
			if ((c != '.') || (c = *namep)) /* ".x" or "..x" */
				return (EINVAL);
			*fnamep++ = '.';
			n--;
		}
	} else {
		/*
		 * filename up to '.'
		 */
		do {
			if (n-- > 0) {
				c = toupper(c);
				if (!pc_validchar(c))
					return (EINVAL);
				*fnamep++ = c;
			}
		} while ((c = *namep++) && c != '.');
	}
	while (n-- > 0) {		/* fill with blanks */
		*fnamep++ = ' ';
	}
	/*
	 * remainder is extension
	 */
	n = PCFEXTSIZE;
	if (c == '.') {
		while ((c = *namep++) && n--) {
			c = toupper(c);
			if (!pc_validchar(c))
				return (EINVAL);
			*fextp++ = c;
		}
	}
	while (n-- > 0) {		/* fill with blanks */
		*fextp++ = ' ';
	}
	return (0);
}
