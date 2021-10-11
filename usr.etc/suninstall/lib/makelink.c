/*      @(#)makelink.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "stdio.h"
#include "errno.h"
#include "sys/param.h"

/*
 *	symlink link_path to path, make directories above path if needed
 */


makelink(path, link_path, progname)
char *path, *link_path, *progname;
{
	char parent_path[MAXPATHLEN], *name;
	char cwd[MAXPATHLEN];
	extern int errno;
	extern char *rindex(), *strncpy(), *getwd();

	/* if requesting a link to itself - return */
	if (strcmp(link_path, path) == 0) return(0);
	if ((name = rindex(path, '/')) == NULL)
		name = path;
	else
		name++;
	(void) strncpy(parent_path, path, name - path);
	parent_path[name-path] = '\0';	/* null terminate result */
	(void) makedirpath(parent_path);
	(void) getwd(cwd);
	if (chdir(parent_path) == 0) {
		if (symlink(link_path, name) != 0) {
			if (errno == ENOTDIR || errno == ENOENT)
				(void) log("%s:\tcouldn't link %s to %s \n", 
				    progname, link_path, name);
			else if (errno != EEXIST)
				(void) log("%s:\tcouldn't create link of %s to %s - errno %d\n", 
				    progname, path, link_path, errno);
		}
		(void) chdir(cwd);
	}
	return(0);
}
