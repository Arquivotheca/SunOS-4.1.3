#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)basename.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)basename.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		basename()
 *
 *	Description:	Return the last component of 'path'.
 */

#include <string.h>


#define NULL 0


char *
basename(path)
	char *		path;
{
	char *		cp;			/* scratch char pointer */


	cp = strrchr(path, '/');

	return((cp == NULL) ? path : cp + 1);
} /* end basename() */
