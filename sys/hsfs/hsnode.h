/*	@(#)hsnode.h 1.1 92/07/30 SMI  */

/*
 * High Sierra filesystem structure definitions
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

struct	hs_direntry {
	u_int		ext_lbn;	/* LBN of start of extent */
	u_int		ext_size;    	/* no. of data bytes in extent */
	struct timeval	cdate;		/* creation date */
	enum vtype	type;		/* file type */
	u_short		mode;		/* mode and type of file (UNIX) */
	u_short		nlink;		/* no. of links to file */
	u_short		uid;		/* owner's user id */
	u_short		gid;		/* owner's group id */
	u_int		xar_prot :1;	/* 1 if protection in XAR */
	u_char		xar_len;	/* no. of Logical blocks in XAR */
	u_char		intlf_sz;	/* intleaving size */
	u_char		intlf_sk;	/* intleaving skip factor */
};

struct	ptable {
	u_char	filler[7];		/* filler */
	u_char	dname_len;		/* length of directory name */
	u_char	dname[HS_DIR_NAMELEN+1];/* directory name */
};

struct ptable_idx {
	struct ptable_idx *idx_pptbl_idx;/* parent's path table index entry */
	struct ptable	*idx_mptbl;	/* path table entry for myself */
	u_short	idx_nochild;		/* no. of childern */
	u_short idx_childid;		/* directroy no of first child */  
};

/* hs_offset, hs_ptbl_idx, base  apply to VDIR type only */
struct  hsnode {
	struct hsnode	*hs_hash;	/* next hsnode in hash list */
	struct hsnode	*hs_freef;	/* next hsnode in free list */
	struct hsnode	*hs_freeb;	/* previous hsnode in free list */
	struct vnode	hs_vnode;	/* the real vnode for the file */
	struct hs_direntry hs_dirent;
					/* the directory entry for this file */
	u_int		hs_dir_lbn;	/* LBN of directory entry */
	u_int		hs_dir_off;	/* offset in LBN of directory entry */
	struct ptable_idx	*hs_ptbl_idx;	/* path table index*/
	u_int		hs_offset;	/* start offset in dir for searching */
	u_int		hs_flags;	/* (see below) */
};


struct  hsfid {
	u_short		hf_len;		/* length of fid */
	u_short		hf_dir_off;	/* offset in LBN of directory entry */
	u_int		hf_ext_lbn;	/* LBN of data extent */
	u_int		hf_dir_lbn;	/* LBN of directory entry */
};


struct	hs_volume {
	u_long		vol_size; 	/* no. of Logical blocks in Volume */
	u_int		lbn_size;	/* no. of bytes in a block */
	u_int		lbn_shift;	/* shift to convert lbn to bytes */
	u_int		lbn_secshift;	/* shift to convert lbn to sec */
	u_char		file_struct_ver;/* version of directory structure */
	short		vol_uid;	/* uid of volume */
	short		vol_gid;	/* gid of volume */
	u_short		vol_prot;	/* protection (mode) of volume */
	struct timeval	cre_date;	/* volume creation time */
	struct timeval	mod_date;	/* volume modification time */
	struct	hs_direntry	root_dir;	
					/* directory entry for Root Directory */
	u_short		ptbl_len;	/* number of bytes in Path Table */
	u_int		ptbl_lbn;	/* logical block no of Path Table */
	u_short		vol_set_size;	/* number of CD in this vol set */
	u_short		vol_set_seq;	/* the sequence number of this CD */
};

/*
 * hsnode incore table 
 *
 */
#define HS_HASHSIZE	32		/* hsnode hash table size */
#ifdef i386 || sun4c
#define HS_HSTABLESIZE	4 * PAGESIZE	/* hsnode incore table size */
#else
#define HS_HSTABLESIZE	2 * PAGESIZE	/* hsnode incore table size */
#endif

