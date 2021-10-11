/*  @(#)tmp.h 1.1 92/07/30 SMI */

/*
 * Utility and library routines that look at directories want to
 * `see' something like an inode.  While each tmpnode has a unique
 * number (its memory address), the address turns out (signed long)
 * to be unsuitable for library utilities.  The following data structure
 * is used to allocate a (small) tmpfs index number for these purposes.
 */
#define	TMPIMAPNODES	128
#define	TMPIMAPSIZE	TMPIMAPNODES/NBBY

#define	MAXMNTLEN	512	/* max length of pathname tmpfs is mounted on */

struct tmpimap {
	u_char timap_bits[TMPIMAPSIZE];	/* bitmap of available index numbers */
					/* 0 == free number */
	struct tmpimap *timap_next;	/* ptr to more index numbers */
};

/*
 * Temporary file system per-mount data and other random stuff
 * There is a linked list of these things rooted at tmpfs_mountp
 */

struct tmount {
	struct tmount	*tm_next;	/* for linked list */
	struct vfs	*tm_vfsp;	/* filesystem's vfs struct */
	struct tmpnode	*tm_rootnode;	/* root tmpnode */
	u_int		tm_mntno;	/* minor # of mounted `device' */
	struct tmpimap	tm_inomap;	/* inode allocator maps */
	u_int		tm_direntries;	/* number of directory entries */
	u_int		tm_directories; /* number of directories */
	u_int		tm_files;	/* number of regular files */
	u_int		tm_kmemspace;	/* bytes of kmem_alloc'd memory */
	u_int		tm_anonmem;	/* bytes of anon memory actually used */
	char 		tm_mntpath[MAXMNTLEN]; /* name of tmpfs mount point */

};

#ifdef KERNEL
char *tmp_memalloc();
void tmp_memfree();
#define	GET_TIME(tv)		((*tv) = time)
#define	VFSP_TO_TM(vfsp)	((struct tmount *)(vfsp)->vfs_data)
#define	VP_TO_TM(vp)		((struct tmount *)(vp)->v_vfsp->vfs_data)
#define	VP_TO_TN(vp)		((struct tmpnode *)(vp)->v_data)
#endif KERNEL

/*
 * Don't allocate more anon pages for tmp files if free anon space
 * goes under TMPHIWATER.  Hideous deadlocks can occur.  This can be
 * patched in tmpfs_hiwater.
 * XXX better heuristic needed
 */
#define	TMPHIWATER	4*1024*1024	/* ie, 4 Megabytes */

/*
 * Each tmpfs can allocate only a certain amount of kernel memory,
 * which is used for directories (may change), anon maps, inode maps,
 * and other goodies.  This is statically set (during first tmp_mount())
 * as a percent of physmem.  The actual percentage can be patched in
 * tmpfs_maxprockmem.
 * XXX better heuristic needed
 */
#define	TMPMAXPROCKMEM	2		/* Means 2 procent of physical memory */

/*
 * patchable variables controlling debugging output
 * defined in tmp_vnodeops XXX
 */
#define	TMPFSDEBUG 	1	/* XXX REMOVE ALL OF THESE FOR FCS!! */

#ifdef TMPFSDEBUG
extern int tmpfsdebug;		/* general debugging (e.g. function calls) */
extern int tmpdebugerrs;	/* report non-fatal error conditions */
extern int tmplockdebug;	/* report on tmpnode locking and unlocking */
extern int tmpdirdebug;		/* report of file and directory manipulation */
extern int tmprwdebug;		/* read/write debugging */
extern int tmpdebugalloc;	/* tmpfs memory and swap allocation */
#endif TMPFSDEBUG
