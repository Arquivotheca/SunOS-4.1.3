/*LINTLIBRARY*/
/*      @(#)report.h 1.1 92/07/30 SMI      */

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>

extern FILE	*get_report_file();
extern char	*get_target_being_reported_for();
extern void	report_dependency();
extern char	*file_lock();

#define SUNPRO_DEPENDENCIES "SUNPRO_DEPENDENCIES"
#define LD 	"LD"
#define COMP 	"COMP"

/* the following definitions define the interface between make and
 * NSE - the two systems must track each other.
 */
#define NSE_DEPINFO 		".nse_depinfo"
#define NSE_DEPINFO_LOCK 	".nse_depinfo.lock"
#define NSE_DEP_ENV 		"NSE_DEP"
#define NSE_TFS_PUSH 		"/usr/nse/bin/tfs_push"
#define NSE_TFS_PUSH_LEN 	8
#define NSE_VARIANT_ENV 	"NSE_VARIANT"
#define NSE_RT_SOURCE_NAME 	"Shared_Source"
