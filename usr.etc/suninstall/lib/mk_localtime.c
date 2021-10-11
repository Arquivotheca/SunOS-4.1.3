#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mk_localtime.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mk_localtime.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mk_localtime.c
 *
 *	Description:	Make the localtime link in ZONEINFO_PATH.  
 *			If prefix is non-NULL, then add it to the path.
 *			If sharepath is non-NULL, then add it to the path;
 *			otherwise use "/usr/share".
 */

#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		sprintf();




void
mk_localtime(sys_p, prefix, sharepath)
	sys_info *	sys_p;
	char *		prefix;
	char *		sharepath;
{
	char		local_path[MAXPATHLEN];	/* path to localtime file */
	char		tz_path[MAXPATHLEN];	/* path to timezone file */


	if (prefix == NULL)
		prefix = "";

	(void) sprintf(local_path, "%s%s%s/localtime", prefix, 
			sharepath == NULL ? "/usr/share" : sharepath,
			ZONEINFO_PATH);

	(void) unlink(local_path);

	(void) sprintf(tz_path, "%s%s%s/%s", prefix, 
			sharepath == NULL ? "/usr/share" : sharepath,
			ZONEINFO_PATH, sys_p->timezone);

	if (link(tz_path, local_path) != 0) {
#ifndef TEST_JIG
		menu_log("%s: %s: cannot link to localtime.", progname,
			 tz_path);
		menu_abort(1);
#endif TEST_JIG
	}
} /* end mk_localtime() */
