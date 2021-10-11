/*      @(#)log_start.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern char *sprintf();

log_start(program)
char *program;
{
	FILE *logfd;
	char cmd[256];

	if (access("/usr/etc/install/files",F_OK)) {
		(void) sprintf(cmd,
			"mkdir -p /usr/etc/install/files 2>/dev/null");
		(void) system(cmd);
	}

	if ((logfd = fopen(LOGFILE,"a")) != NULL) {
		(void) fprintf(logfd,"%s: started ",program);
		(void) fclose(logfd);
		(void) sprintf(cmd,"/bin/date >> %s",LOGFILE);
		(void) system(cmd);
	} else
		(void) fprintf(stderr,"%s: couldnt open %s\n",
			program,LOGFILE);
}

