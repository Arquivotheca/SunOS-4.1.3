/*	@(#)lockfs.h 1.1 92/07/30 SMI	*/

#ifndef _sys_lockfs_h
#define	_sys_lockfs_h

struct lockfs {
	u_long		lf_lock;	/* desired lock */
	u_long		lf_flags;	/* misc flags */
	u_long		lf_key;		/* lock key */
	u_long		lf_comlen;	/* length of comment */
	caddr_t		lf_comment;	/* address of comment */
};

/*
 * lf_lock and lf_locking
 */
#define	LOCKFS_ULOCK	0	/* unlock */
#define	LOCKFS_WLOCK	1	/* write  lock */
#define	LOCKFS_NLOCK	2	/* name   lock */
#define	LOCKFS_DLOCK	3	/* delete lock */
#define	LOCKFS_HLOCK	4	/* hard   lock */
#define	LOCKFS_MAXLOCK	5	/* maximum lock number */

/*
 * lf_flags
 */
#define	LOCKFS_BUSY	0x00000001	/* lock is being set */
#define	LOCKFS_MOD	0x00000002	/* file system modified */

#define	LOCKFS_MAXCOMMENTLEN	1024	/* maximum comment length */

/*
 * some nice checking macros
 */
#define	LOCKFS_IS_BUSY(LF)	((LF)->lf_flags & LOCKFS_BUSY)
#define	LOCKFS_IS_MOD(LF)	((LF)->lf_flags & LOCKFS_MOD)

#define	LOCKFS_CLR_BUSY(LF)	((LF)->lf_flags &= ~LOCKFS_BUSY)
#define	LOCKFS_CLR_MOD(LF)	((LF)->lf_flags &= ~LOCKFS_MOD)

#define	LOCKFS_SET_MOD(LF)	((LF)->lf_flags |= LOCKFS_MOD)
#define	LOCKFS_SET_BUSY(LF)	((LF)->lf_flags |= LOCKFS_BUSY)

#define	LOCKFS_IS_WLOCK(LF)	((LF)->lf_lock == LOCKFS_WLOCK)
#define	LOCKFS_IS_HLOCK(LF)	((LF)->lf_lock == LOCKFS_HLOCK)
#define	LOCKFS_IS_ULOCK(LF)	((LF)->lf_lock == LOCKFS_ULOCK)

#endif /* !_sys_lockfs_h */
