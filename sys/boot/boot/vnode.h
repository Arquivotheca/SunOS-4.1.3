/*	@(#)vnode.h 1.1 92/07/30 SMI	*/

/*
 * The vnode is the focus of all file activity in UNIX.
 * There is a unique vnode allocated for each active file,
 * each current directory, each mounted-on file, text file, and the root.
 */

/*
 * vnode types. VNON means no type.
 */
enum vtype 	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VBAD, VFIFO };

struct vnode {
	u_short		v_flag;			/* vnode flags (see below)*/
	u_short		v_count;		/* reference count */
	u_short		v_shlockc;		/* count of shared locks */
	u_short		v_exlockc;		/* count of exclusive locks */
	struct vfs	*v_vfsmountedhere; 	/* ptr to vfs mounted here */
	struct vnodeops	*v_op;			/* vnode operations */
	union {
		struct socket	*v_Socket;	/* unix ipc */
		struct stdata	*v_Stream;	/* stream */
	} v_s;
	struct vfs	*v_vfsp;		/* ptr to vfs we are in */
	enum vtype	v_type;			/* vnode type */
	dev_t		v_rdev;			/* device (VCHR, VBLK) */
	caddr_t		v_data;			/* private data for fs */
};
#define	v_stream	v_s.v_Stream
#define	v_socket	v_s.v_Socket

/*
 * vnode flags.
 */
#define	VROOT		0x01	/* root of its file system */
#define VTEXT		0x02	/* vnode is a pure text prototype */
#define	VEXLOCK		0x10	/* exclusive lock */
#define	VSHLOCK		0x20	/* shared lock */
#define	VLWAIT		0x40	/* proc is waiting on shared or excl. lock */
#define	VTEXTMOD	0x80	/* text has been modified (e.g. ptrace) */

/*
 * Operations on vnodes.
 */
struct vnodeops {
	int	(*vn_open)();
	int	(*vn_close)();
	int	(*vn_rdwr)();
	int	(*vn_ioctl)();
	int	(*vn_select)();
	int	(*vn_getattr)();
	int	(*vn_setattr)();
	int	(*vn_access)();
	int	(*vn_lookup)();
	int	(*vn_create)();
	int	(*vn_remove)();
	int	(*vn_link)();
	int	(*vn_rename)();
	int	(*vn_mkdir)();
	int	(*vn_rmdir)();
	int	(*vn_readdir)();
	int	(*vn_symlink)();
	int	(*vn_readlink)();
	int	(*vn_fsync)();
	int	(*vn_inactive)();
	int	(*vn_bmap)();
	int	(*vn_strategy)();
	int	(*vn_bread)();
	int	(*vn_brelse)();
	int	(*vn_lockctl)();
	int	(*vn_fid)();
	int	(*vn_dump)();
};

#ifdef KERNEL

#define VOP_OPEN(VPP,F,C)		(*(*(VPP))->v_op->vn_open)(VPP, F, C)
#define VOP_CLOSE(VP,F,C)		(*(VP)->v_op->vn_close)(VP,F,C)
#define VOP_RDWR(VP,UIOP,RW,F,C)	(*(VP)->v_op->vn_rdwr)(VP,UIOP,RW,F,C)
#define VOP_IOCTL(VP,C,D,F,CR)		(*(VP)->v_op->vn_ioctl)(VP,C,D,F,CR)
#define VOP_SELECT(VP,W,C)		(*(VP)->v_op->vn_select)(VP,W,C)
#define VOP_GETATTR(VP,VA,C)		(*(VP)->v_op->vn_getattr)(VP,VA,C)
#define VOP_SETATTR(VP,VA,C)		(*(VP)->v_op->vn_setattr)(VP,VA,C)
#define VOP_ACCESS(VP,M,C)		(*(VP)->v_op->vn_access)(VP,M,C)
#define VOP_LOOKUP(VP,NM,VPP,C)		(*(VP)->v_op->vn_lookup)(VP,NM,VPP,C)
#define VOP_CREATE(VP,NM,VA,E,M,VPP,C)	(*(VP)->v_op->vn_create) \
						(VP,NM,VA,E,M,VPP,C)
#define VOP_REMOVE(VP,NM,C)		(*(VP)->v_op->vn_remove)(VP,NM,C)
#define VOP_LINK(VP,TDVP,TNM,C)		(*(VP)->v_op->vn_link)(VP,TDVP,TNM,C)
#define VOP_RENAME(VP,NM,TDVP,TNM,C)	(*(VP)->v_op->vn_rename) \
						(VP,NM,TDVP,TNM,C)
