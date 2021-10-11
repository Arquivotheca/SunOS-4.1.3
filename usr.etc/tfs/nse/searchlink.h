/*	@(#)searchlink.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */
#ifndef _NSE_SEARCHLINK_H
#define _NSE_SEARCHLINK_H

#ifndef _NSE_TYPES_H
#include <nse/types.h>
#endif

#ifndef _NSE_ERR_H
#include <nse/err.h>
#endif

#define NSE_TFS_VERSION	2
#define NSE_TFS_ORIG_VERSION	1
#define NSE_TFS_HAS_BACK(n) (n > 1)
#define NSE_TFS_VERSION_HDR "VERSION"

#define NSE_TFS_FILE_PREFIX	".tfs_"
#define NSE_TFS_FILE_PREFIX_LEN	5

#define NSE_TFS_FILE	".tfs_info"
#define NSE_TFS_BACK_FILE ".tfs_backfiles"
#define NSE_TFS_FORWARD_FILE ".tfs_forward"
#define NSE_TFS_UTIMES_FILE ".tfs_utimes"

/*
 * The following string is written into a backlink to indicate that a
 * directory rename is in progress, to allow completion of the rename
 * later if the tfsd should die in the middle of it.
 */
#define NSE_TFS_RENAME_STR "!TFS_RENAME"

#define NSE_TFS_WILDCARD "$view"

/*
 * Error code that nse_set_cond_searchlink() returns when a searchlink
 * already exists for the directory.
 */
#define NSE_SL_ALREADY_SET_DIFF		-701
#define NSE_SL_ALREADY_SET_SAME		-702

#ifdef NSE_ERROR_COLLECT
static char *(searchlink_errors[]) = {
	"searchlink",
	"-701 -799",
	NSE_BEGIN_ERROR_TAB,
	"Internal error:  Searchlink already set to different value in directory %s",
	"Internal error:  Searchlink already set to same value in directory %s",
	NSE_END_ERROR_TAB
	};
#undef NSE_BEGIN_ERROR_TAB
#define NSE_BEGIN_ERROR_TAB (char *) searchlink_errors
#endif

struct Nse_whiteout {
	char		*name;
	struct Nse_whiteout *next;
};

typedef struct Nse_whiteout *Nse_whiteout;
typedef struct Nse_whiteout Nse_whiteout_rec;


Nse_err		nse_get_searchlink();
Nse_err		nse_set_searchlink();
Nse_err		nse_set_cond_searchlink();
Nse_err		nse_get_whiteout();
Nse_err		nse_set_whiteout();
Nse_err		nse_clear_whiteout();
Nse_err		nse_purge_whiteout();
void		nse_dispose_whiteout();
Nse_err		nse_get_backlink();
Nse_err		nse_set_backlink();
Nse_err		nse_clear_backlink();
Nse_err		nse_get_tfs_info();
Nse_err		nse_set_tfs_info();
Nse_err		nse_shift_tfs_info();
Nse_err		nse_copy_tfs_info();
Nse_err		nse_move_tfs_info();
Nse_err		nse_clear_tfs_info();

#endif _NSE_SEARCHLINK_H
