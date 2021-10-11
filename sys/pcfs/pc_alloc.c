#ifndef lint
static	char sccsid[] = "@(#)pc_alloc.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Routines to allocate and deallocate data blocks on the disk
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_dir.h>
#include <pcfs/pc_node.h>

/*
 * internal routines
 */
static short pc_getcluster();		/* get the next cluster number */

/*
 * Convert file logical block (cluster) numbers to disk block numbers.
 * Also return read ahead block if asked for.
 * Used for reading only. Use pc_balloc for writing.
 */
int
pc_bmap(pcp, lcn, dbnp, rabnp)
	register struct pcnode *pcp;	/* pcnode for file */
	register daddr_t lcn;		/* logical cluster no */
	daddr_t *dbnp;			/* ptr to phys block no */
	daddr_t *rabnp;			/* ptr to read ahead phys block */
					/* may be zero if not wanted */
{
	register struct pcfs *fsp;	/* pcfs that file is in */
	struct vnode * vp;

PCFSDEBUG(7)
printf("pc_bmap: pcp=0x%x, lcn=%d\n", pcp, lcn);

	vp = PCTOV(pcp);
	fsp = VFSTOPCFS(vp->v_vfsp);
	if (lcn < 0)
		return (ENOENT);
	if (vp->v_flag & VROOT) {
		register daddr_t lbn;	/* logical (disk) block number */

		lbn = pc_cltodb(fsp, lcn);
		if (lbn >= fsp->pcfs_rdirsec)
			return (ENOENT);
		*dbnp = fsp->pcfs_rdirstart + lbn;
		if (rabnp != (daddr_t *)0) {
			if ((lbn + 1) < fsp->pcfs_rdirsec)
				*rabnp = *dbnp + 1;
			else
				*rabnp = (daddr_t)0;
		}
	} else {
		register short cn;	/* current cluster number */
		register short ncn;	/* next cluster number */

		if (lcn >= fsp->pcfs_ncluster)
			return (ENOENT);
		if (vp->v_type == VREG &&
		    (pcp->pc_size == 0 ||
		    lcn >= howmany(pcp->pc_size, fsp->pcfs_clsize))) {
			return (ENOENT);
		}
		ncn = pcp->pc_scluster;
		do {
			cn = ncn;
			if (!pc_validcl(fsp, cn)) {
				if (cn >= PCF_LASTCLUSTER &&
				    vp->v_type == VDIR) {
					return (ENOENT);
				} else {
					pc_badfs(fsp);
					return (EIO);
				}
			}
			ncn = pc_getcluster(fsp, cn);
		} while (--lcn >= 0);
		*dbnp = pc_cldaddr(fsp, cn);
		if (rabnp != (daddr_t *)0) {
			if (ncn >= PCF_LASTCLUSTER) {
				*rabnp = (daddr_t)0;
			} else {
				if (!pc_validcl(fsp, ncn)) {
					pc_badfs(fsp);
					return (EIO);
				}
				*rabnp = pc_cldaddr(fsp, ncn);
			}
		}
	}
	return (0);
}

/*
 * Allocate file logical blocks (clusters).
 * Return disk address of corresponidng cluster.
 */
int
pc_balloc(pcp, lcn, dbnp)
	register struct pcnode *pcp;	/* pcnode for file */
	register daddr_t lcn;		/* logical cluster no */
	daddr_t *dbnp;			/* ptr to phys block no */
{
	register struct pcfs *fsp;	/* pcfs that file is in */
	struct vnode * vp;

PCFSDEBUG(7)
printf("pc_balloc: pcp=0x%x, lcn=%d\n", pcp, lcn);

	vp = PCTOV(pcp);
	fsp = VFSTOPCFS(vp -> v_vfsp);

	if (lcn < 0) {
		return (EFBIG);
	}

	if (vp->v_flag & VROOT) {
		register daddr_t lbn;

		lbn = pc_cltodb(fsp, lcn);
		if (lbn >= fsp->pcfs_rdirsec)
			return (ENOSPC);
		*dbnp = fsp->pcfs_rdirstart + lbn;
	} else {
		register short cn;	/* current cluster number */
		register short ncn;	/* next cluster number */

		if (lcn >= fsp->pcfs_ncluster)
			return (ENOSPC);
		if ((vp->v_type == VREG && pcp->pc_size == 0) ||
		    (vp->v_type == VDIR && lcn == 0)) {
			switch (cn = pc_alloccluster(fsp)) {
			case PCF_FREECLUSTER:
				return (ENOSPC);
			case PCF_ERRORCLUSTER:
				return (EIO);
			}
			pcp->pc_scluster = cn;
		} else {
			cn = pcp->pc_scluster;
			if (!pc_validcl(fsp, cn)) {
				pc_badfs(fsp);
				return (EIO);
			}
		}
		while (lcn-- > 0) {
			ncn = pc_getcluster(fsp, cn);
			if (ncn >= PCF_LASTCLUSTER) {
				/*
				 * Extend file (no holes).
				 */
				switch (ncn = pc_alloccluster(fsp)) {
				case PCF_FREECLUSTER:
					return (ENOSPC);
				case PCF_ERRORCLUSTER:
					return (EIO);
				}
				pc_setcluster(fsp, cn, ncn);
			} else if (!pc_validcl(fsp, ncn)) {
				pc_badfs(fsp);
				return (EIO);
			}
			cn = ncn;
		}
		*dbnp = pc_cldaddr(fsp, cn);
	}
	return (0);
}

