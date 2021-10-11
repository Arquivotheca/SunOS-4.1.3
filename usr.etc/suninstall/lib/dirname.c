#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)dirname.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)dirname.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		dirname()
 *
 *	Description:	Return the parent directory of 'path'.  Returns "."
 *		if 'path' has no parent.
 *
 *	Note:		This routine returns a pointer to static data which
 *		is overlayed with each call.
 */

#include <sys/param.h>
#include <string.h>


char *
dirname(path)
	char *		path;
{
	char *		cp;			/* scratch char pointer */
						/* path to parent */
	static	char		parent[MAXPATHLEN];


	bzero(parent, sizeof(parent));

	cp = strrchr(path, '/');
	if (cp == NULL)
		parent[0] = '.';
	else
		(void) strncpy(parent, path, cp - path);

	return(parent);
} /* end dirname() */
