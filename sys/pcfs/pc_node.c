#ifndef lint
static	char sccsid[] = "@(#)pc_node.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_dir.h>
#include <pcfs/pc_node.h>


struct pchead pcfhead[NPCHASH];
struct pchead pcdhead[NPCHASH];

void pvn_vptrunc();

/*
 * fake entry for root directory, since this does not have a parent
 * pointing to it.
 */
static struct pcdir rootentry = {
	"",
	"",
	PCA_DIR,
	0,
	"",
	{0, 0},
	0,
	0
};

static int pc_getentryblock();

void
pc_init()
{
	register struct pchead *hdp, *hfp;
	register int i;

	for (i = 0; i < NPCHASH; i++) {
		hdp = &pcdhead[i];
		hfp = &pcfhead[i];
		hdp->pch_forw =  (struct pcnode *)hdp;
		hdp->pch_back =  (struct pcnode *)hdp;
		hfp->pch_forw =  (struct pcnode *)hfp;
		hfp->pch_back =  (struct pcnode *)hfp;
	}
}

struct pcnode *
pc_getnode(fsp, blkno, offset, ep)
	register struct pcfs *fsp;	/* filsystem for node */
	register daddr_t blkno;		/* phys block no of dir entry */
	register int offset;		/* offset of dir entry in block */
	register struct pcdir *ep;	/* node dir entry */
{
	register struct pcnode *pcp;
	register struct pchead *hp;
	register struct vnode *vp;
	register short scluster;

	if (!(fsp->pcfs_flags & PCFS_LOCKED))
		panic("pc_getnode");
	if (ep == (struct pcdir *)0) {
		ep = &rootentry;
		scluster = 0;
	} else {
		scluster = ltohs(ep->pcd_scluster);
	}
	/*
	 * First look for active nodes.
	 * File nodes are identified by the location (blkno, offset) of
	 * its directory entry.
	 * Directory nodes are identified by the starting cluster number
	 * for the entries.
	 */
	if (ep->pcd_attr & PCA_DIR) {
		hp = &pcdhead[PCDHASH(fsp, scluster)];
		for (pcp = hp->pch_forw;
		    pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if ((fsp == VFSTOPCFS(PCTOV(pcp)->v_vfsp)) &&
			    (scluster == pcp->pc_scluster)) {
				VN_HOLD(PCTOV(pcp));
				return (pcp);
			}
		}
	} else {
		hp = &pcfhead[PCFHASH(fsp, blkno, offset)];
		for (pcp = hp->pch_forw;
		    pcp != (struct pcnode *)hp; pcp = pcp->pc_forw) {
			if ((fsp == VFSTOPCFS(PCTOV(pcp)->v_vfsp)) &&
			    ((pcp->pc_flags & PC_INVAL) == 0) &&
			    (blkno == pcp->pc_eblkno) &&
			    (offset == pcp->pc_eoffset)) {
				VN_HOLD(PCTOV(pcp));
				return (pcp);
			}
		}
	}
	/*
	 * Cannot find node in active list. Allocate memory for a new node
	 * initialize it, and put it on the active list.
	 */
	pcp =
	    (struct pcnode *) kmem_alloc((u_int)sizeof (struct pcnode));
	bzero((caddr_t)pcp, sizeof (struct pcnode));
	vp = (struct vnode *) kmem_alloc(sizeof (struct vnode));
	bzero((caddr_t) vp, sizeof (struct vnode));
	pcp->pc_vn = vp;
	vp->v_count = 1;
	if (ep->pcd_attr & PCA_DIR) {
		vp->v_op = &pcfs_dvnodeops;
		vp->v_type = VDIR;
		if (scluster == 0) {
			vp->v_flag = VROOT;
			blkno = offset = 0;
		}
	} else {
		vp->v_op = &pcfs_fvnodeops;
		vp->v_type = VREG;
		fsp->pcfs_frefs++;
	}
	fsp->pcfs_nrefs++;
	vp->v_data = (caddr_t)pcp;
	vp->v_vfsp = PCFSTOVFS(fsp);
	pcp->pc_entry = *ep;
	pcp->pc_eblkno = blkno;
	pcp->pc_eoffset = offset;
	pcp->pc_scluster = ltohs(ep->pcd_scluster);
	pcp->pc_size = ltohl(ep->pcd_size);
	pcp->pc_flags = 0;
	insque(pcp, hp);
	return (pcp);
}

