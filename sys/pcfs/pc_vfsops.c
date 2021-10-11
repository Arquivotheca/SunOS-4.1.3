#ifndef lint
static	char sccsid[] = "@(#)pc_vfsops.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/conf.h>
#undef NFSCLIENT
#include <sys/mount.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_fs.h>
#include <pcfs/pc_dir.h>
#include <pcfs/pc_node.h>

/*
 * pcfs vfs operations.
 */
static int pcfs_mount();
static int pcfs_unmount();
static int pcfs_root();
static int pcfs_statfs();
static int pcfs_sync();
#ifdef notdef
static int pcfs_vget();
#endif notdef
extern int pcfs_invalop();
extern int pcfs_badop();

struct vfsops pcfs_vfsops = {
	pcfs_mount,
	pcfs_unmount,
	pcfs_root,
	pcfs_statfs,
	pcfs_sync,
	pcfs_badop,
	pcfs_invalop,	/* vfs_mountroot */
	pcfs_invalop,	/* vfs_swapvp */
};

/*
 * Default device to mount on.
 */

int pcfsdebuglevel = 0;
static int pcinit = 0;

/*
 * pc_mount system call
 */
static int
pcfs_mount(vfsp, path, data)
	struct vfs *vfsp;
	char *path;
	caddr_t data;
{
	int error;
	dev_t dev;
	struct vnode *vp;
	struct pc_args args;
PCFSDEBUG(6)
printf("pcfs_mount ");
	/*
	 * Get arguments
	 */
	error = copyin(data, (caddr_t)&args, sizeof (struct pc_args));
	if (error) {
		return (error);
	}

	/*
	 * Get the device to be mounted on
	 */
	error = lookupname(args.fspec, UIO_USERSPACE, FOLLOW_LINK,
	    (struct vnode **)0, &vp);
	if (error) {
		if (u.u_error == ENOENT)
			return (ENODEV);	/* needs translation */
		return (error);
	}
	if (vp->v_type != VBLK) {
		VN_RELE(vp);
		return (ENOTBLK);
	}
	dev = vp->v_rdev;
	VN_RELE(vp);
	if (major(dev) >= nblkdev) {
		return (ENXIO);
	}
	/*
	 * Mount the filesystem
	 */
	error = pcfs_mountfs(dev, path, vfsp);
	return (error);
}

static struct pcfs *pc_mounttab = NULL;

/*ARGSUSED*/
static int
pcfs_mountfs(dev, path, vfsp)
	dev_t dev;
	char *path;
	struct vfs *vfsp;
{
	register struct pcfs *fsp;
	int error;

	for (fsp = pc_mounttab; fsp != NULL; fsp = fsp->pcfs_nxt) {
		if (fsp->pcfs_devvp->v_rdev == dev) {
			return (EBUSY);
		}
	}
	if (!pcinit) {
		pcinit++;
		pc_init();
	}

	fsp = (struct pcfs *)kmem_alloc((u_int)sizeof (struct pcfs));
	bzero((caddr_t)fsp, sizeof (struct pcfs));
	fsp->pcfs_devvp = bdevvp(dev);
	fsp->pcfs_vfs = vfsp;
	error = pc_getfat(fsp);

	if (error) {
		VN_RELE(fsp->pcfs_devvp);
		kmem_free((caddr_t)fsp, (u_int) sizeof (struct pcfs));
		return (error);
	}
	fsp->pcfs_nxt = pc_mounttab;
	pc_mounttab = fsp;
	vfsp->vfs_fsid.val[0] = (long)dev;
	vfsp->vfs_fsid.val[1] = MOUNT_PC;
	vfsp->vfs_bsize = fsp->pcfs_clsize;
	vfsp->vfs_data = (caddr_t)fsp;
	return (0);
}

/*
 * vfs operations
 */

