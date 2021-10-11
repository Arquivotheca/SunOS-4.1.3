/*      @(#)message.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

/*VARARGS1*/
message(fmt, a1, a2, a3, a4, a5)
        char    *fmt;
        int     a1, a2, a3, a4, a5;
{
	FILE *log;

        (void) fprintf(stdout, fmt, a1, a2, a3, a4, a5);
	if( (log = fopen(LOGFILE,"a")) == NULL ) {
                (void) fprintf(stderr,"\nUnable to open %s.",LOGFILE);
                aborting(1);
        }
	(void) fprintf(log, fmt, a1, a2, a3, a4, a5);
	(void) fclose(log);
}

