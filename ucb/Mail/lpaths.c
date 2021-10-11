#ifndef lint
static	char *sccsid = "@(#)lpaths.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

/*
 *	libpath(file) - return the full path to the library file
 */

#include "uparm.h"

extern	char *strcpy(), *strcat();

char *
libpath(file)
char *file;		/* the file name */
{
	static char buf[100];	/* build name here */

	strcpy(buf, LIBPATH);
	strcat(buf, "/");
	strcat(buf, file);
	return(buf);
}
