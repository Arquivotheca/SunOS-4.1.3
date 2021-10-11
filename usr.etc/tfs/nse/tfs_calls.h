/*	@(#)tfs_calls.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#ifndef _NSE_TFS_CALLS_H
#define _NSE_TFS_CALLS_H

#ifndef _NSE_ERR_H
#include <nse/err.h>
#endif

/*
 * TFS error codes.
 */

#define NSE_TFS_NO_DIRECTORY    -901    /* Directory to mount doesn't exist */
#define NSE_TFS_NO_MOUNTPT      -902    /* Dir. to mount on doesn't exist */
#define	NSE_TFS_NOT_UNDER_MOUNTPT -903	/* File specified not under a mount
					 * pt. */
#define NSE_TFS_NOT_IN_FRONT_FS	-904	/* File not in front file system */
#define NSE_TFS_TOO_MANY_ENVS	-905	/* Too many environments activated */
#define NSE_TFS_NOT_ACTIVATED	-906	/* Not activated */


#ifdef NSE_ERROR_COLLECT
static char *(tfs_errors[]) = {
        "tfs",
        "-901 -999",
        NSE_BEGIN_ERROR_TAB,
        "Internal error: no directory",
        "Internal error: no mount point",
	"Internal error: File %s is not under a control point",
	"Internal error: File %s is not in front file system",
	"Too many activated environments",
	"Not activated",
	NSE_END_ERROR_TAB
};
#undef NSE_BEGIN_ERROR_TAB
#define NSE_BEGIN_ERROR_TAB (char *) tfs_errors
#endif

/* tfs_client.c */
Nse_err		nse_tfs_flush();
Nse_err		nse_unwhiteout();
struct direct	*nse_readdir_whiteout();
void		nse_tfs_init();
Nse_err		nse_tfs_getname();
Nse_err		nse_tfs_set_cond_searchlink();
Nse_err		nse_set_searchlink();
Nse_err		nse_tfs_clear_front_fs();
Nse_err		nse_tfs_wo_readonly_files();
Nse_err		nse_tfs_set_searchlink();
Nse_err		nse_set_searchlink();
Nse_err		nse_tfs_mount_branch_dir();
Nse_err		nse_tfs_umount_branch_dir();
Nse_err		nse_tfs_rpc_call();
Nse_err		nse_tfs_rpc_call_to_host();
Nse_err		nse_tfs_rpc_call_to_host_1();
char		*nse_tfs_get_br_mount();
Nse_err		nse_tfs_push();
Nse_err		nse_tfs_pull();
Nse_err		nse_tfs_sync();
Nse_err		nse_tfs_mount();
Nse_err		nse_tfs_umount();


#endif _NSE_TFS_CALLS_H
