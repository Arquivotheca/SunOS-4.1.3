#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mklink.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mklink.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mklink.c
 *
 *	Description:	Make 'link_path' a symbolic link to 'path'.  Use
 *		mkdir_path to make any new directories needed in 'link_path'.
 *		This routine should not use dirname() since it might be
 *		called with the return value of dirname() call.
 */

#include <errno.h>
#include "install.h"
#include "menu.h"


/*
 *	External functions:
 */
extern	char *		strncpy();
extern	char *		strrchr();




void
mklink(path, link_path)
	char *		path;
	char *		link_path;
{
	char *		last_p;			/* ptr to last path part */
	char		parent[MAXPATHLEN];	/* parent's pathname */


	/*
	 *	Assuming that 'path' exists, a link to itself is already done.
	 */
	if (strcmp(path, link_path) == 0)
		return;

	/*
	 *	Find last part of the pathname.
	 */
	last_p = strrchr(link_path, '/');
	if (last_p == NULL)
		last_p = link_path;

	(void) strncpy(parent, link_path, last_p - link_path);
	parent[last_p - link_path] = '\0';	/* force NULL termination */

	mkdir_path(parent);

	if (symlink(path, link_path) != 0 && errno != EEXIST) {
		menu_log("%s: %s, %s: %s.", progname, path, link_path,
			 err_mesg(errno));
		menu_abort(1);
	}
} /* end mklink() */
