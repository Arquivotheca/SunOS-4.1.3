/*	@(#)vfs_stat.h 1.1 92/07/30 SMI 	*/

/*
 * The stat structure is a two dimentional array. The major index is
 * the op number defined below, the minor index is VS_HIT or VS_MISS.
 */

#ifndef _sys_vfs_stat_h
#define _sys_vfs_stat_h

#define	VS_CALL		0	/* Op called */
#define	VS_MISS		1	/* Cache miss */

/*
 * VFS OPS
 */
#define	VS_ROOT		0
#define	VS_STATFS	1
#define	VS_SYNC		2
#define	VS_VGET		3

/*
 * Vnode ops
 */
#define VS_OPEN		4
#define VS_CLOSE	5
#define VS_READ		6
#define VS_WRITE	7
#define VS_IOCTL	8
#define VS_SELECT	9
#define VS_GETATTR	10
#define VS_SETATTR	11
#define VS_ACCESS	12
#define VS_LOOKUP	13
#define VS_CREATE	14
#define VS_REMOVE	15
#define VS_LINK		16
#define VS_RENAME	17
#define VS_MKDIR	18
#define VS_RMDIR	19
#define VS_READDIR	20
#define VS_SYMLINK	21
#define VS_READLINK	22
#define VS_FSYNC	23
#define VS_INACTIVE	24
#define VS_LOCKCTL	25
#define VS_FID		26
#define VS_GETPAGE	27
#define VS_PUTPAGE	28
#define VS_MAP		29
#define VS_CMP		30
#define VS_REALVP	31
#define VS_CNTL		32

#define	VS_NOPS		33

#ifndef KERNEL
char *vs_opnames[VS_NOPS] = {
	"root", "statfs", "sync", "vget", "open", "close", "read", "write",
	"ioctl", "select", "getattr", "setattr", "access", "lookup", "create",
	"remove", "link", "rename", "mkdir", "rmdir", "readdir", "symlink",
	"readlink", "fsync", "inactive", "lockctl", "fid", "getpage",
	"putpage", "map", "cmp", "realvp", "cntl"
};
#endif

struct vfsstats {
	time_t	vs_time;
	int	vs_counts[VS_NOPS][2];
};

#ifdef VFSSTATS
#define	VFS_RECORD(vfs, op, res) \
{ \
	if ((vfs)->vfs_stats) \
		((struct vfsstats *)(vfs)->vfs_stats)->vs_counts[op][res]++; \
}
#else
#define	VFS_RECORD(vfs, op, res)
#endif

#endif /*!_sys_vfs_stat_h*/
