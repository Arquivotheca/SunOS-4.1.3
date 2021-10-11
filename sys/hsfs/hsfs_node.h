/* @(#)hsfs_node.h 1.1 92/07/30 SMI */
/*
 * High Sierra filesystem structure definitions
 * Copyright (c) 1989, 1990 by Sun Microsystem, Inc.
 */

#ifndef	_HSFS_NODE_H_
#define	_HSFS_NODE_H_

struct	hs_direntry {
	u_int		ext_lbn;	/* LBN of start of extent */
	u_int		ext_size;    	/* no. of data bytes in extent */
	struct timeval	cdate;		/* creation date */
	struct timeval	mdate;		/* last modification date */
	struct timeval	adate;		/* last access date */
	enum vtype	type;		/* file type */
	mode_t		mode;		/* mode and type of file (UNIX) */
	u_int		nlink;		/* no. of links to file */
	uid_t		uid;		/* owner's user id */
	gid_t		gid;		/* owner's group id */
	dev_t		r_dev;		/* major/minor device numbers */
	u_int		xar_prot :1;	/* 1 if protection in XAR */
	u_char		xar_len;	/* no. of Logical blocks in XAR */
	u_char		intlf_sz;	/* intleaving size */
	u_char		intlf_sk;	/* intleaving skip factor */
	u_short		sym_link_flag;	/* flags for sym link */
	char		*sym_link; 	/* path of sym link for readlink() */
};

struct	ptable {
	u_char	filler[7];		/* filler */
	u_char	dname_len;		/* length of directory name */
	u_char	dname[HS_DIR_NAMELEN+1];	/* directory name */
};

struct ptable_idx {
	struct ptable_idx *idx_pptbl_idx; /* parent's path table index entry */
	struct ptable	*idx_mptbl;	/* path table entry for myself */
	u_short	idx_nochild;		/* no. of children */
	u_short idx_childid;		/* directory no of first child */
};

/* hs_offset, hs_ptbl_idx, base  apply to VDIR type only */
struct  hsnode {
	struct hsnode	*hs_hash;	/* next hsnode in hash list */
	struct hsnode	*hs_freef;	/* next hsnode in free list */
	struct hsnode	*hs_freeb;	/* previous hsnode in free list */
	struct vnode	hs_vnode;	/* the real vnode for the file */
	struct hs_direntry hs_dirent;	/* the directory entry for this file */
	u_int		hs_dir_lbn;	/* LBN of directory entry */
	u_int		hs_dir_off;	/* offset in LBN of directory entry */
	struct ptable_idx	*hs_ptbl_idx;	/* path table index */
	u_int		hs_offset;	/* start offset in dir for searching */
	long		hs_mapcnt;	/* mappings to file pages */
	u_long		hs_vcode;	/* version code */
	u_int		hs_flags;	/* (see below) */
#ifdef NOTUSED
	mutex_t		hs_contents_lock;	/* protects hsnode contents */
#endif
						/* 	except hs_offset */
};

/* hs_flags */
#define	HREF	1			/* hsnode is referenced */

struct  hsfid {
	u_short		hf_len;		/* length of fid */
	u_short		hf_dir_off;	/* offset in LBN of directory entry */
	u_int		hf_ext_lbn;	/* LBN of data extent */
	u_int		hf_dir_lbn;	/* LBN of directory entry */
};


/*
 * All of the fields in the hs_volume are read-only once they have been
 * initialized.
 */