static int
pcfs_unmount(vfsp)
	register struct vfs *vfsp;
{
	register struct pcfs *fsp, *fsp1;

PCFSDEBUG(6)
printf("pc_unmount\n");
	fsp = VFSTOPCFS(vfsp);
	if (fsp->pcfs_nrefs)
		return (EBUSY);

	/* now there should be no pcp node on pcfhead or pcdhead. */

	if (fsp->pcfs_fatp != (u_char *)0) /* ?? has the fat been synced */
		pc_invalfat(fsp);
	else
		binval(fsp->pcfs_devvp);	/* ?? do we need this */

	if (fsp == pc_mounttab) {
		pc_mounttab = fsp->pcfs_nxt;
	} else {
		for (fsp1 = pc_mounttab; fsp1 != NULL; fsp1 = fsp1->pcfs_nxt)
			if (fsp1->pcfs_nxt == fsp)
				fsp1->pcfs_nxt = fsp->pcfs_nxt;
	}

	VN_RELE(fsp->pcfs_devvp);
	kmem_free((caddr_t)fsp, (u_int) sizeof (struct pcfs));
	return (0);
}

/*
 * find root of pcfs
 */
static int
pcfs_root(vfsp, vpp)
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct pcfs *fsp;
	struct pcnode *pcp;

	fsp = VFSTOPCFS(vfsp);
	PC_LOCKFS(fsp);
	pcp = pc_getnode(fsp, (daddr_t)0, 0, (struct pcdir *)0);
PCFSDEBUG(6)
printf("pcfs_root(0x%x) pcp= 0x%x\n", vfsp, pcp);
	PC_UNLOCKFS(fsp);
	*vpp = PCTOV(pcp);
	pcp->pc_flags |= PC_EXTERNAL;
	return (0);
}

/*
 * Get file system statistics.
 */
static int
pcfs_statfs(vfsp, sbp)
	register struct vfs *vfsp;
	struct statfs *sbp;
{
	register struct pcfs *fsp;
	int error;

	fsp = VFSTOPCFS(vfsp);
	error = pc_getfat(fsp);
	if (error)
		return (error);
	sbp->f_bsize = fsp->pcfs_clsize;
	sbp->f_blocks = fsp->pcfs_ncluster;
	sbp->f_bavail = sbp->f_bfree = pc_freeclusters(fsp);
	bzero((caddr_t)&sbp->f_fsid, sizeof (fsid_t));
#ifdef notdef
	sbp->f_fsid.val[0] = (long)fsp->pcfs_devvp->v_rdev;
#endif notdef
	sbp->f_fsid = vfsp->vfs_fsid;
	return (0);
}

/*
 * Flush any pending I/O.
 */
static int
pcfs_sync(vfsp)
register struct vfs *vfsp;
{
	struct pcfs * fsp;
	static int updlock = 0;
	int error = 0;

	if ((updlock) || (!pcinit)) {
		return (0);
	}
	updlock = 1;

	if (vfsp != NULL) {
		error = pc_syncfsnodes(VFSTOPCFS(vfsp));
	} else {
		fsp = pc_mounttab;
		while (fsp != NULL) {
			error = pc_syncfsnodes(fsp);

			if (error) break;
			fsp = fsp -> pcfs_nxt;
		}
	}
	updlock = 0;
	return (error);
}

static int
pc_syncfsnodes(fsp)
register struct pcfs * fsp;
{
	register struct pchead * hp;
	register struct pcnode * pcp;
	int error;

	PC_LOCKFS(fsp);
	if (! (error = pc_syncfat(fsp))) {
		hp = pcfhead;
		while (hp < & pcfhead [ NPCHASH ]) {
			pcp = hp->pch_forw;
			while (pcp != (struct pcnode *) hp) {
				if (VFSTOPCFS (PCTOV (pcp) -> v_vfsp) == fsp)
					if (error = pc_nodesync (pcp))
						break;
				pcp = pcp -> pc_forw;
			}
			if (error)
				break;
			hp ++;
		}
	}
	PC_UNLOCKFS (fsp);
	return (error);
}

