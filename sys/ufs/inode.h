/* @(#)inode.h 1.1 92/07/30 SMI; from UCB 4.24 83/07/01 */

/*
 * The I node is the focus of all local file activity in UNIX.
 * There is a unique inode allocated for each active file,
 * each current directory, each mounted-on file, each mapping,
 * and the root.  An inode is `named' by its dev/inumber pair.
 * Data in icommon is read in from permanent inode on volume.
 */

#ifndef	_ufs_inode_h
#define	_ufs_inode_h

#define	NDADDR	12		/* direct addresses in inode */
#define	NIADDR	3		/* indirect addresses in inode */
#define	FSL_SIZE (NDADDR+NIADDR-1) * sizeof (daddr_t)
				/* max fast symbolic name length is 56 */

struct	inode {
	struct	inode *i_chain[2];	/* must be first */
	struct	vnode i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	u_short	i_flag;
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	off_t	i_diroff;	/* offset in dir, where we found last entry */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct	dquot *i_dquot;	/* quota structure controlling this file */
	long	i_owner;	/* proc index of process locking inode */
	long	i_count;	/* number of inode locks for i_owner */
	union {
		daddr_t	if_nextr;	/* next byte read offset (read-ahead) */
		struct	socket *is_socket;
	} i_un;
	struct	{
		struct inode  *if_freef;	/* free list forward */
		struct inode **if_freeb;	/* free list back */
	} i_fr;
	struct 	icommon {
		u_short	ic_mode;	/*  0: mode and type of file */
		short	ic_nlink;	/*  2: number of links to file */
		uid_t	ic_uid;		/*  4: owner's user id */
		gid_t	ic_gid;		/*  6: owner's group id */
		quad	ic_size;	/*  8: number of bytes in file */
#ifdef	KERNEL
		struct timeval ic_atime; /* 16: time last accessed */
		struct timeval ic_mtime; /* 24: time last modified */
		struct timeval ic_ctime; /* 32: last time inode changed */
#else
		time_t	ic_atime;	/* 16: time last accessed */
		long	ic_atspare;
		time_t	ic_mtime;	/* 24: time last modified */
		long	ic_mtspare;
		time_t	ic_ctime;	/* 32: last time inode changed */
		long	ic_ctspare;
#endif
		daddr_t	ic_db[NDADDR];	/* 40: disk block addresses */
		daddr_t	ic_ib[NIADDR];	/* 88: indirect blocks */
		long	ic_flags;	/* 100: status, currently unused */
		long	ic_blocks;	/* 104: blocks actually held */
		long	ic_gen;		/* 108: generation number */
		/*
		 * XXX - the disk spares were used to avoid changing
		 * the size of the incore inode in a minor release.
		 * Fix for 5.0 release.
		 * Also remove the code in iget, iupdat that clears them.
		 */
		long	ic_delaylen;	/* 112: delayed writes, units=bytes */
		long	ic_delayoff;	/* 116: where we started delaying */
		long	ic_nextrio;	/* 120: where to start the next clust */
		long	ic_writes;	/* 124: number of outstanding writes */
	} i_ic;
};

struct	dinode {
	union {
		struct	icommon di_icom;
		char	di_size[128];
	} di_un;
};

#define	i_mode		i_ic.ic_mode
#define	i_nlink		i_ic.ic_nlink
#define	i_uid		i_ic.ic_uid
#define	i_gid		i_ic.ic_gid
/* ugh! -- must be fixed */
#if	defined(vax) || defined(i386)
#define	i_size		i_ic.ic_size.val[0]
#endif
#if	defined(mc68000) || defined(sparc)
#define	i_size		i_ic.ic_size.val[1]
#endif
#define	i_db		i_ic.ic_db
#define	i_ib		i_ic.ic_ib
#define	i_atime		i_ic.ic_atime
#define	i_mtime		i_ic.ic_mtime
#define	i_ctime		i_ic.ic_ctime
#define	i_blocks	i_ic.ic_blocks
#define	i_rdev		i_ic.ic_db[0]
#define	i_gen		i_ic.ic_gen
#define	i_nextr		i_un.if_nextr
#define	i_socket	i_un.is_socket
#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define	i_freef		i_fr.if_freef
#define	i_freeb		i_fr.if_freeb
#define	i_delayoff	i_ic.ic_delayoff	/* XXX */
#define	i_delaylen	i_ic.ic_delaylen	/* XXX */
#define	i_nextrio	i_ic.ic_nextrio		/* XXX */
#define	i_writes	i_ic.ic_writes		/* XXX */