#define VOP_MKDIR(VP,NM,VA,VPP,C)	(*(VP)->v_op->vn_mkdir)(VP,NM,VA,VPP,C)
#define VOP_RMDIR(VP,NM,C)		(*(VP)->v_op->vn_rmdir)(VP,NM,C)
#define VOP_READDIR(VP,UIOP,C)		(*(VP)->v_op->vn_readdir)(VP,UIOP,C)
#define VOP_SYMLINK(VP,LNM,VA,TNM,C)	(*(VP)->v_op->vn_symlink) \
						(VP,LNM,VA,TNM,C)
#define VOP_READLINK(VP,UIOP,C)		(*(VP)->v_op->vn_readlink)(VP,UIOP,C)
#define VOP_FSYNC(VP,C)			(*(VP)->v_op->vn_fsync)(VP,C)
#define VOP_INACTIVE(VP,C)		(*(VP)->v_op->vn_inactive)(VP,C)
#define VOP_BMAP(VP,BN,VPP,BNP)		(*(VP)->v_op->vn_bmap)(VP,BN,VPP,BNP)
#define VOP_STRATEGY(BP)		(*(BP)->b_vp->v_op->vn_strategy)(BP)
#define VOP_BREAD(VP,BN,BPP)		(*(VP)->v_op->vn_bread)(VP,BN,BPP)
#define VOP_BRELSE(VP,BP)		(*(VP)->v_op->vn_brelse)(VP,BP)
#define VOP_LOCKCTL(VP,LD,CMD,C)	(*(VP)->v_op->vn_lockctl) \
						(VP,LD,CMD,C)
#define VOP_FID(VP, FIDPP)		(*(VP)->v_op->vn_fid)(VP, FIDPP)
#define VOP_DUMP(VP, ADDR, BN, CT)	(*(VP)->v_op->vn_dump)(VP, ADDR, BN, CT)

/*
 * flags for above
 */
#define IO_UNIT		0x01		/* do io as atomic unit for VOP_RDWR */
#define IO_APPEND	0x02		/* append write for VOP_RDWR */
#define IO_SYNC		0x04		/* sync io for VOP_RDWR */

#endif

/*
 * Vnode attributes.  A field value of -1
 * represents a field whose value is unavailable
 * (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	short		va_uid;		/* owner user id */
	short		va_gid;		/* owner group id */
	long		va_fsid;	/* file system id (dev for now) */
	long		va_nodeid;	/* node id */
	short		va_nlink;	/* number of references to file */
	u_long		va_size;	/* file size in bytes (quad?) */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timeval	va_atime;	/* time of last access */
	struct timeval	va_mtime;	/* time of last modification */
	struct timeval	va_ctime;	/* time file ``created */
	dev_t		va_rdev;	/* device the file represents */
	long		va_blocks;	/* kbytes of disk space held by file */
};

/*
 *  Modes. Some values same as Ixxx entries from inode.h for now
 */
#define	VSUID	04000		/* set user id on execution */
#define	VSGID	02000		/* set group id on execution */
#define VSVTX	01000		/* save swapped text even after use */
#define	VREAD	0400		/* read, write, execute permissions */
#define	VWRITE	0200
#define	VEXEC	0100

#ifdef KERNEL
/*
 * public vnode manipulation functions
 */
extern int vn_open();			/* open vnode */
extern int vn_create();			/* creat/mkdir vnode */
extern int vn_rdwr();			/* read or write vnode */
extern int vn_close();			/* close vnode */
extern void vn_rele();			/* release vnode */
extern int vn_link();			/* make hard link */
extern int vn_rename();			/* rename (move) */
extern int vn_remove();			/* remove/rmdir */
extern void vattr_null();		/* set attributes to null */
extern int getvnodefp();		/* get fp from vnode fd */

#define VN_HOLD(VP)	{ \
	(VP)->v_count++; \
}

#define VN_RELE(VP)	{ \
	vn_rele(VP); \
}

#define VN_INIT(VP, VFSP, TYPE, DEV)	{ \
	(VP)->v_flag = 0; \
	(VP)->v_count = 1; \
	(VP)->v_shlockc = (VP)->v_exlockc = 0; \
	(VP)->v_vfsp = (VFSP); \
	(VP)->v_type = (TYPE); \
	(VP)->v_rdev = (DEV); \
	(VP)->v_socket = 0; \
}

/*
 * flags for above
 */
enum rm		{ FILE, DIRECTORY };		/* rmdir or rm (remove) */
enum symfollow	{ NO_FOLLOW, FOLLOW_LINK };	/* follow symlinks (lookuppn) */
enum vcexcl	{ NONEXCL, EXCL};		/* (non)excl create (create) */

/*
 * Global vnode data.
 */
extern struct vnode	*rootdir;		/* root (i.e. "/") vnode */
extern enum vtype	mftovt_tab[];
#define	MFMT		0170000		/* type of file */
#define MFTOVT(M)	(mftovt_tab[((M) & MFMT) >> 13])

#endif