/*
 * Free file cluster chain after the first skipcl clusters.
 */
int
pc_bfree(pcp, skipcl)
	struct pcnode *pcp;
	register short skipcl;
{
	register struct pcfs *fsp;
	register short cn;
	register short ncn;
	register int n;
	struct vnode * vp;

	vp = PCTOV(pcp);
	if (vp -> v_flag & VROOT) {
		panic("pc_bfree");
	}

PCFSDEBUG(7)
printf("pc_bfree: pcp=0x%x, after first %d clusters\n", pcp, skipcl);

	if (pcp -> pc_size == 0 && vp -> v_type == VREG) {
		return (0);
	}
	fsp = VFSTOPCFS(vp -> v_vfsp);
	if (vp -> v_type == VREG) {
		n = howmany(pcp->pc_size, fsp->pcfs_clsize);
		if (n > fsp->pcfs_ncluster) {
			pc_badfs(fsp);
			return (EIO);
		}
	} else {
		n = fsp->pcfs_ncluster;
	}
	cn = pcp->pc_scluster;
	if (skipcl == 0) {
		pcp->pc_scluster = PCF_LASTCLUSTER;
	}
	while (n--) {
		if (!pc_validcl(fsp, cn)) {
			pc_badfs(fsp);
			return (EIO);
		}
		ncn = pc_getcluster(fsp, cn);
		if (skipcl == 0)
			pc_setcluster(fsp, cn, PCF_FREECLUSTER);
		else
			skipcl--;
		if (ncn >= PCF_LASTCLUSTER && vp -> v_type == VDIR)
			break;
		cn = ncn;
	}
	return (0);
}

/*
 * Return the number of free blocks in the filesystem.
 */
int
pc_freeclusters(fsp)
	register struct pcfs *fsp;
{
	register short cn;
	register int free;

	/*
	 * make sure the fat is in core
	 */
	free = 0;
	for (cn = PCF_FIRSTCLUSTER;
	    cn < fsp->pcfs_ncluster + PCF_FIRSTCLUSTER; cn++) {
		if (pc_getcluster(fsp, cn) == PCF_FREECLUSTER) {
			free++;
		}
	}
	return (free);
}

/*
 * Cluster manipulation routines.
 * Fat must be resident.
 */

/*
 * Get the next cluster in the file cluster chain.
 */
static short
pc_getcluster(fsp, cn)
	register struct pcfs *fsp;
	register short cn;		/* current cluster number in chain */
{
	register unsigned char *fp;

	if (fsp->pcfs_fatp == (u_char *)0 || !pc_validcl(fsp, cn))
		panic("pc_getcluster");
	fp = fsp->pcfs_fatp + (cn + (cn >> 1));
	if (cn & 01) {
		cn = ((*fp++ & 0xf0) >> 4);
		cn += (*fp << 4);
	} else {
		cn = *fp++;
		cn += ((*fp & 0x0f) << 8);
	}
	return (cn);
}

/*
 * Set a cluster in the fat to a value.
 */
void
pc_setcluster(fsp, cn, ncn)
	register struct pcfs *fsp;
	register short cn;		/* fat cluster no to be set */
	register short ncn;		/* new value */
{
	register unsigned char *fp;

	if (fsp->pcfs_fatp == (u_char *)0 || !pc_validcl(fsp, cn))
		panic("pc_setcluster");
	fsp->pcfs_flags |= PCFS_FATMOD;
	fp = fsp->pcfs_fatp + (cn + (cn >> 1));
	if (cn & 01) {
		*fp = (*fp & 0x0f) | ((ncn << 4) & 0xf0);
		fp++;
		*fp = (ncn >> 4) & 0xff;
	} else {
		*fp++ = ncn & 0xff;
		*fp = (*fp & 0xf0) | ((ncn >> 8) & 0x0f);
	}
}

/*
 * Allocate a new cluster.
 */
short
pc_alloccluster(fsp)
	register struct pcfs *fsp;	/* file sys to allocate in */
{
	register short cn;

	if (fsp->pcfs_fatp == (u_char *)0)
		panic("pc_addcluster: no fat");
	for (cn = PCF_FIRSTCLUSTER;
	    cn < fsp->pcfs_ncluster + PCF_FIRSTCLUSTER; cn++) {
		if (pc_getcluster(fsp, cn) == PCF_FREECLUSTER) {
			register struct buf *bp;

			pc_setcluster(fsp, cn, PCF_LASTCLUSTER);
			/*
			 * zero the new cluster
			 */
			bp = getblk(fsp->pcfs_devvp,
				pc_cldaddr(fsp, cn), fsp->pcfs_clsize);
			clrbuf(bp);
			bwrite(bp);
			if (bp -> b_flags & B_ERROR) {
				pc_diskchanged(fsp);
				return (PCF_ERRORCLUSTER);
			}
PCFSDEBUG(7)
printf("pc_alloccluster: new cluster = %d\n", cn);
			return (cn);
		}
	}
	return (PCF_FREECLUSTER);
}
