/*	@(#)lslog.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lslog.c	1.7.1.1"

/*
 * error/logging/cleanup functions for the network listener process.
 */


/* system include files	*/

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <varargs.h>
#include <string.h>
#include <errno.h>
#include <sys/utsname.h>
#include <tiuser.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <values.h>
#include <ctype.h>

#ifdef	S4
#include <status.h>
#endif

/* listener include files */

#include "lsparam.h"		/* listener parameters		*/
#include "lsfiles.h"		/* listener files info		*/
#include "lserror.h"		/* listener error codes		*/

extern char Lastmsg[];
extern int Child;
extern char *Netspec;
extern FILE *Logfp;
extern FILE *Debugfp;

/*
 * error handling and debug routines
 * most routines take two args: code and exit.
 * code is a #define in lserror.h.
 * if EXIT bit in exitflag is non-zero, the routine exits. (see cleanup() )
 * define COREDUMP to do the obvious.
 */


/*
 * error: catastrophic error handler
 */

error(code, exitflag)
int code, exitflag;
{
	char scratch[BUFSIZ];

	if (!(exitflag & NO_MSG)) {
		strcpy(scratch, err_list[code].err_msg);
		cleanup(code, exitflag, scratch);
	}
	cleanup(code, exitflag, NULL);
}

/*
 * tli_error:  Deal (appropriately) with an error in a TLI call
 */

static char *tlirange = "Unknown TLI error (t_errno > t_nerr)";

tli_error(code, exitflag)
int code, exitflag;
{
	void	t_error();
	extern	int	t_errno;
	extern	char	*t_errlist[];
	extern	int	t_nerr;
	extern char *sys_errlist[];
	extern int sys_nerr;
	extern char *range_err();
	char	scratch[256];
	register char *p;

	p = ( t_errno < t_nerr ? t_errlist[t_errno] : tlirange );

	sprintf(scratch, "%s: %s", err_list[code].err_msg, p);
	if (t_errno == TSYSERR)  {
		p = (errno < sys_nerr ? sys_errlist[errno] : range_err());
		strcat(scratch, ": ");
		strcat(scratch, p);
	}
	cleanup(code, exitflag, scratch);
}


/*
 * sys_error: error in a system call
 */

sys_error(code, exitflag)
int code, exitflag;
{
	extern int errno;
	extern char *sys_errlist[];
	extern int sys_nerr;
	register char *p;
	char scratch[256];
	extern char *range_err();

	p = (errno < sys_nerr ? sys_errlist[errno] : range_err());

	sprintf(scratch, "%s: %s", err_list[code].err_msg, p);
	cleanup(code, exitflag, scratch);
}


/*
 * sysf_error: error in a system call with a file name (or other string)
 */

sysf_error(code, s, exitflag)
int code, exitflag;
char *s;
{
	char *p;
	char scratch[256];
	extern char *range_err();
	extern int errno;
	extern char *sys_errlist[];
	extern int sys_nerr;

	p = (errno < sys_nerr ? sys_errlist[errno] : range_err());

	sprintf(scratch, "File/Directory %s: %s: %s", 
		s, err_list[code].err_msg, p);
	cleanup(code, exitflag, scratch);
}


/*
 * cleanup:	if 'flag', and main listener is exiting, clean things
 *		up and exit.  Dumps core if !(flag & NOCORE).
 *		Tries to send a message to someone if the listener
 *		is exiting due to an error. (Inherrently machine dependent.)
 *		If (Validate), listener is just validating data base - 
 *		no clenaup is desired (real listener may be running also)
 */

#ifdef	S4
static char *eformat = 
"The STARLAN NETWORK listener process has terminated.\n\
All further Incoming Services are disabled.\n\
Consult your STARLAN NETWORK UNIX PC User's Guide.";
#endif

cleanup(code, flag, msg)
register code, flag;
char *msg;
{
	extern int Nfd1, Nfd2, Nfd3;
	extern void logexit();
	extern Background, Child, Nflag, Validate;

	if (!(flag & EXIT)) {
		logmessage(msg);
		return;
	}

	if (!(Child) && !(Validate))  {

		unlink(PIDNAME);		/* try removing pid file */

		/*
		 * unbind anything that we bound.
		 * Needs more intelligence.
		 */

		t_unbind(Nfd1);
		t_unbind(Nfd2);
		t_unbind(Nfd3);

		if (!Background)
			fprintf(stderr,Lastmsg);

#ifdef	S4
		else  if (!(flag & NORMAL)) {
			eprintf(ST_OTHER, ST_DISPLAY, (char *)0, eformat);
		}
#endif	/* S4 */

	}

#ifdef	COREDUMP
	if (!(flag & NOCORE))
		abort();
#endif	/* COREDUMP */

	logexit(err_list[code].err_code, msg);
}



