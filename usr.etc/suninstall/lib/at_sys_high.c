#ifndef lint
static	char		mls_sccsid[] = "@(#)at_sys_high.c 1.1 92/07/30 SMI; SunOS MLS";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		at_sys_high()
 *
 *	Description:	Determine if a file system object should be at
 *		system_high.  Returns 1 if the object should be at
 *		system_high and 0 otherwise.
 */

#include "install.h"
#include "menu.h"




/*
 *	The list of directories that should be at system_high
 */
static	char *		sys_high_list[] = {
	AUDIT_DIR,

	CP_NULL
};


int
at_sys_high(name)
	char *		name;
{
	char **		cpp;			/* scratch ptr to char ptr */


	for (cpp = sys_high_list; *cpp; cpp++) {
		if (strcmp(name, *cpp) == 0)	/* exact match */
			return(1);

		/*
		 *	Directory is a prefix of the name.  Thus name must
		 *	be at system_high to keep with monotonically
		 *	non-decreasing.
		 */
		if (strncmp(name, *cpp, strlen(*cpp)) == 0 &&
		    name[strlen(*cpp)] == '/')
			return(1);
	}

	return(0);
} /* end at_sys_high() */
