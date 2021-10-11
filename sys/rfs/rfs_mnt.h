/*	@(#)rfs_mnt.h 1.1 92/07/30 SMI	*/

#ifndef _rfs_rfs_mnt_h
#define _rfs_rfs_mnt_h

/*
 * RFS client-side mount structure.
 * One allocated on every mount.
 * Private part of vfs structure.
 */
struct	rfsmnt
{
	struct vfs	*rm_vfsp;	/* vfs structure for this filesystem */
	struct vnode	*rm_rootvp;	/* root vnode for this filesystem */
	int		rm_refcnt;	/* Number of references to mount */
	char 		rm_name[MAXDNAME];	/* name of remote resource */
	u_short		rm_flags;	/* flags */
	short		rm_fstyp;	/* File system type */
	int 		rm_mntno;	/* Major device # of this mount */
        long    	rm_bsize;       /* Block size */
        long    	rm_frsize;      /* Fragment size (if supported) */
        long    	rm_blocks;      /* Total # blocks on file system */
        long    	rm_files;       /* Total # of file nodes (inodes) */
	int		rm_bcount;	/* Number of references to mount */
        char    	rm_fname[6];    /* Volume name */
        char    	rm_fpack[6];    /* Pack name */
};

#ifdef KERNEL
extern struct rfsmnt *rfsmount;
extern int nrfsmount;
#endif

#define MFREE   0x0     /* Unused - free */
#define MINUSE  0x1     /* In use - fs mounted on this entry */
#define RINUSE  MINUSE
#define MINTER  0x2     /* Mount system call is using this entry */
#define MRDONLY 0x4     /* File system is mounted read only */


/* remoteness flags (rm_flags) */
/* (or'ed with m_flags in srmount, so must use different bits) */

#define MLBIN      0x10
#define MDOTDOT    0x20
#define MFUMOUNT   0x40
#define MLINKDOWN  0x80
#define MCACHE 	   0x100     /* mount files are cacheable */

/*
 * Convert vfs ptr to private mount data ptr.
 */
#define VFSTORFSM(VFSP)	((struct rfsmnt *)(VFSP->vfs_data))


/* Flag bits passed to the mount system call--values are part of RFS protocol
   and thus cannot be changed */
#define MS_RDONLY       0x1     /* read only bit */
#define MS_FSS          0x2     /* FSS (4-argument) mount */
#define MS_CACHE        0x8     /* RFS client caching */


/* Server mount table stuff */
/* #ident	"@(#)kern-port:sys/mount.h	10.10" */


/* 
 * Server mount structure. 
 * Search the table looking for (sysid, rootvnode) pair.
 */

struct srmnt {
	sysid_t sr_sysid;
	char    sr_flags;
	struct  vnode *sr_rootvnode;	/* vnode on server's machine */
	int     sr_mntindx;		/* mount index of requestor  */
	int 	sr_refcnt;		/* number of open files      */
	int	sr_dotdot;		/* for lbin mount, so .. works */
	long	sr_bcount;		/* Server i/o count */
	int	sr_slpcnt;
} ;

/* sr_bcount unit size */
#define RFS_BKSIZE  1024
#ifdef KERNEL
extern struct srmnt *srmount;
extern int nsrmount;
#endif

/* Return values if crossing mount point */
#define DOT_DOT	0x1

#endif /*!_rfs_rfs_mnt_h*/