int
pc_lockfs(fsp)
	register struct pcfs *fsp;
{
	int error;

	PC_LOCKFS(fsp);
PCFSDEBUG(6)
printf("pcfs_lockfs(0x%x) locked\n", fsp);
	error = pc_getfat(fsp);
	if (error) {
PCFSDEBUG(6)
printf("pcfs_lockfs(0x%x) getfat error\n", fsp);
		pc_unlockfs(fsp);
	}
	return (error);
}

void
pc_unlockfs(fsp)
	register struct pcfs *fsp;
{

	if (!(fsp->pcfs_flags & PCFS_LOCKED))
		panic("pc_unlockfs");
PCFSDEBUG(6)
printf("pcfs_unlockfs(0x%x)\n", fsp);
	PC_UNLOCKFS(fsp);
}

struct bootsec {
	char	instr[3];
	char	version[8];
	u_char	bps[2];			/* bytes per sector */
	u_char	spcl;			/* sectors per alloction unit */
	u_char	res_sec[2];		/* reserved sectors, starting at 0 */
	u_char	nfat;			/* number of FATs */
	u_char	rdirents[2];		/* number of root directory entries */
	u_char	totalsec[2];		/* total sectors in logical image */
	char	mediadesriptor;		/* media descriptor byte */
	u_short	fatsec;			/* number of sectors per FAT */
	u_short	spt;			/* sectors per track */
	u_short nhead;			/* number of heads */
	u_short hiddensec;		/* number of hidden sectors */
};

/*
 * Get the file allocation table.
 * If there is an old one, invalidate it.
 */
