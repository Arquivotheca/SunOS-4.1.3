/*      @(#)makedevice.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "stdio.h"
#include "sys/param.h"

extern char *sprintf(), *getwd();

makedevice(path,dev,unit,progname)
char *path, *dev, *progname;
int unit;
{
	char cmd[BUFSIZ];
	char cwd[MAXPATHLEN];

	(void) getwd(cwd);
	(void) sprintf(cmd,"%s/dev",path);
	if ( chdir(cmd) < 0 ) {
                (void) fprintf(stderr,"%s:\tUnable to cd to %s\n",
		    progname, cmd);
		return(-1);
        } else {
		if (!strcmp(dev,"xt") || !strcmp(dev,"mt"))
                	(void) system("rm -f /dev/*mt*\n");
        	(void) sprintf(cmd,"/dev/MAKEDEV %s%d 2>/dev/null\n",dev,unit);
        	(void) system(cmd);
		(void) chdir(cwd);
	}
	return(0);
}