int
syncpcp(pcp, flags)
	register struct pcnode *pcp;
	int flags;
{
	int err;
	if (PCTOV(pcp)->v_pages == NULL)
		err = 0;
	else
		err = VOP_PUTPAGE(PCTOV(pcp), 0, 0, flags, (struct ucred *)0);

	return (err);
}

void
pc_rele(pcp)
	register struct pcnode *pcp;
{
	register struct pcfs *fsp;
	struct vnode * vp;
	int error;

PCFSDEBUG(6)
printf("pc_rele pcp=0x%x\n", pcp);

	vp = PCTOV(pcp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	if (!(fsp->pcfs_flags & PCFS_LOCKED) || vp -> v_count != 0 ||
	    pcp->pc_flags & PC_RELE_PEND) {
		panic("pc_rele");
	}

	if (! (vp->v_type == VDIR || pcp->pc_flags & PC_INVAL)) {
		/*
		 * If the file was removed while active it may be safely
		 * truncated now.
		 */
		if (pcp->pc_flags & (PC_MOD | PC_CHG)) { /* %% ?? right place */
			if (pc_nodesync(pcp)) {
				return;
			}
		}

		if (pcp->pc_entry.pcd_filename[0] == PCD_ERASED) {

			error = pc_getfat(fsp);
			if (error == 0) {
				(void) pc_truncate(pcp, 0L);
				if (error = pc_syncfat(fsp)) {
					return;
				}
			}
		}
		error = syncpcp(pcp, B_INVAL);
	}
	remque(pcp);
	if ((vp->v_type == VREG) && ! (pcp->pc_flags & PC_INVAL)) {
		fsp->pcfs_frefs--;
	}
	if (fsp->pcfs_frefs == 0) {	/* %% ?? Is this needed? */
		binval(fsp->pcfs_devvp);
	}
	fsp->pcfs_nrefs--;
#ifdef notdef
	if (fsp->pcfs_nrefs < 0 || fsp->pcfs_frefs < 0)
		panic("pc_rele: ref count");
#endif notdef
	if (fsp->pcfs_nrefs < 0 && fsp->pcfs_frefs < 0)
		panic("pc_rele: nrefs & frefs count");
	if (fsp->pcfs_nrefs < 0) {
		panic("pc_rele: nrefs count");
	}
	if (fsp->pcfs_frefs < 0) {
		panic("pc_rele: frefs count");
	}

	kmem_free((caddr_t) PCTOV(pcp), sizeof (struct vnode));
	kmem_free((caddr_t)pcp, (u_int)sizeof (struct pcnode));
	return;
}

/*
 * Mark a pcnode as modified with the current time.
 */
void
pc_mark(pcp)
	register struct pcnode *pcp;
{

	if (PCTOV(pcp)->v_type == VREG) {
		pc_tvtopct(&time, &pcp->pc_entry.pcd_mtime);
		pcp->pc_entry.pcd_scluster = htols(pcp->pc_scluster);
		pcp->pc_entry.pcd_size = htoll(pcp->pc_size);
		pcp->pc_flags |= PC_MOD | PC_CHG;
	}
}

/*
 * Truncate a file to a length.
 * Node must be locked.
 */
int
pc_truncate(pcp, length)
	register struct pcnode *pcp;
	long length;
{
	register struct pcfs *fsp;
	struct vnode * vp;
	u_int off;
	int error = 0;

PCFSDEBUG(6)
printf("pc_truncate pcp=0x%x\n", pcp, length);
	vp = PCTOV(pcp);
	if (pcp->pc_flags & PC_INVAL)
		return (EIO);
	fsp = VFSTOPCFS(vp->v_vfsp);
	/*
	 * directories are always truncated to zero and are not marked
	 */
	if (vp->v_type == VDIR) {
		error = pc_bfree(pcp, (short)0);
		return (error);
	}
	/*
	 * If length is the same as the current size
	 * just mark the pcnode and return.
	 */
	if (length > pcp->pc_size) {
		daddr_t bno;
		u_int size = pcp->pc_size;

		/*
		 * We are extending a file.
		 * Extend it with pc_balloc (no holes).
		 */
		error = pc_balloc(pcp, pc_lblkno(fsp, length), &bno);
		if (error == 0) {
			pcp->pc_size = length;
			error = pc_bzero(pcp, size, (u_int)(length-size),
				IO_SYNC);
		}
		/* ?? What to do when error? */
	} else if (length < pcp->pc_size) {
		/*
		 * We are shrinking a file.
		 * Free blocks after the block that length points to.
		 */
		error = pc_bfree(pcp, (short)howmany(length, fsp->pcfs_clsize));
		off = pc_blkoff(fsp, length);
		if (off == 0) {
			pvn_vptrunc(PCTOV(pcp), (u_int)length, (u_int)0);
		} else {
			pvn_vptrunc(PCTOV(pcp), (u_int)length,
			    (u_int)(fsp->pcfs_clsize - off));
		}

		pcp->pc_size = length;
	}
	pc_mark(pcp);
	return (error);
}

/*
 * Sync all data associated with a file.
 * Flush all the blocks in the buffer cache out to disk, sync the FAT and
 * update the directory entry.
 */
int
pc_nodesync(pcp)
	register struct pcnode *pcp;
{
	register struct pcfs *fsp;
	register int flags;
	int err;
	struct vnode * vp;

PCFSDEBUG(6)
printf("pc_nodesync pcp=0x%x\n", pcp);
	vp = PCTOV(pcp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	flags = pcp->pc_flags;
	pcp->pc_flags &= ~(PC_MOD | PC_CHG);
	if ((flags & (PC_MOD | PC_CHG)) && (vp->v_type == VDIR)) {
		panic("pc_nodesync");
	}
	err = 0;
	if (flags & PC_MOD) {
		/*
		 * Flush all data blocks from buffer cache and
		 * update the fat which points to the data.
		 */
		err = syncpcp(pcp, 0); /* %% ?? how to handle error? */

		if (err) {
			pc_diskchanged (fsp);
			return (EIO);
		}
		err = pc_syncfat(fsp);
		if (err) {
			return (err);
		}
	}
	if (flags & PC_CHG) {
		/*
		 * update the directory entry
		 */
		err = pc_nodeupdate(pcp);
	}
	return (err);
}

/*
 * Update the node's directory entry.
 */
int
pc_nodeupdate(pcp)
	register struct pcnode *pcp;
{
	struct buf *bp;
	int error;
	struct vnode * vp;

	vp = PCTOV(pcp);
	if (vp -> v_flag & VROOT) {
		panic("pc_nodeupdate");
	}
	if (pcp->pc_flags & PC_INVAL)
		return (0);
PCFSDEBUG(6)
printf("pc_nodeupdate pcp=0x%x, bn=%d, off=%d\n",
pcp, pcp->pc_eblkno, pcp->pc_eoffset);

	if (pc_getentryblock(pcp, &bp)) {
		return (0);
	}
	pcp->pc_entry.pcd_scluster = htols(pcp->pc_scluster);
	pcp->pc_entry.pcd_size = htoll(pcp->pc_size);
	*((struct pcdir *)(bp->b_un.b_addr + pcp->pc_eoffset)) = pcp->pc_entry;
	bwrite(bp);
	error = (bp->b_flags & B_ERROR) ? EIO : 0;
	if (error) {
		pc_diskchanged(VFSTOPCFS(vp->v_vfsp));
	}
	return (error);
}

/*
 * Verify that the disk in the drive is the same one that we
 * got the pcnode from.
 * MUST be called with node unlocked.
 */
int
pc_verify(pcp)
	register struct pcnode *pcp;
{
	struct buf *bp;
	register struct pcdir *ep;
	register struct pcfs *fsp;
	int error;
	struct vnode * vp;

PCFSDEBUG(3)
printf("pc_verify pcp=0x%x, bn=%d\n", pcp, pcp->pc_eblkno);
	vp = PCTOV(pcp);
	if (pcp->pc_flags & PC_INVAL) {
		return (EIO);
	}
	if (vp->v_flag & VROOT) {
		return (0);
	}
	fsp = VFSTOPCFS(vp->v_vfsp);
	/*
	 * If we have verified the disk recently, don't bother.
	 * Otherwise, setup the next verify interval and reset the fat
	 * timer to this because we don't have to time out the fat since
	 * the disk has been verified.
	 */
	if (timercmp(&time, &fsp->pcfs_verifytime, <))
		return (0);
	fsp->pcfs_verifytime = time;
	fsp->pcfs_verifytime.tv_sec += PCFS_DISKTIMEOUT;
	fsp->pcfs_fattime = fsp->pcfs_verifytime;
	error = pc_getentryblock(pcp, &bp);
	if (error)
		return (error);
	ep = (struct pcdir *)(bp->b_un.b_addr + pcp->pc_eoffset);
	/*
	 * Check that the name extension and attributes are the same.
	 */
	if (bcmp(ep->pcd_filename, pcp->pc_entry.pcd_filename, PCFNAMESIZE) ||
	    bcmp(ep->pcd_ext, pcp->pc_entry.pcd_ext, PCFEXTSIZE) ||
	    ((ep->pcd_attr & PCA_DIR) != (pcp->pc_entry.pcd_attr & PCA_DIR)) ||
	    ((ep->pcd_attr & PCA_DIR) &&
	    (ep->pcd_scluster != pcp->pc_entry.pcd_scluster))) {
		error = EIO;
	}
	if (error) {
PCFSDEBUG(1)
printf("pc_verify ");

		pc_diskchanged (fsp);
	} else {
		brelse(bp);
	}
	return (error);
}

/*
 * Get block for entry.
 */
static int
pc_getentryblock(pcp, bpp)
	register struct pcnode *pcp;
	struct buf **bpp;
{
	register struct pcfs *fsp;

	fsp = VFSTOPCFS(PCTOV(pcp)->v_vfsp);
	if (pcp->pc_eblkno >= fsp->pcfs_datastart ||
	    (pcp->pc_eblkno - fsp->pcfs_rdirstart) <
	    (fsp->pcfs_rdirsec & ~(fsp->pcfs_spcl - 1))) {
		*bpp =
		    bread(fsp->pcfs_devvp, pcp->pc_eblkno, fsp->pcfs_clsize);
	} else {
		*bpp =
		    bread(fsp->pcfs_devvp,
		    pcp->pc_eblkno,
		    (int) (fsp->pcfs_datastart-pcp->pc_eblkno)*PC_SECSIZE);
	}
	if ((*bpp)->b_flags & (B_ERROR | B_INVAL)) {
PCFSDEBUG(1)
printf("pc_getentryblock ");
		if ((*bpp)->b_flags & B_ERROR) {
			pc_diskchanged(fsp);
		} else {
			brelse(*bpp);
		}
		return (EIO);
	}
	(*bpp)->b_flags |= B_NOCACHE;		/* don't cache */
	return (0);
}

/*
 * The disk has been changed!
 */
void
pc_diskchanged(fsp)
	register struct pcfs *fsp;
{
	register struct pcnode *pcp, * npcp;
	register struct pchead *hp;

	/*
	 * Eliminate all pcnodes (dir & file) associated to this fs.
	 * If the node is internal, ie, no references outside of
	 * pcfs itself, then release the associated vnode structure.
	 * Invalidate the in core fat.
	 * Invalidate cached data blocks and blocks waiting for I/O.
	 */
PCFSDEBUG(1)
printf("pc_diskchanged fsp=0x%x\n", fsp);

	printf("I/O error or floppy disk change: possible file damage\n");
	for (hp = pcdhead; hp < &pcdhead[NPCHASH]; hp++) {
		for (pcp = hp->pch_forw;
		    pcp != (struct pcnode *)hp; pcp = npcp) {
			npcp = pcp -> pc_forw;
			if (VFSTOPCFS(PCTOV(pcp)->v_vfsp) == fsp) {
				remque(pcp);
				PCTOV(pcp)->v_data = NULL;
				if (! (pcp->pc_flags & PC_EXTERNAL)) {
					kmem_free((caddr_t) PCTOV(pcp),
					    sizeof (struct vnode));
				}
				kmem_free((caddr_t) pcp,
					sizeof (struct pcnode));

				fsp -> pcfs_nrefs --;
			}
		}
	}
	for (hp = pcfhead; fsp->pcfs_frefs && hp < &pcfhead[NPCHASH]; hp++) {
		for (pcp = hp->pch_forw; fsp->pcfs_frefs &&
		    pcp != (struct pcnode *)hp; pcp = npcp) {
			npcp = pcp -> pc_forw;
			if (VFSTOPCFS(PCTOV(pcp)->v_vfsp) == fsp) {
				remque(pcp);
				PCTOV(pcp)->v_data = NULL;
				if (! (pcp->pc_flags & PC_EXTERNAL)) {
					kmem_free((caddr_t) PCTOV(pcp),
						sizeof (struct vnode));
				}
				kmem_free((caddr_t) pcp,
					sizeof (struct pcnode));
				fsp -> pcfs_frefs --;
				fsp -> pcfs_nrefs --;
			}
		}
	}
	if (fsp->pcfs_frefs) {
		panic("pc_diskchanged: frefs");
	}
	if (fsp->pcfs_nrefs) {
		panic ("pc_diskchanged: nrefs");
	}
	if (fsp->pcfs_fatp != (u_char *)0) {
		pc_invalfat(fsp);
	} else
		binval(fsp->pcfs_devvp);
	return;
}