struct hstable {
	struct  vfs *hs_vfsp;		/* point back to vfs */
	int	hs_tablesize;		/* size of the hstable */
	struct	hsnode	*hshash[HS_HASHSIZE];	/* head of hash lists */
	struct	hsnode	*hsfree_f;		/* first entry of free list */
	struct	hsnode	*hsfree_b;		/* last entry of free list */
	int	hs_refct;		/* no. of active hsnode */
	int	hs_nohsnode;		/* no. of hsnode in table */
	struct	hsnode	hs_node[1];		/* should be much more */
};

/*
 * High Sierra filesystem structure.
 * There is one of these for each mounted High Sierra filesystem.
 */
enum hs_vol_type {
	HS_VOL_TYPE_HS = 0, HS_VOL_TYPE_ISO = 1,
	HS_VOL_TYPE_UNIX = 2
};
struct hsfs {
	struct hsfs *hsfs_next;		/* ptr to next entry in linked list */
	struct vfs *hsfs_vfs;		/* vfs for this fs */
	struct vnode *hsfs_rootvp;	/* vnode for root of filesystem */
	struct vnode *hsfs_devvp;	/* device mounted on */
	enum hs_vol_type hsfs_vol_type; /* 0 hsfs 1 iso 2 hsfs+sun 3 iso+sun */
	struct hs_volume hsfs_vol;	/* File Structure Volume Descriptor */
	struct ptable*	hsfs_ptbl;	/* pointer to incore Path Table */
	int hsfs_ptbl_size;		/* size of incore path table */
	struct ptable_idx *hsfs_ptbl_idx;/* pointer to path table index */
	int hsfs_ptbl_idx_size;		/* no. of path table index */
	int hsfs_flags;			/* flags */
	struct hstable *hsfs_hstbl;	/* pointer to incore hsnode table */
};

/*
 * flags in hsfs_flags field
 */
#define HSFS_FATMOD	0x01		/* fat has been modified */
#define HSFS_LOCKED	0x02		/* fs is locked */
#define HSFS_WANTED	0x04		/* locked fs is wanted */

/*
 * File system parameter macros
 */
#define hs_blksize(HSFS, HSP, OFF)	/* file system block size */ \
	((HSP)->hs_vn.v_flag & VROOT? \
	   ((OFF) >= \
	     ((HSFS)->hsfs_rdirsec & ~((HSFS)->hsfs_spcl - 1)) * HS_SECSIZE? \
	       ((HSFS)->hsfs_rdirsec & ((HSFS)->hsfs_spcl - 1)) * HS_SECSIZE: \
	       (HSFS)->hsfs_clsize): \
	   (HSFS)->hsfs_clsize)
#define hs_blkoff(OFF)		/* offset within block */ \
	((OFF) & (HS_SECSIZE - 1))

/*
 * Conversion macros
 */
#define VFS_TO_HSFS(VFSP)	((struct hsfs *)(VFSP)->vfs_data)
#define HSFS_TO_VFS(FSP)	((FSP)->hsfs_vfs)

#define VTOH(VP)		((struct hsnode *)(VP)->v_data)
#define HTOV(HP)		(&((HP)->hs_vnode))

/*
 * Convert between Logical Block Number and Sector Number.
 */
#define LBN_TO_SEC(lbn, vfsp)	((lbn)>>((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_secshift)
	
#define SEC_TO_LBN(sec, vfsp)	((sec)<<((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_secshift)

#define LBN_TO_BYTE(lbn, vfsp)	((lbn)<<((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_shift)

/*
 * lock and unlock macros
 */
#define HS_LOCKFS(HSFS) { \
	while ((HSFS)->hsfs_flags & HSFS_LOCKED) { \
		(HSFS)->hsfs_flags |= HSFS_WANTED; \
		(void) sleep((caddr_t)(HSFS), PRIBIO); \
	} \
	(HSFS)->hsfs_flags |= HSFS_LOCKED; \
}

#define HS_UNLOCKFS(HSFS) { \
	(HSFS)->hsfs_flags &= ~HSFS_LOCKED; \
	if ((HSFS)->hsfs_flags & HSFS_WANTED) { \
		(HSFS)->hsfs_flags &= ~HSFS_WANTED; \
		wakeup((caddr_t)(HSFS)); \
	} \
}

