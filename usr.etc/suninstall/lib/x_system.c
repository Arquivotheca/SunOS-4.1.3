#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)x_system.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)x_system.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		x_system()
 *
 *	Description:	Use system() to perform 'cmd' and check for errors.
 */

#include "install.h"
#include "menu.h"


void
x_system(cmd)
	char *		cmd;
{
#ifdef TEST_JIG
	menu_log("%s", cmd);
#else
	if (system(cmd) != 0) {
		menu_log("%s: '%s' failed.", progname, cmd);
		menu_abort(1);
	}
#endif
} /* end x_system() */
