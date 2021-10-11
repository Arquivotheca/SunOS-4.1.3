/*      @(#)makedirpath.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DIR_PERMS  02755

/*
 *	make all the directories in the path
 */

extern char	*rindex();

makedirpath(pathname)
char *pathname;
{
	int err;
	char *slash;

	if (mkdir(pathname, DIR_PERMS) == 0) {
		/* since the setgid bit gets ignored by the mkdir command,
		** let's forcibly set it. If some error occurs, lets let
		** mkdir catch it.
		*/
		chmod(pathname, DIR_PERMS);
		return (0);
	}
	if (errno != ENOENT)
		return (-1);
	slash = rindex(pathname, '/');
	if (slash == NULL)
		return (-1);
	*slash = '\0';
	err = makedirpath(pathname);
	*slash = '/';
	if (err)
		return (err);
	return mkdir(pathname, DIR_PERMS);
}
