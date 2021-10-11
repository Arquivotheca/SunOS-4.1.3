/*	@(#)tfs_user.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifndef _NSE_TFS_USER_H
#define _NSE_TFS_USER_H

#include <sys/time.h>
#include <rpc/rpc.h>
#include <sys/vfs.h>
#include <errno.h>
#include <nfs/nfs.h>

/*
 * Arguments/results for TFS procedures which are callable by user
 * processes (as opposed to those callable by the kernel, which are in
 * tfs.h.)
 */

/*
 * Fhandle handed out by the user-level tfsd.  Overlays the fhandle_t
 * structure.
 */
#define TFS_FHMAXDATA	(NFS_FHSIZE - ((2 * sizeof(u_long)) + sizeof(time_t)))

struct tfsfh {
	u_long		fh_id;
	u_long		fh_parent_id;
	time_t		fh_timestamp;
	char		fh_data[TFS_FHMAXDATA];
};
#define TFS_FH(fh)	((struct tfsfh *)fh)

/*
 * Arguments for TFS_MOUNT & TFS_UMOUNT.
 *
 * Example values for the 'directory' and 'tfs_mount_pt' fields:
 *	tfs_mount_pt	/usr/src/nse
 *	directory	/nse/br.mumble/var-sun3/working/usr/src/nse
 */
typedef struct Tfs_mount_args {
	char		*environ;
	char		*tfs_mount_pt;	/* virtual name of mountpt */
	char		*directory;	/* dir. to mount on mountpt */
	char		*hostname;
	short		pid;
	short		writeable_layers;   /* # of writeable layers */
	short		back_owner;	    /* owner of files in back layers */
	bool_t		back_read_only;	    /* are back layers read-only? */
	char		*default_view;	    /* default value for '$view' */
	char		*conditional_view;  /* conditional value for '$view' */
} *Tfs_mount_args, Tfs_mount_args_rec;


typedef struct Tfs_unmount_args {
	char		*environ;
	char		*tfs_mount_pt;
	char		*hostname;
	short		pid;
} *Tfs_unmount_args, Tfs_unmount_args_rec;


/*
 * Old (version 1) mount & unmount args
 */
typedef struct Tfs_old_mount_args {
	char		*environ;
	char		*tfs_mount_pt;	/* virtual name of mountpt */
	char		*directory;	/* dir. to mount on mountpt */
	char		*hostname;
	int		pid;
	int		writeable_layers;
} *Tfs_old_mount_args, Tfs_old_mount_args_rec;


/*
 * Results of TFS_MOUNT request.
 */
typedef struct Tfs_mount_res {
	int		status;
	fhandle_t	fh;		/* root fhandle */
	int		port;		/* Port which will receive NFS
					 * requests.
					 */
	int		pid;		/* pid of TFS (for printf's) */
} *Tfs_mount_res, Tfs_mount_res_rec;


/*
 * Clients of the TFS specify which file to operate on in TFS-specific
 * operations (unwhiteout, getname, etc.)  by specifying the fhandle of the
 * file.  The fhandle of the file consists of the nodeid of the file and
 * the nodeid of the parent directory.
 */

typedef struct Tfs_fhandle {
	long		nodeid;
	long		parent_id;
} *Tfs_fhandle, Tfs_fhandle_rec;


/*
 * Arguments for routines which need a name.
 */
typedef struct Tfs_name_args {
	Tfs_fhandle_rec	fhandle;
	char		*name;
} *Tfs_name_args, Tfs_name_args_rec;


/*
 * Arguments for TFS_GET_WO (get white-out entries)
 */
typedef struct Tfs_get_wo_args {
	Tfs_fhandle_rec	fhandle;
	int		nbytes;		/* Maximum # of bytes to read */
	int		offset;		/* Offset into directory */
} *Tfs_get_wo_args, Tfs_get_wo_args_rec;


/*
 * Results of TFS_GET_WO
 */
typedef struct Tfs_get_wo_res {
	enum nfsstat    status;		/* Result status */
	char		*buf;		/* Buffer containing dir. entries */
	int		count;		/* Size of resulting buffer */
	int		offset;		/* Offset into directory */
	int		eof;		/* End-of-file has been reached? */
} *Tfs_get_wo_res, Tfs_get_wo_res_rec;


/*
 * Results of TFS_GETNAME
 */
typedef struct Tfs_getname_res {
	enum nfsstat	status;
	char		*path;
} *Tfs_getname_res, Tfs_getname_res_rec;


/*
 * Arguments for TFS_SET_SEARCHLINK
 */
typedef struct Tfs_searchlink_args {
	Tfs_fhandle_rec	fhandle;
	char		*value;
	bool_t		conditional;	/* Set the searchlink conditionally?
					 * (only when one not already there)
					 */
} *Tfs_searchlink_args, Tfs_searchlink_args_rec;


bool_t		xdr_tfs_mount_args();
bool_t		xdr_tfs_unmount_args();
bool_t		xdr_tfs_old_mount_args();
bool_t		xdr_tfs_mount_res();
bool_t		xdr_tfs_fhandle();
bool_t		xdr_tfs_name_args();
bool_t		xdr_tfs_get_wo_args();
bool_t		xdr_tfs_get_wo_res();
bool_t		xdr_tfs_getname_res();
bool_t		xdr_tfs_searchlink_args();

/*
 * Procedure #s for version #2 TFS calls.  These procedure numbers are
 * numbered starting where the TFS kernel protocol leaves off (in tfs.h)
 */
#define TFS_MOUNT	13
#define TFS_UNMOUNT	14
#define TFS_FLUSH	15
#define TFS_SYNC	16
#define TFS_UNWHITEOUT	17
#define TFS_GET_WO	18
#define TFS_GETNAME	19
#define TFS_PUSH	20
#define TFS_SET_SEARCHLINK 21
#define TFS_CLEAR_FRONT	22
#define TFS_WO_READONLY_FILES 23
#define TFS_PULL	24
#define	TFS_USER_NPROC	25


/*
 * Procedure #s for version #1 TFS calls.  The TFS procedure numbers were
 * numbered starting where NFS procedure numbers left off.
 */
#define TFS_OLD_USER_MINPROC 18
#define TFS_OLD_MOUNT	18
#define TFS_OLD_UNMOUNT	19
/*
 * TFS_FLUSH = 20 .... TFS_WO_READONLY_FILES = 28
 */
#define	TFS_OLD_USER_MAXPROC 28

#define TFS_OLD_VERSION	((u_long)1)

#endif _NSE_TFS_USER_H
