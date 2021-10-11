#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)x_chdir.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)x_chdir.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		x_chdir()
 *
 *	Description:	Use chdir() to change directories and check
 *		for errors.
 */

#include <errno.h>
#include "install.h"
#include "menu.h"


void
x_chdir(dir)
	char *		dir;
{
	if (chdir(dir) != 0) {
		menu_log("%s: %s: %s.", progname, dir, err_mesg(errno));
		menu_log("\tchdir(2) failed.");
		menu_abort(1);
	}
} /* end x_chdir() */