int
pc_getfat(fsp)
	register struct pcfs *fsp;
{
	register struct buf *bp = 0;
	register struct buf *tp = 0;
	register u_char *fatp;
	struct bootsec *bootp;
	struct vnode *devvp;
	int error;

PCFSDEBUG(3)
printf("pc_getfat\n");
	devvp = fsp->pcfs_devvp;
	if (fsp->pcfs_fatp) {
		/*
		 * There is a fat in core.
		 * If there are open file pcnodes or we have modified it or
		 * it hasn't timed out yet use the in core fat.
		 * Otherwise invalidate it and get a new one
		 */
		if (fsp->pcfs_frefs ||
		    (fsp->pcfs_flags & PCFS_FATMOD) ||
		    timercmp(&time, &fsp->pcfs_fattime, <)) {
PCFSDEBUG(3)
printf("pc_getfat: use same fat\n");
			return  (0);
		} else {
			pc_invalfat(fsp);
		}
	}
	/*
	 * Open block device mounted on.
	 */
	error = VOP_OPEN(&devvp,
		(PCFSTOVFS(fsp)->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		u.u_cred);
	if (error)
		/* return (error); */
		return (EIO);
	/*
	 * Get fat and check it for validity
	 */
	tp = bread(devvp, (daddr_t)0, (PC_FATBLOCK+PC_MAXFATSEC) * PC_SECSIZE);
	if (tp->b_flags & (B_ERROR | B_INVAL)) {
PCFSDEBUG(1)
printf("pc_getfat: ");
		if (tp->b_flags & B_ERROR)
			pc_diskchanged (fsp);
		else
			brelse (tp);
		return (EIO);
	}
	tp->b_flags |= B_INVAL;
	bootp = (struct bootsec *)tp->b_un.b_addr;
	fatp = (u_char *)(tp->b_un.b_addr + PC_FATBLOCK*PC_SECSIZE);
	if (fatp[1] != 0xFF || fatp[2] != 0xFF ||
		ltohs(bootp->res_sec[0]) != 1) {
		/* we don't support disks with reserved sectors. */
		error = EINVAL;
		goto out;
	}
	switch (fatp[0]) {
	case SS8SPT:
	case DS8SPT:
	case SS9SPT:
	case DS9SPT:
	case DS18SPT:
	case DS9_15SPT:
		fsp->pcfs_spcl = (int)bootp->spcl;
		fsp->pcfs_fatsec = (int)ltohs(bootp->fatsec);
		fsp->pcfs_spt = (int)ltohs(bootp->spt);
		fsp->pcfs_rdirsec = (int)ltohs(bootp->rdirents[0])
					* sizeof (struct pcdir) / PC_SECSIZE;
		fsp->pcfs_clsize = fsp->pcfs_spcl * PC_SECSIZE;
		fsp->pcfs_rdirstart = PC_FATBLOCK +
					(bootp->nfat * fsp->pcfs_fatsec);
		fsp->pcfs_datastart = fsp->pcfs_rdirstart + fsp->pcfs_rdirsec;
		fsp->pcfs_ncluster = ((int)ltohs(bootp->totalsec[0]) -
					fsp->pcfs_datastart) / fsp->pcfs_spcl;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	/*
	 * Copy the fat into a buffer in its native size.
	 */
	bp = geteblk(fsp->pcfs_fatsec * PC_SECSIZE);
	bcopy((caddr_t)fatp, (caddr_t)bp->b_un.b_addr,
		(u_int)(fsp->pcfs_fatsec * PC_SECSIZE));
	brelse(tp);

	fsp->pcfs_fatbp = bp;
	fsp->pcfs_fatp = (u_char *)bp->b_un.b_addr;
	fsp->pcfs_fattime = time;
	fsp->pcfs_fattime.tv_sec += PCFS_DISKTIMEOUT;
	return (0);
out:
PCFSDEBUG(1)
printf("pc_getfat: illegal disk format\n");
	if (tp)
		brelse(tp);

	(void) VOP_CLOSE(devvp, (PCFSTOVFS(fsp)->vfs_flag & VFS_RDONLY) ?
		FREAD : FREAD|FWRITE, 1, u.u_cred);
	return (error);
}

int
pc_syncfat(fsp)
	register struct pcfs *fsp;
{
	register struct buf *bp;
	register int fatsize;

PCFSDEBUG(6)
printf("pcfs_syncfat\n");
	if ((fsp->pcfs_fatp == (u_char *)0) || !(fsp->pcfs_flags & PCFS_FATMOD))
		return (0);
	/*
	 * write out fat
	 */
	fsp->pcfs_flags &= ~PCFS_FATMOD;
	fsp->pcfs_fattime = time;
	fsp->pcfs_fattime.tv_sec += PCFS_DISKTIMEOUT;
	fatsize = fsp->pcfs_fatbp->b_bcount;
	bp = getblk(fsp->pcfs_devvp, (daddr_t)PC_FATBLOCK, fatsize);
	bp->b_flags |= B_NOCACHE;		/* don't cache */
	bcopy((caddr_t)fsp->pcfs_fatp, bp->b_un.b_addr, (u_int)fatsize);
	bwrite(bp);
	if (bp -> b_flags & B_ERROR) {
		pc_diskchanged (fsp);
		return (EIO);
	}
	/*
	 * write out alternate fat
	 */
	bp = getblk(fsp->pcfs_devvp,
		(daddr_t)(PC_FATBLOCK + fsp->pcfs_fatsec), fatsize);
	bp->b_flags |= B_NOCACHE;		/* don't cache */
	bcopy((caddr_t)fsp->pcfs_fatp, bp->b_un.b_addr, (u_int)fatsize);
	bawrite(bp);
PCFSDEBUG(6)
printf("pcfs_syncfat: wrote out fat\n");
	return (0);
}

void
pc_invalfat(fsp)
	register struct pcfs *fsp;
{

PCFSDEBUG(6)
printf("pc_invalfat\n");
	if (fsp->pcfs_fatp == (u_char *)0 || fsp->pcfs_frefs)
		panic("pc_invalfat");
	/*
	 * Release old fat, invalidate all the blocks associated
	 * with the device.
	 */
	brelse(fsp->pcfs_fatbp);
	fsp->pcfs_fatbp = (struct buf *)0;
	fsp->pcfs_fatp = (u_char *)0;
	binval(fsp->pcfs_devvp);
	/* ?? Also have to invalidate all the vnode pages. */
	/*
	 * close mounted device
	 */
	(void) VOP_CLOSE(fsp->pcfs_devvp,
			(PCFSTOVFS(fsp)->vfs_flag & VFS_RDONLY) ?
			FREAD : FREAD|FWRITE, 1, u.u_cred);
}

pc_badfs(fsp)
	struct pcfs *fsp;
{

	uprintf("corrupted PC file system on dev 0x%x\n",
		fsp->pcfs_devvp->v_rdev);
}

#ifdef notdef
/*
 * The problem with supporting NFS mount PCFS filesystem is that there
 * is no good place to keep the generation number. The only possible
 * place is inside a directory entry. (There are a few words that are
 * not used.) But directory entries come and go. That is, if a
 * directory is removed completely, its directory blocks are freed
 * and the generation numbers are lost. Whereas in ufs, inode blocks
 * are dedicated for inodes, so the generation numbers are permanently
 * kept on the disk.
 */
static int
pcfs_vget(vfsp, vpp, fidp)
	struct vfs *vfsp;
	struct vnode **vpp;
	struct fid *fidp;
{
	struct pcnode *pcp;
	struct pcdir *ep;
	struct pcfid *pcfid;
	struct pcfs *fsp;
	long eblkno;
	int eoffset;
	struct buf *bp;
	int error;

	pcfid = (struct pcfid *)fidp;
	fsp = VFSTOPCFS(vfsp);

	if (pcfid->pcfid_fileno == -1) {
		pcp = pc_getnode(fsp, (daddr_t)0, 0, (struct pcdir *)0);
		pcp -> pc_flags |= PC_EXTERNAL;
		*vpp = PCTOV(pcp);
		return (0);
	}
	if (pcfid->pcfid_fileno < 0) {
		eblkno = pc_cldaddr(fsp, -pcfid->pcfid_fileno - 1);
		eoffset = 0;
	} else {
		eblkno = pcfid->pcfid_fileno / ENTPS;
		eoffset = (pcfid->pcfid_fileno % ENTPS) * sizeof (struct pcdir);
	}
	error = pc_lockfs(fsp);
	if (error) {
		*vpp = NULL;
		return (error);
	}

	if (eblkno >= fsp->pcfs_datastart || (eblkno-fsp->pcfs_rdirstart)
	    < (fsp->pcfs_rdirsec & ~(fsp->pcfs_spcl - 1))) {
		bp = bread(fsp->pcfs_devvp, eblkno, fsp->pcfs_clsize);
	} else {
		bp = bread(fsp->pcfs_devvp, eblkno,
			(int) (fsp->pcfs_datastart - eblkno)*PC_SECSIZE);
	}
	if ((bp)->b_flags & (B_ERROR | B_INVAL)) {
		if (bp -> b_flags & B_ERROR)
			pc_diskchanged (fsp);
		else
			brelse (bp);
		*vpp = NULL;
		pc_unlockfs(fsp);
		return (EIO);
	}
	ep = (struct pcdir *)(bp->b_un.b_addr + eoffset);
	if ((ep->pcd_gen == pcfid->pcfid_gen) &&
		pc_validchar(ep->pcd_filename[0])) {
		pcp = pc_getnode(fsp, eblkno, eoffset, ep);
		*vpp = PCTOV(pcp);
		pcp -> pc_flags |= PC_EXTERNAL;
	} else
		*vpp = NULL;
	pc_unlockfs(fsp);
	bp->b_flags |= B_NOCACHE;
	brelse(bp);
	return (0);
}
#endif notdef
