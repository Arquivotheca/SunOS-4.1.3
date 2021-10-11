/*	@(#)pc_node.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

struct pcnode {
	struct pcnode *pc_forw;		/* active list ptrs, must be first */
	struct pcnode *pc_back;
	int pc_flags;			/* see below */
	struct vnode * pc_vn;		/* vnode for pcnode */
	long pc_size;			/* size of file */
	short pc_scluster;		/* starting cluster of file */
	daddr_t pc_eblkno;		/* disk blkno for entry */
	int pc_eoffset;			/* offset in disk block of entry */
	struct pcdir pc_entry;		/* directory entry of file */
};

/*
 * flags
 */
#define	PC_MOD		0x01		/* file data has been modified */
#define	PC_CHG		0x02		/* node data has been changed */
#define	PC_INVAL	0x04		/* node is invalid */
#define	PC_RELE_PEND	0x08		/* node is waiting to be released */
#define	PC_EXTERNAL	0x10		/* vnode ref is held externally */

#define	PCTOV(PCP)	((PCP)->pc_vn)
#define	VTOPC(VP)	((struct pcnode *)((VP)->v_data))

#define	PRIPCNODE	(PZERO - 1)

/*
 * Make a unique integer for a file
 */
#define	ENTPS		(PC_SECSIZE / sizeof (struct pcdir))

#define	pc_makenodeid(BN, OFF, EP) \
	((EP)->pcd_attr & PCA_DIR? \
		-ltohs((EP)->pcd_scluster) - 1: \
		((BN) * ENTPS) + ((OFF) / sizeof (struct pcdir)))

#define	NPCHASH 1

#if NPCHASH == 1
#define	PCFHASH(FSP, BN, O)	0
#define	PCDHASH(FSP, SC)	0
#else
#define	PCFHASH(FSP, BN, O)	(((unsigned)FSP + BN + O) % NPCHASH)
#define	PCDHASH(FSP, SC)	(((unsigned)FSP + SC) % NPCHASH)
#endif

struct pchead {
	struct pcnode *pch_forw;
	struct pcnode *pch_back;
};

/*
 * pcnode file, directory and invalid node operations vectors
 */
extern struct vnodeops pcfs_fvnodeops;
extern struct vnodeops pcfs_dvnodeops;
extern struct pchead pcfhead[];
extern struct pchead pcdhead[];

/*
 * pcnode routines
 */
extern void pc_init();
extern struct pcnode *pc_getnode();	/* get a pcnode for a file */
extern void pc_rele();			/* release a pcnode */
extern void pc_mark();			/* mark pcnode with current time */
extern int pc_nodesync();		/* flush node blocks and update entry */
extern int pc_nodeupdate();		/* update node directory entry */
extern int pc_bmap();			/* map logical to physical blocks */
extern int pc_balloc();			/* allocate blocks for a file */
extern int pc_bfree();			/* free blocks for a file */
extern int pc_verify();			/* verify a pcnode with the disk */
extern void pc_diskchanged();		/* call when disk change is detected */