#define	di_ic		di_un.di_icom
#define	di_mode		di_ic.ic_mode
#define	di_nlink	di_ic.ic_nlink
#define	di_uid		di_ic.ic_uid
#define	di_gid		di_ic.ic_gid
#if	defined(vax) || defined(i386)
#define	di_size		di_ic.ic_size.val[0]
#endif
#if	defined(mc68000) || defined(sparc)
#define	di_size		di_ic.ic_size.val[1]
#endif
#define	di_db		di_ic.ic_db
#define	di_ib		di_ic.ic_ib
#define	di_atime	di_ic.ic_atime
#define	di_mtime	di_ic.ic_mtime
#define	di_ctime	di_ic.ic_ctime
#define	di_rdev		di_ic.ic_db[0]
#define	di_blocks	di_ic.ic_blocks
#define	di_gen		di_ic.ic_gen

/* flags */
#define	ILOCKED		0x0001		/* inode is locked */
#define	IUPD		0x0002		/* file has been modified */
#define	IACC		0x0004		/* inode access time to be updated */
#define	IMOD		0x0008		/* inode has been modified */
#define	IWANT		0x0010		/* some process waiting on lock */
#define	ISYNC		0x0020		/* do all allocation synchronously */
#define	ICHG		0x0040		/* inode has been changed */
#define	ILWAIT		0x0080		/* someone waiting on file lock */
#define	IREF		0x0100		/* inode is being referenced */
#define	INOACC		0x0200		/* no access time update in getpage */
#define	IMODTIME	0x0400		/* mod time already set */
#define	IFASTSYMLNK	0x0800		/* fast symbolic link */
#define	IMODACC		0x1000		/* only access time changed */

/* modes */
#define	IFMT		0170000		/* type of file */
#define	IFIFO		0010000		/* named pipe (fifo) */
#define	IFCHR		0020000		/* character special */
#define	IFDIR		0040000		/* directory */
#define	IFBLK		0060000		/* block special */
#define	IFREG		0100000		/* regular */
#define	IFLNK		0120000		/* symbolic link */
#define	IFSOCK		0140000		/* socket */

#define	ISUID		04000		/* set user id on execution */
#define	ISGID		02000		/* set group id on execution */
#define	ISVTX		01000		/* save swapped text even after use */
#define	IREAD		0400		/* read, write, execute permissions */
#define	IWRITE		0200
#define	IEXEC		0100

/* Statistics on inodes */
struct	instats {
	int in_hits;			/* Cache hits */
	int in_misses;			/* Cache misses */
	int in_malloc;			/* kmem_alloce'd */
	int in_mfree;			/* kmem_free'd */
	int in_maxsize;			/* Largest size reached by cache */
	int in_frfront;			/* # put at front of freelist */
	int in_frback;			/* # put at back of freelist */
	int in_dnlclook;		/* # examined in dnlc */
	int in_dnlcpurge;		/* # purged from dnlc */
};

#ifdef	KERNEL
extern int ninode;			/* high-water mark for inode cache */

extern struct vnodeops ufs_vnodeops;	/* vnode operations for ufs */

extern ino_t dirpref();
extern daddr_t blkpref();

/*
 * A struct fbuf is used to get a mapping to part of a file using the
 * segkmap facilities.  After you get a mapping, you can fbrelse() it
 * (giving a seg code to pass back to segmap_release), you can fbwrite()
 * it (causes a synchronous write back using inode mapping information),
 * or you can fbiwrite it (causing indirect synchronous write back to
 * the block number given without using inode mapping information).
 */

struct	fbuf {
	addr_t	fb_addr;
	u_int	fb_count;
};

extern int fbread(/* vp, off, len, rw, fbpp */);
extern void fbzero(/* vp, off, len, fbpp */);
extern int fbwrite(/* fb */);
extern int fbiwrite(/* fb, ip, bn */);
extern void fbrelse(/* fb, rw */);

/*
 * Convert between inode pointers and vnode pointers
 */
#define	VTOI(VP)	((struct inode *)(VP)->v_data)
#define	ITOV(IP)	((struct vnode *)&(IP)->i_vnode)

/*
 * Convert inode pointer to file system, mountpoint, or lockfs
 */
