#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)log.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)log.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		log.c
 *
 *	Description:	This file contains routine to implement message
 *		logging.
 */

#include <errno.h>
#include <stdio.h>
#include <varargs.h>
#include "install.h"




/*
 *	Name:		_log()
 *
 *	Description:	Print a message to stdout and the LOGFILE.  This
 *		routine is included to provide a varargs interface.
 */

void
_log(fmt, ap)
        char *		fmt;
	va_list		ap;
{
	FILE *		fp;			/* ptr to LOGFILE */
	

        (void) vfprintf(stdout, fmt, ap);

	fp = fopen(LOGFILE, "a");
	if (fp == NULL)
                (void) fprintf(stderr,"%s: %s: %s", progname, LOGFILE,
			       err_mesg(errno));
	else {
		(void) vfprintf(fp, fmt, ap);
		(void) fclose(fp);
	}
} /* end _log() */




/*
 *	Name:		log()
 *
 *	Description:	Print a message to stdout and the LOGFILE.  This
 *		routine calls _log() to do the work.
 */

/*VARARGS1*/
void
log(fmt, va_alist)
	char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */


	va_start(ap);				/* init varargs stuff */

	_log(fmt, ap);
} /* end log() */
