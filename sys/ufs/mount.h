/*	@(#)mount.h 1.1 92/07/30 SMI; from UCB 4.4 82/07/19	*/

#ifndef _ufs_mount_h
#define	_ufs_mount_h

/*
 * Mount structure.
 * One allocated on every ufs mount.
 * Used to find the super block.
 */
struct	mount {
	struct mount	*m_nxt;		/* linked list of all mounts */
	struct vfs	*m_vfsp;	/* vfs structure for this filesystem */
	struct vnode	*m_devvp;	/* vnode for block device mounted */
	struct buf	*m_bufp;	/* pointer to superblock */
	dev_t		m_dev;		/* device mounted */
	u_short		m_qflags;	/* QUOTA: filesystem flags */
	struct inode	*m_qinod;	/* QUOTA: pointer to quota file */
	u_long		m_btimelimit;	/* QUOTA: block time limit */
	u_long		m_ftimelimit;	/* QUOTA: file time limit */
	struct ulockfs	*m_ul;		/* FIOLFS: file system locking */
	u_long		m_dio;		/* FIODIO: file system delayed io */
};

#ifdef KERNEL
/*
 * Convert vfs ptr to mount ptr.
 */
#define	VFSTOM(VFSP)	((struct mount *)((VFSP)->vfs_data))

/*
 * m_dio bits
 */
#define	MDIO_ON		0x00000001		/* delayed IO is enabled */
#define	MDIO_LOCK	0x00000002		/* protects m_dio */

/*
 * mount table
 */
extern struct mount	*mounttab;

/*
 * Reader/writer mounttab lock.
 */
#define	MOUNTTAB_LOCKED	0x00000001	/* must be 1 for bundled vdconf.c */
#define	MOUNTTAB_WANTED	0x00000002	/* must be 2 for bundled vdconf.c */

extern int	mounttab_flags;		/* locking flags */
extern int	mounttab_rlock;		/* # of read locks */

/*
 * MLOCK is a blocking readlock (compatibility with bundled vdconf.c)
 */
#define	MLOCK() { \
	while (mounttab_flags & MOUNTTAB_LOCKED) { \
		mounttab_flags |= MOUNTTAB_WANTED; \
		(void) sleep((caddr_t)mounttab, PINOD); \
	} \
	mounttab_flags |= MOUNTTAB_LOCKED; \
}

/*
 * MWLOCK is a blocking writelock
 */
#define	MWLOCK() { \
	MLOCK(); \
	while (mounttab_rlock) { \
		mounttab_flags |= MOUNTTAB_WANTED; \
		(void) sleep((caddr_t)mounttab, PINOD); \
	} \
}

/*
 * MUNLOCK unlocks MLOCK or MWLOCK
 */
#define	MUNLOCK() { \
	mounttab_flags &= ~MOUNTTAB_LOCKED; \
	if (mounttab_flags & MOUNTTAB_WANTED) { \
		mounttab_flags &= ~MOUNTTAB_WANTED; \
		wakeup((caddr_t)mounttab); \
	} \
}

/*
 * MRLOCK is a nonblocking readlock
 */
#define	MRLOCK() { \
	while (mounttab_flags & MOUNTTAB_LOCKED) { \
		mounttab_flags |= MOUNTTAB_WANTED; \
		(void) sleep((caddr_t)mounttab, PINOD); \
	} \
	mounttab_rlock++; \
}

/*
 * MRULOCK unlocks a MRLOCK
 */
#define	MRULOCK() { \
	if (--mounttab_rlock == 0) \
		if (mounttab_flags & MOUNTTAB_WANTED) { \
			mounttab_flags &= ~MOUNTTAB_WANTED; \
			wakeup((caddr_t)mounttab); \
		} \
}

/*
 * Operations
 */
struct mount *getmp();
#endif KERNEL

#endif /*!_ufs_mount_h*/