struct	hs_volume {
	u_long		vol_size; 	/* no. of Logical blocks in Volume */
	u_int		lbn_size;	/* no. of bytes in a block */
	u_int		lbn_shift;	/* shift to convert lbn to bytes */
	u_int		lbn_secshift;	/* shift to convert lbn to sec */
	u_char		file_struct_ver; /* version of directory structure */
	uid_t		vol_uid;	/* uid of volume */
	gid_t		vol_gid;	/* gid of volume */
	u_int		vol_prot;	/* protection (mode) of volume */
	struct timeval	cre_date;	/* volume creation time */
	struct timeval	mod_date;	/* volume modification time */
	struct	hs_direntry root_dir;	/* dir entry for Root Directory */
	u_short		ptbl_len;	/* number of bytes in Path Table */
	u_int		ptbl_lbn;	/* logical block no of Path Table */
	u_short		vol_set_size;	/* number of CD in this vol set */
	u_short		vol_set_seq;	/* the sequence number of this CD */
	char		vol_id[32];		/* volume id in PVD */
};

/*
 * hsnode incore table
 *
 */
#define	HS_HASHSIZE	32		/* hsnode hash table size */
#if i386 || sun4c
#define	HS_HSTABLESIZE	4 * PAGESIZE	/* hsnode incore table size */
#else
#define	HS_HSTABLESIZE	2 * PAGESIZE	/* hsnode incore table size */
#endif

struct hstable {
	struct  vfs *hs_vfsp;		/* point back to vfs */
	int	hs_tablesize;		/* size of the hstable */
#ifdef NOTUSED
	rwlock_t hshash_lock;		/* protect hash table */
	mutex_t hsfree_lock;		/* protect free list */
#endif
	struct	hsnode	*hshash[HS_HASHSIZE];	/* head of hash lists */
	struct	hsnode	*hsfree_f;		/* first entry of free list */
	struct	hsnode	*hsfree_b;		/* last entry of free list */
	int 	hs_refct;
	int	hs_nohsnode;		/* no. of hsnode in table */
	struct	hsnode	hs_node[1];		/* should be much more */
};

/*
 * High Sierra filesystem structure.
 * There is one of these for each mounted High Sierra filesystem.
 */
enum hs_vol_type {
	HS_VOL_TYPE_HS = 0, HS_VOL_TYPE_ISO = 1
};
#define	HSFS_MAGIC 0x03095500
struct hsfs {
	struct hsfs *hsfs_next;		/* ptr to next entry in linked list */
	long hsfs_magic;		/* should be HSFS_MAGIC */
	struct vfs *hsfs_vfs;		/* vfs for this fs */
	struct vnode *hsfs_rootvp;	/* vnode for root of filesystem */
	struct vnode *hsfs_devvp;	/* device mounted on */
	enum hs_vol_type hsfs_vol_type; /* 0 hsfs 1 iso 2 hsfs+sun 3 iso+sun */
	struct hs_volume hsfs_vol;	/* File Structure Volume Descriptor */
	struct ptable*	hsfs_ptbl;	/* pointer to incore Path Table */
	int hsfs_ptbl_size;		/* size of incore path table */
	struct ptable_idx *hsfs_ptbl_idx; /* pointer to path table index */
	int hsfs_ptbl_idx_size;		/* no. of path table index */
	struct hstable *hsfs_hstbl;	/* pointer to incore hsnode table */
	u_long ext_impl_bits;		/* ext. information bits */
	u_short sua_offset;		/* the SUA offset */
};

/*
 * File system parameter macros
 */
#define	hs_blksize(HSFS, HSP, OFF)	/* file system block size */ \
	((HSP)->hs_vn.v_flag & VROOT ? \
	    ((OFF) >= \
		((HSFS)->hsfs_rdirsec & ~((HSFS)->hsfs_spcl - 1))*HS_SECSIZE ?\
		((HSFS)->hsfs_rdirsec & ((HSFS)->hsfs_spcl - 1))*HS_SECSIZE :\
		(HSFS)->hsfs_clsize): \
	    (HSFS)->hsfs_clsize)
#define	hs_blkoff(OFF)		/* offset within block */ \
	((OFF) & (HS_SECSIZE - 1))

/*
 * Conversion macros
 */
#define	VFS_TO_HSFS(VFSP)	((struct hsfs *)(VFSP)->vfs_data)
#define	HSFS_TO_VFS(FSP)	((FSP)->hsfs_vfs)

