#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mkldir_path.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mkldir_path.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mkldir_path.c
 *
 *	Description:	Make a labeled directory path.  Including any missing
 *		path elements.  Equivalent to the "mkdir -Z label -p" command.
 *		This routine should not use dirname() since it might be called
 *		with the return value of dirname() call.
 *
 *	Note:	If name points to read-only memory, then this routine will fail.
 */

#include <sys/types.h>
#include <sys/label.h>
#include <errno.h>
#include <string.h>
#include "install.h"
#include "menu.h"


/*
 *	Local constants:
 */
#define	MODE		02755




/*
 *	Local routines:
 */
static	void		fatal();


void
mkldir_path(name, label_p)
	char *		name;
	blabel_t *	label_p;
{
	char *		slash_p;		/* ptr to slash */


	/*
	 *	First, just try to make the directory
	 */
	if (mkldir(name, MODE, label_p) == 0)
		return;

	/*
	 *	Directory already exists.
	 */
	if (errno == EEXIST)
		return;

	/*
	 *	Some failure other than "no such entry"
	 */
	if (errno != ENOENT)
		fatal(name, errno);

	/*
	 *	Strip off the last path element and try again recusively.
	 */
	slash_p = strrchr(name, '/');
	if (slash_p == 0)
		fatal(name, errno);

	*slash_p = '\0';
	mkldir_path(name, label_p);
	*slash_p = '/';


	/*
	 *	Back from recursion so this should work
	 */
	if (mkldir(name, MODE, label_p) != 0)
		fatal(name, errno);
} /* end mkldir_path() */




static void
fatal(name, code)
	char *		name;
	int		code;
{
	menu_log("%s: %s: %s.", progname, name, err_mesg(code));
	menu_abort(1);
} /* end fatal() */
