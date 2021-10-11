/*	@(#)pc_fs.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * PC (MSDOS) compatible virtual file system.
 *
 * A main goal of the implementaion was to maintain statelessness
 * except while files are open. Thus mounting and unmounting merely
 * declared the file system name. The user may change disks at almost
 * any time without concern (just like the PC). It is assumed that when
 * files are open for writing the disk access light will be on, as a
 * warning not to change disks. The implementation must, however, detect
 * disk change and recover gracefully. It does this by comparing the
 * in core entry for a directory to the on disk entry whenever a directory
 * is searched. If a discrepancy is found active directories become root and
 * active files are marked invalid.
 *
 * There are only two type of nodes on the PC file system; files and
 * directories. These are represented by two separate vnode op vectors,
 * and they are kept in two separate tables. Files are known by the
 * disk block number and block (cluster) offset of the files directory
 * entry. Directories are known by the starting cluster number.
 *
 * The file system is locked for during each user operation. This is
 * done to simplify disk verification error conditions.
 */

struct pcfs {
	struct vfs *pcfs_vfs;		/* vfs for this fs */
	int pcfs_flags;			/* flags */
	struct vnode *pcfs_devvp;	/* device mounted on */
	int pcfs_spcl;			/* sectors per cluster */
	int pcfs_spt;			/* sectors per track */
	int pcfs_fatsec;		/* number of sec in fat */
	int pcfs_rdirsec;		/* number of sec in root dir */
	daddr_t pcfs_rdirstart;		/* start blkno of root dir */
	daddr_t pcfs_datastart;		/* start blkno of data area */
	int pcfs_clsize;		/* cluster size in bytes */
	int pcfs_ncluster;		/* number of clusters in fs */
	int pcfs_nrefs;			/* number of active pcnodes */
	int pcfs_frefs;			/* number of active file pcnodes */
	struct buf *pcfs_fatbp;		/* ptr to fat buffer */
	u_char *pcfs_fatp;		/* ptr to fat data */
	struct timeval pcfs_fattime;	/* time fat becomes invalid */
	struct timeval pcfs_verifytime;	/* time to reverify disk */
	long pcfs_owner;		/* proc index of process locking pcfs */
	long pcfs_count;		/* # of pcfs locks for pcfs_owner */
	struct pcfs *pcfs_nxt;		/* linked list of all mounts */
};

/*
 * flags
 */
#define	PCFS_FATMOD	0x01		/* fat has been modified */
#define	PCFS_LOCKED	0x02		/* fs is locked */
#define	PCFS_WANTED	0x04		/* locked fs is wanted */

/*
 * Disk timeout value in sec.
 * This is used to time out the in core fat and to re-verify the disk.
 * This should be less than the time it takes to change floppys
 */
#define	PCFS_DISKTIMEOUT	2

#define	VFSTOPCFS(VFSP)		((struct pcfs *)((VFSP)->vfs_data))
#define	PCFSTOVFS(FSP)		((FSP)->pcfs_vfs)

/*
 * special cluster numbers in fat
 */
#define	PCF_FREECLUSTER		0x00	/* cluster is available */
#define	PCF_ERRORCLUSTER	0x01	/* error occurred allocating cluster */
#define	PCF_LASTCLUSTER		0xFF8	/* >= means last cluster in file */
#define	PCF_FIRSTCLUSTER	2	/* first data cluster number */

/*
 * file system constants
 */
#define	PC_MAXFATSEC	9		/* maximum number of sectors in fat */

/*
 * file system parameter macros
 */
#define	pc_blksize(PCFS, PCP, OFF)	/* file system block size */ \
	(PCTOV(PCP)->v_flag & VROOT? \
	    ((OFF) >= \
	    ((PCFS)->pcfs_rdirsec & ~((PCFS)->pcfs_spcl - 1)) * PC_SECSIZE? \
	    ((PCFS)->pcfs_rdirsec & ((PCFS)->pcfs_spcl - 1)) * PC_SECSIZE: \
	    (PCFS)->pcfs_clsize): \
	    (PCFS)->pcfs_clsize)

#define	pc_blkoff(PCFS, OFF)		/* offset within block */ \
	((OFF) & ((PCFS)->pcfs_clsize - 1))

#define	pc_lblkno(PCFS, OFF)		/* logical block (cluster) no */ \
	((daddr_t)((OFF) / (PCFS)->pcfs_clsize))

#define	pc_dbtocl(PCFS, DB)		/* disk blks to clusters */ \
	((int)((DB) / (PCFS)->pcfs_spcl))

#define	pc_cltodb(PCFS, CL)		/* clusters to disk blks */ \
	((daddr_t)((CL) * (PCFS)->pcfs_spcl))

#define	pc_cldaddr(PCFS, CL)		/* disk address for cluster */ \
	((daddr_t)((PCFS)->pcfs_datastart + \
	    ((CL) - PCF_FIRSTCLUSTER) * (PCFS)->pcfs_spcl))

#define	pc_daddrcl(PCFS, DADDR)		/* cluster for disk address */ \
	((int)((((DADDR) - (PCFS)->pcfs_datastart) / (PCFS)->pcfs_spcl) + 2))

#define	pc_validcl(PCFS, CL)		/* check that cluster no is legit */ \
	((CL) >= PCF_FIRSTCLUSTER && \
	    (CL) < (PCFS)->pcfs_ncluster + PCF_FIRSTCLUSTER)

/*
 * lock and unlock macros
 */
#define	PC_LOCKFS(PCFS) { \
	while (((PCFS)->pcfs_flags & PCFS_LOCKED) && \
		(PCFS)->pcfs_owner != uniqpid()) { \
		(PCFS)->pcfs_flags |= PCFS_WANTED; \
		(void) sleep((caddr_t)(PCFS), PRIBIO); \
	} \
	(PCFS)->pcfs_owner = uniqpid(); \
	(PCFS)->pcfs_count++; \
	(PCFS)->pcfs_flags |= PCFS_LOCKED; \
}
#define	PC_UNLOCKFS(PCFS) { \
	if (--(PCFS)->pcfs_count < 0) \
		panic("PC_UNLOCKFS"); \
	if ((PCFS)->pcfs_count == 0) { \
		(PCFS)->pcfs_flags &= ~PCFS_LOCKED; \
		if ((PCFS)->pcfs_flags & PCFS_WANTED) { \
			(PCFS)->pcfs_flags &= ~PCFS_WANTED; \
			wakeup((caddr_t)(PCFS)); \
		} \
	} \
}

/*
 * external routines.
 */
extern int pc_lockfs();			/* lock fs and get fat */
extern void pc_unlockfs();		/* ulock the fs */
extern int pc_getfat();			/* get fat from disk */
extern void pc_invalfat();		/* invalidate incore fat */
extern int pc_syncfat();		/* sync fat to disk */
extern int pc_freeclusters();		/* number of free clusters in the fs */
extern short pc_alloccluster();		/* allocate a new cluster */
extern void pc_setcluster();		/* set the next cluster fat */

/*
 * debugging
 */
extern int pcfsdebuglevel;
#define	PCFSDEBUG(X)  if (X <= pcfsdebuglevel)