#define	VTOH(VP)		((struct hsnode *)(VP)->v_data)
#define	HTOV(HP)		(&((HP)->hs_vnode))

/*
 * Convert between Logical Block Number and Sector Number.
 */
#define	LBN_TO_SEC(lbn, vfsp)	((lbn)>>((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_secshift)

#define	SEC_TO_LBN(sec, vfsp)	((sec)<<((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_secshift)

#define	LBN_TO_BYTE(lbn, vfsp)	((lbn)<<((struct hsfs *)((vfsp)->vfs_data))->  \
				hsfs_vol.lbn_shift)

#define	MAXNAMELEN 256
#define	ISVDEV(t) ((t == VCHR) || (t == VBLK) || (t == VFIFO))
/*
 * global routines.
 */
#ifdef __STDC__

extern size_t strlen(char *);
extern char *strcpy(char *s1, char *s2);
extern char *strncpy(char *s1, char *s2, size_t n);

/* read a sector */
extern int hs_readsector(struct vnode *vp, u_int secno,	u_char *ptr);
/* lookup/construct an hsnode/vnode */
extern struct vnode *hs_makenode(struct hs_direntry *dp,
	u_int lbn, u_int off, struct vfs *vfsp);
/* make hsnode from directory lbn/off */
extern int hs_remakenode(u_int lbn, u_int off, struct vfs *vfsp,
	struct vnode **vpp);
/* lookup name in directory */
extern int hs_dirlook(struct vnode *dvp, char *name, int namlen,
	struct vnode **vpp, struct cred *cred);
/* find an hsnode in the hash list */
extern struct vnode *hs_findhash(u_int ext, struct vfs *vfsp);
/* destroy an hsnode */
extern void hs_freenode(struct hsnode *hp, struct vfs *vfsp, int nopage);
/* destroy the incore hnode table */
extern void hs_freehstbl(struct vfs *vfsp);
/* parse a directory entry */
extern int hs_parsedir(struct hsfs *fsp, u_char *dirp,
	struct hs_direntry *hdp, char *dnp, int *dnlen);
/* convert d-characters */
extern int hs_namecopy(char *from, char *to, int size);
/* destroy the incore hnode table */
extern void hs_filldirent(struct vnode *vp, struct hs_direntry *hdp);
/* check vnode protection */
extern int hs_access(struct vnode *vp, mode_t m, struct cred *cred);


extern void hs_parse_dirdate(u_char *dp, struct timeval *tvp);
extern void hs_parse_longdate(u_char *dp, struct timeval *tvp);
extern void hs_freehstbl(struct vfs *vfsp);
extern void hs_freenode(struct hsnode *hp, struct vfs *vfsp, int nopage);
extern int hs_uppercase_copy(char *from, char *to, int size);
extern void hs_check_root_dirent(struct vnode *vp, struct hs_direntry *hdp);

#else

extern size_t strlen();
extern char *strcpy();
extern char *strncpy();

extern int hs_readsector();		/* read a sector */

extern struct vnode *hs_makenode();	/* lookup/construct an hsnode/vnode */
extern int hs_remakenode();		/* make hsnode from directory lbn/off */
extern int hs_dirlook();		/* lookup name in directory */
extern struct vnode *hs_findhash();	/* find an hsnode in the hash list */
extern void hs_freenode();		/* destroy an hsnode */
extern void hs_freehstbl();		/* destroy the incore hnode table */

extern int hs_parsedir();		/* parse a directory entry */
extern int hs_namecopy();		/* convert d-characters */
extern void hs_filldirent();		/* destroy the incore hnode table */

extern int hs_access();			/* check vnode protection */

extern void hs_parse_dirdate();
extern void hs_parse_longdate();
extern void hs_freehstbl();
extern void hs_freenode();
extern int hs_uppercase_copy();
extern void hs_check_root_dirent();
#endif

#endif /* !_HSFS_NODE_H_ */
