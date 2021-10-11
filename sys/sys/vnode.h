/*	@(#)vnode.h 1.1 92/07/30 SMI 	*/

/*
 * The vnode is the focus of all file activity in UNIX.
 * There is a unique vnode allocated for each active file, each current
 * directory, each mounted-on file, each mapped file, and the root.
 */

#ifndef _sys_vnode_h
#define _sys_vnode_h

/*
 * vnode types. VNON means no type.
 */
enum vtype 	{ VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VBAD, VFIFO };

struct vnode {
	u_short		v_flag;			/* vnode flags (see below) */
	u_short		v_count;		/* reference count */
	u_short		v_shlockc;		/* count of shared locks */
	u_short		v_exlockc;		/* count of exclusive locks */
	struct vfs	*v_vfsmountedhere; 	/* ptr to vfs mounted here */
	struct vnodeops	*v_op;			/* vnode operations */
	union {
		struct socket	*v_Socket;	/* unix ipc */
		struct stdata	*v_Stream;	/* stream */
		struct page	*v_Pages;	/* vnode pages list */
	} v_s;
	struct vfs	*v_vfsp;		/* ptr to vfs we are in */
	enum vtype	v_type;			/* vnode type */
	dev_t		v_rdev;			/* device (VCHR, VBLK) */
	long		*v_filocks;		/* File/Record locks ... */
	caddr_t		v_data;			/* private data for fs */
};
#define	v_stream	v_s.v_Stream
#define	v_socket	v_s.v_Socket
#define	v_pages		v_s.v_Pages

/*
 * vnode flags.
 */
#define	VROOT		0x01	/* root of its file system */
#define	VEXLOCK		0x02	/* exclusive lock */
#define	VSHLOCK		0x04	/* shared lock */
#define	VLWAIT		0x08	/* proc is waiting on shared or excl. lock */
#define	VNOCACHE	0x10	/* don't keep cache pages on vnode */
#define	VNOMAP		0x20	/* vnode cannot be mapped */
#define	VISSWAP		0x40	/* vnode is part of virtual swap device */

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
	int	(*vn_lockctl)();
	int	(*vn_fid)();
	int	(*vn_getpage)();
	int	(*vn_putpage)();
	int	(*vn_map)();
	int	(*vn_dump)();
	int	(*vn_cmp)();
	int	(*vn_realvp)();
	int	(*vn_cntl)();
};

#ifdef KERNEL

#define VOP_OPEN(VPP,F,C)		(*(*(VPP))->v_op->vn_open)(VPP, F, C)
#define VOP_CLOSE(VP,F,C,CR)		(*(VP)->v_op->vn_close)(VP,F,C,CR)
#define VOP_RDWR(VP,UIOP,RW,F,C)	(*(VP)->v_op->vn_rdwr)(VP,UIOP,RW,F,C)
#define VOP_IOCTL(VP,C,D,F,CR)		(*(VP)->v_op->vn_ioctl)(VP,C,D,F,CR)
#define VOP_SELECT(VP,W,C)		(*(VP)->v_op->vn_select)(VP,W,C)
#define VOP_GETATTR(VP,VA,C)		(*(VP)->v_op->vn_getattr)(VP,VA,C)
#define VOP_SETATTR(VP,VA,C)		(*(VP)->v_op->vn_setattr)(VP,VA,C)
#define VOP_ACCESS(VP,M,C)		(*(VP)->v_op->vn_access)(VP,M,C)
#define VOP_LOOKUP(VP,NM,VPP,C,PN,F)	(*(VP)->v_op->vn_lookup)(VP,NM,VPP,C,PN,F)
#define VOP_CREATE(VP,NM,VA,E,M,VPP,C)	(*(VP)->v_op->vn_create) \
						(VP,NM,VA,E,M,VPP,C)
#define VOP_REMOVE(VP,NM,C)		(*(VP)->v_op->vn_remove)(VP,NM,C)
#define VOP_LINK(VP,TDVP,TNM,C)		(*(TDVP)->v_op->vn_link)(VP,TDVP,TNM,C)
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
#define VOP_LOCKCTL(VP,LD,CMD,C,ID)	(*(VP)->v_op->vn_lockctl) \
						(VP,LD,CMD,C,ID)
#define VOP_FID(VP, FIDPP)		(*(VP)->v_op->vn_fid)(VP, FIDPP)
#define VOP_GETPAGE(VP,OF,SZ,PR,PL,PS,SG,A,RW,C)\
					(*(VP)->v_op->vn_getpage) \
						(VP,OF,SZ,PR,PL,PS,SG,A,RW,C)