/*
 * range_err:	returns a string to use when errno > sys_nerr
 */

static char *sysrange = "Unknown system error (errno %d > sys_nerr)";
static char range_buf[128];

char *
range_err()
{
	extern int errno;

	sprintf(range_buf,sysrange,errno);
	return(range_buf);
}


void
logexit(exitcode, msg)
int exitcode;
char *msg;
{
	FILE *fp;
	long clock;
	extern long time();
	extern char *ctime();

/*
 * note: fp == NULL if we are the child, hence further checks are unnecessary
 */

	fp = (FILE *) NULL;
	if (!Child) {
		fp = fopen("/dev/console", "w");
		if (fp == NULL) {
			logmessage("couldn't open /dev/console");
		}
	}
	if (msg) {
		logmessage(msg); /* put it in the log */
		if (fp) {
			fprintf(fp, "LISTEN: ");
			fprintf(fp, msg); /* and on the console */
			fprintf(fp, "\n");
		}
	}
	if (!Child)
		logmessage("*** listener terminating!!! ***");
	if (fp) {
		(void) time(&clock);
		fprintf(fp, "LISTEN: *** listener on netspec <%s> terminating at %s", Netspec, ctime(&clock));
	}
	if (fp)
		fclose(fp);
	exit(exitcode);
}


#ifdef	DEBUGMODE

/*	from vax sys V (5.0.5): @(#)sprintf.c	1.5
 *	works with doprnt.c from 5.0.5 included in listener a.out
 *	This does have dependencies on FILE * stuff, so it may change
 *	in future releases.
 */


/*VARARGS2*/
int
debug(level, format, va_alist)
int level;
char *format;
va_dcl
{
	char buf[256];
	FILE siop;
	va_list ap;
	extern int _doprnt();
	extern char *stamp();

	siop._cnt = MAXINT;
	siop._base = siop._ptr = (unsigned char *)buf;
	siop._flag = _IOWRT;
	siop._file = _NFILE;
	va_start(ap);
	(void) _doprnt(format, ap, &siop);
	va_end(ap);
	*siop._ptr = '\0'; /* plant terminating null character */
	fprintf(Debugfp, stamp(buf));
	fflush(Debugfp);
}

#endif



/*
 * log:		given a message number (code), write a message to the logfile
 * logmessage:	given a string, write a message to the logfile
 */

static char *logprot = "%s; %d; %s\n";
static char *valprot = "%s\n";

log(code)
int code;
{
	logmessage(err_list[code].err_msg);
}


static int nlogs;		/* maintains size of logfile	*/

logmessage(s)
char *s;
{
	register err = 0;
	register FILE *nlogfp;
	extern char *stamp();
	extern int Logmax;
	extern int Splflag;

	/*
	 * The listener may be maintaining the size of it's logfile.
	 * Nothing in here should make the listener abort.
	 * If it can't save the file, it rewinds the existing log.
	 * Note that the algorithm is not exact, child listener's
	 * messages do not affect the parent's count.
	 */

	if (!Logfp)
		return;
	if (!Child && Logmax && ( nlogs >= Logmax ) && !Splflag)  {
		nlogs = 0;
		DEBUG((1, "Logfile exceeds Logmax (%d) lines", Logmax));
		unlink(OLOGNAME); /* remove stale saved logfile */
		if (link(LOGNAME, OLOGNAME))  {
			++err;
			DEBUG((1,"errno %d linking log to old logfile",errno));
		}
		if (unlink(LOGNAME))  {
			++err;
			rewind(Logfp);
			DEBUG((1, "Cannot unlink LOGFILE, errno %d", errno));
		}
		else  if (nlogfp = fopen(LOGNAME, "w"))  { 
			fclose(Logfp);
			Logfp = nlogfp;
			fcntl(fileno(Logfp), F_SETFD, 1); /* reset close-on-exec */
			DEBUG((1, "logmessage: logfile saved successfully"));
		}  else  {
			++err;
			rewind(Logfp);
			DEBUG((1, "Lost the logfile, errno %d", errno));
		}
		if (err)
			fprintf(Logfp, stamp("Trouble saving the logfile"));
	}

	fprintf(Logfp, stamp(s));
	fflush(Logfp);
	++nlogs;
}


char *
stamp(msg)
char *msg;
{
	long clock;
	long time();
	char *timestamp;
	extern char *ctime();
	extern Validate;	/* no timestamp if validating data base file */

	if (Validate)
		sprintf(Lastmsg, valprot, msg);
	else  {
		(void)time(&clock);
		timestamp = ctime(&clock);
		*(strchr(timestamp,'\n')) = (char)0;	/* del newline char */

		sprintf(Lastmsg, logprot, timestamp, getpid(), msg);
	}
	return(Lastmsg);
}
