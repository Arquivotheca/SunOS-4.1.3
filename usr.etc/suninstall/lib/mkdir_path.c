#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)mkdir_path.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)mkdir_path.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		mkdir_path.c
 *
 *	Description:	Make a directory path.  Including any missing path
 *		elements.  Equivalent to the "mkdir -p" command.  This routine
 *		should not use dirname() since it might be called with the
 *		return value of dirname() call.
 *
 *		If this is a SunOS MLS system, then we attempt to determine
 *		at which label value to create the directory.  This check is
 *		needed in addition to the routine mkldir_path() since a call
 *		to mkdir_path() can create missing directories that may have
 *		to be at other labels.  e.g. /etc/security/audit/wilma may be
 *		the argument, and /etc/security/audit could be missing and must
 *		be at system_high.
 *
 *	Note:	If name points to read-only memory, then this routine will fail.
 */

#ifdef SunB1
#include <sys/types.h>
#include <sys/label.h>
#endif /* SunB1 */
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
mkdir_path(name)
	char *		name;
{
#ifdef SunB1
	blabel_t	label_buf;		/* label buffer */
	char *		real_name;		/* pointer to real name */
#endif /* SunB1 */
	char *		slash_p;		/* ptr to slash */


#ifdef SunB1
	/*
	 *	Determine the real pathname for the label check
	 */
	real_name = name;
	if (is_miniroot())
		real_name += strlen(dir_prefix());

	/*
	 *	Figure out what the label should be.
	 */
	if (at_sys_high(real_name))
		blhigh(BL_ALLPARTS, &label_buf);
	else
		bllow(BL_ALLPARTS, &label_buf);
#endif /* SunB1 */

	/*
	 *	First, just try to make the directory
	 */
#ifdef SunB1
	if (mkldir(name, MODE, &label_buf) == 0) {
		(void) chmod(name, MODE);
#else
	if (mkdir(name, MODE) == 0) {
		(void) chmod(name, MODE);
#endif /* SunB1 */
		return;
	}

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
	mkdir_path(name);
	*slash_p = '/';


	/*
	 *	Back from recursion so this should work
	 */
#ifdef SunB1
	if (mkldir(name, MODE, &label_buf) != 0) {
		(void) chmod(name, MODE);
#else
	if (mkdir(name, MODE) != 0)  {
		(void) chmod(name, MODE);
#endif /* SunB1 */
		fatal(name, errno);
	}
} /* end mkdir_path() */




static void
fatal(name, code)
	char *		name;
	int		code;
{
	menu_log("%s: %s: %s.", progname, name, err_mesg(code));
	menu_abort(1);
} /* end fatal() */
