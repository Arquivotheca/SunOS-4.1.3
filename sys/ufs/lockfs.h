/*	@(#)lockfs.h 1.1 92/07/30 SMI	*/
#ifndef _ufs_lockfs_h
#define	_ufs_lockfs_h

/*
 * ufs lockfs structure.
 *	one per mount struct, allocated at mount time
 */
struct	ulockfs {
	u_long		ul_vacount[VA_MAX];	/* vop access counts */
	u_long		ul_vamask;		/* vop access mask */
	u_long		ul_flags;		/* flags */
	u_long		ul_hold;		/* hold count */
	struct proc	*ul_sbowner;		/* superblock owner */
	struct lockfs	ul_lockfs;		/* ioctl lockfs struct */
};

/*
 * ul_flags
 */
#define	ULOCKFS_VAWANT		0x00000001	/* vop access sleeper */
#define	ULOCKFS_NOIACC		0x00000002	/* don't keep access times */
#define	ULOCKFS_NOIDEL		0x00000004	/* don't free deleted files */
#define	ULOCKFS_FUMOUNT		0x00000008	/* forcibly unmounting */
#define	ULOCKFS_WANT		0x00000010	/* wanted */

/*
 * misc macros
 */
#define	ULOCKFS_IS_NOIACC(UL)	((UL)->ul_flags & ULOCKFS_NOIACC)
#define	ULOCKFS_IS_NOIDEL(UL)	((UL)->ul_flags & ULOCKFS_NOIDEL)
#define	UTOL(UL)		(&(UL)->ul_lockfs)

#endif /* !_ufs_lockfs_h */