#define VOP_PUTPAGE(VP,OF,SZ,FL,C)	(*(VP)->v_op->vn_putpage)(VP,OF,SZ,FL,C)
#define VOP_MAP(VP,OF,AS,A,SZ,P,MP,FL,C)\
					(*(VP)->v_op->vn_map) \
						(VP,OF,AS,A,SZ,P,MP,FL,C)
#define VOP_DUMP(VP, ADDR, BN, CT)	(*(VP)->v_op->vn_dump)(VP, ADDR, BN, CT)
#define VOP_CMP(VP1, VP2)		(*(VP1)->v_op->vn_cmp)(VP1, VP2)
#define VOP_REALVP(VP, VPP)		(*(VP)->v_op->vn_realvp)(VP, VPP)
#define VOP_CNTL(VP,CMD,IND,OUTD,INF,OUTF)	(*(VP)->v_op->vn_cntl)\
						(VP,CMD,IND,OUTD,INF,OUTF)

/*
 * Flags for above
 */
#define IO_UNIT		0x01		/* do io as atomic unit for VOP_RDWR */
#define IO_APPEND	0x02		/* append write for VOP_RDWR */
#define IO_SYNC		0x04		/* sync io for VOP_RDWR */
#define LOOKUP_DIR	0x08		/* want parent dir. vnode, VOP_LOOKUP */
#define IO_NDELAY	0x10		/* No blocking, for VOP_RDWR */
/*
 * flags for VOP_CNTL
 */
#define CNTL_INT32	0x00000001	/* data is a 4 byte int */

#endif KERNEL

/*
 * Vnode attributes.  A field value of -1		
 * represents a field whose value is unavailable
 * (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
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
 * Compare two vnodes for equality.
 * This macro should always be used,
 * rather than calling VOP_CMP directly.
 */
#define VN_CMP(VP1,VP2)		((VP1) == (VP2) ? 1 : \
	((VP1) && (VP2) && ((VP1)->v_op == (VP2)->v_op) ? VOP_CMP(VP1,VP2) : 0))

/*
 * Flags for above
 */
enum rm		{ FILE, DIRECTORY };		/* rmdir or rm (remove) */
enum symfollow	{ NO_FOLLOW, FOLLOW_LINK };	/* follow symlinks (lookuppn) */
enum vcexcl	{ NONEXCL, EXCL};		/* (non)excl create (create) */

/*
 * Flags passed to namesetattr().
 */
#define	ATTR_UTIME	0x01		/* non-default utime(2) request */

/*
 * Global vnode data.
 */
extern struct vnode	*rootdir;		/* root (i.e. "/") vnode */
extern enum vtype	mftovt_tab[];
#define	MFMT		0170000			/* type of file */
#define MFTOVT(M)	(mftovt_tab[((M) & MFMT) >> 13])
extern long fake_vno;		/* prototype for fake vnode numbers used */
				/* by sockets and specfs       		 */
/*
 * Kernel tracing hooks for various vnode operations
 */
#ifdef TRACE
#include <sys/trace.h>
#undef VOP_LOOKUP
#define VOP_LOOKUP(VP,NM,VPP,C,PN,F)	\
(traceval = (*(VP)->v_op->vn_lookup)(VP,NM,VPP,C,PN,F), \
 traceval ? 0 : \
 trace6(TR_VN_LOOKUP,*(VPP),trs(NM,0),trs(NM,1),trs(NM,2),trs(NM,3),trs(NM,4)),\
 traceval)
#undef VOP_CREATE
#define VOP_CREATE(VP,NM,VA,E,M,VPP,C)	\
(traceval = (*(VP)->v_op->vn_create)(VP,NM,VA,E,M,VPP,C), \
 traceval ? 0 : \
 trace6(TR_VN_CREATE,*(VPP),trs(NM,0),trs(NM,1),trs(NM,2),trs(NM,3),trs(NM,4)),\
 traceval)
#undef VOP_MKDIR
#define VOP_MKDIR(VP,NM,VA,VPP,C)	\
(traceval = (*(VP)->v_op->vn_mkdir)(VP,NM,VA,VPP,C), \
 traceval ? 0 : \
 trace6(TR_VN_MKDIR,*(VPP),trs(NM,0),trs(NM,1),trs(NM,2),trs(NM,3),trs(NM,4)),\
 traceval)
#undef VOP_INACTIVE
#define VOP_INACTIVE(VP,C)	\
(traceval = (*(VP)->v_op->vn_inactive)(VP,C), \
 traceval ? 0 : trace1(TR_VN_INACTIVE, VP),	\
 traceval)
#undef VN_RELE
#define VN_RELE(VP)	{ \
	vn_rele(VP); \
	(void) trace1(TR_VN_RELE, VP); \
}
#endif TRACE

#endif KERNEL

#endif /*!_sys_vnode_h*/
