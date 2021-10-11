/*	@(#)mount.h 1.1 92/07/30 SMI	*/

/*
 * mount options
 */

#ifndef	_sys_mount_h
#define	_sys_mount_h

#define	M_RDONLY	0x01	/* mount fs read only */
#define	M_NOSUID	0x02	/* mount fs with setuid not allowed */
#define	M_NEWTYPE	0x04	/* use type string instead of int */
#define	M_GRPID		0x08	/* Old BSD group-id on create */
#define	M_REMOUNT	0x10	/* change options on an existing mount */
#define	M_NOSUB		0x20	/* Disallow mounts beneath this mount */
#define	M_MULTI		0x40	/* Do multi-component lookup on files */
#define	M_SYS5		0x80	/* Mount with Sys 5-specific semantics */

#ifdef	KERNEL
/*
 * File system types, these correspond to entries in fsconf
 */
#define	MOUNT_UFS	1
#define	MOUNT_NFS	2
#define	MOUNT_PC	3
#define	MOUNT_LO	4
#define	MOUNT_TFS	5
#define	MOUNT_TMP	6
#define	MOUNT_MAXTYPE	7
#endif	KERNEL

struct	ufs_args {
	char	*fspec;
};

#ifdef	NFSCLIENT
#include <sys/pathconf.h> 			/* static pathconf kludge */

struct	nfs_args {
	struct sockaddr_in	*addr;		/* file server address */
	caddr_t			fh;		/* File handle to be mounted */
	int			flags;		/* flags */
	int			wsize;		/* write size in bytes */
	int			rsize;		/* read size in bytes */
	int			timeo;		/* initial timeout in .1 secs */
	int			retrans;	/* times to retry send */
	char			*hostname;	/* server's hostname */
	int			acregmin;	/* attr cache file min secs */
	int			acregmax;	/* attr cache file max secs */
	int			acdirmin;	/* attr cache dir min secs */
	int			acdirmax;	/* attr cache dir max secs */
	char			*netname;	/* server's netname */
	struct pathcnf		*pathconf;	/* static pathconf kludge */
};

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT	0x001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE	0x002	/* set write size */
#define	NFSMNT_RSIZE	0x004	/* set read size */
#define	NFSMNT_TIMEO	0x008	/* set initial timeout */
#define	NFSMNT_RETRANS	0x010	/* set number of request retrys */
#define	NFSMNT_HOSTNAME	0x020	/* set hostname for error printf */
#define	NFSMNT_INT	0x040	/* allow interrupts on hard mount */
#define	NFSMNT_NOAC	0x080	/* don't cache attributes */
#define	NFSMNT_ACREGMIN	0x0100	/* set min secs for file attr cache */
#define	NFSMNT_ACREGMAX	0x0200	/* set max secs for file attr cache */
#define	NFSMNT_ACDIRMIN	0x0400	/* set min secs for dir attr cache */
#define	NFSMNT_ACDIRMAX	0x0800	/* set max secs for dir attr cache */
#define	NFSMNT_SECURE	0x1000	/* secure mount */
#define	NFSMNT_NOCTO	0x2000	/* no close-to-open consistency */
#define	NFSMNT_POSIX	0x4000	/* static pathconf kludge info */
#endif	NFSCLIENT

#ifdef	PCFS
struct	pc_args {
	char	*fspec;
};
#endif	PCFS

#ifdef	LOFS
struct	lo_args {
	char    *fsdir;
};
#endif	LOFS

#ifdef	RFS

struct	token {
	int	t_id;	 /* token id for differentiating multiple ckts	*/
	char	t_uname[64]; /* full domain name of machine, 64 = MAXDNAME */
};

struct	rfs_args {
	char    *rmtfs;		/* name of service (fs) */
	struct token *token;	/* identifier of remote mach */
};

/*
 * RFS mount option flags
 */
#define	RFS_RDONLY	0x001	/* read-only: passed with remote mnt request */
#endif	RFS

#endif	/* !_sys_mount_h */