#define	ITOF(IP)	((IP)->i_fs)
#define	ITOM(IP)	((struct mount *)(ITOV(IP)->v_vfsp->vfs_data))
#define	ITOU(IP)	(ITOM(IP)->m_ul)

/*
 * Convert between vnode types and inode formats
 */
extern enum vtype	iftovt_tab[];
extern int		vttoif_tab[];
#define	IFTOVT(M)	(iftovt_tab[((M) & IFMT) >> 13])
#define	VTTOIF(T)	(vttoif_tab[(int)(T)])

#define	MAKEIMODE(T, M)	(VTTOIF(T) | (M))

/*
 * Lock and unlock inodes.
 */
#define	ILOCK(ip) { \
	while (((ip)->i_flag & ILOCKED) && \
	    (ip)->i_owner != uniqpid()) { \
		(ip)->i_flag |= IWANT; \
		(void) sleep((caddr_t)(ip), PINOD); \
	} \
	(ip)->i_owner = uniqpid(); \
	(ip)->i_count++; \
	(ip)->i_flag |= ILOCKED; \
	masterprocp->p_swlocks++; \
}

#define	IUNLOCK(ip) { \
	if (--(ip)->i_count < 0) \
		panic("IUNLOCK"); \
	masterprocp->p_swlocks--; \
	if ((ip)->i_count == 0) { \
		(ip)->i_flag &= ~ILOCKED; \
		if ((ip)->i_flag & IWANT) { \
			(ip)->i_flag &= ~IWANT; \
			wakeup((caddr_t)(ip)); \
		} \
	} \
}

#define	IUPDAT(ip, waitfor) { \
	if (ip->i_flag & (IUPD|IACC|ICHG|IMOD|IMODACC)) \
		iupdat(ip, waitfor); \
}

/*
 * Mark an inode with the current (unique) timestamp.
 */
struct	timeval iuniqtime;

#define	IMARK(ip) { \
	if (time.tv_sec > iuniqtime.tv_sec || \
	    time.tv_usec > iuniqtime.tv_usec) { \
		iuniqtime = time; \
	} else { \
		iuniqtime.tv_usec++; \
	} \
	if ((ip)->i_flag & IACC) \
		(ip)->i_atime = iuniqtime; \
	if ((ip)->i_flag & IUPD) { \
		(ip)->i_mtime = iuniqtime; \
		(ip)->i_flag |= IMODTIME; \
	} \
	if ((ip)->i_flag & ICHG) { \
		ip->i_diroff = 0; \
		(ip)->i_ctime = iuniqtime; \
	} \
}

#define	ITIMES(ip) { \
	if ((ip)->i_flag & (IUPD|IACC|ICHG)) { \
		if (((ip)->i_flag & (IUPD|IACC|ICHG)) == IACC) \
			(ip)->i_flag |= IMODACC; \
		else \
			(ip)->i_flag |= IMOD; \
		IMARK(ip); \
		(ip)->i_flag &= ~(IACC|IUPD|ICHG); \
	} \
}

/*
 * Allocate the specified block in the inode
 * and make sure any in-core pages are initialized.
 */
#define	BMAPALLOC(ip, lbn, size) \
	bmap_write((ip), (lbn), 0, (daddr_t*)0, (int*)0, (size), 0)

#define	ESAME	(-1)		/* trying to rename linked files (special) */

/*
 * Check that file is owned by current user or user is su.
 */
#define	OWNER(CR, IP)	(((CR)->cr_uid == (IP)->i_uid)? 0: (suser()? 0: EPERM))

#define	UFS_HOLE	(daddr_t)-1	/* value used when no block allocated */

/*
 * enums
 */
enum	de_op	{ DE_CREATE, DE_LINK, DE_RENAME };	/* direnter ops */

/*
 * This overlays the fid structure (see vfs.h)
 */
struct	ufid {
	u_short	ufid_len;
	ino_t	ufid_ino;
	long	ufid_gen;
};

#define	INOHSZ	512
#if	((INOHSZ&(INOHSZ-1)) == 0)
#define	INOHASH(dev, ino)	(((dev)+(ino))&(INOHSZ-1))
#else
#define	INOHASH(dev, ino)	(((unsigned)((dev)+(ino)))%INOHSZ)
#endif

union ihead {				/* inode LRU cache, Chris Maltby */
	union  ihead *ih_head[2];
	struct inode *ih_chain[2];
};

extern union ihead ihead[];

#endif	KERNEL

#endif	/* !_ufs_inode_h */
