#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)syslog.c 1.1 92/07/30 SMI"; /* from UCB 5.9 5/7/86 */
#endif
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by /etc/syslogd.
 *
 * Author: Eric Allman
 * Modified to use UNIX domain IPC by Ralph Campbell
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <netdb.h>
#include <strings.h>
#include <varargs.h>
#include <vfork.h>

#define	MAXLINE	1024			/* max message size */
#define	NULL	0			/* manifest */

#define	PRIMASK(p)	(1 << ((p) & LOG_PRIMASK))
#define	PRIFAC(p)	(((p) & LOG_FACMASK) >> 3)
#define	IMPORTANT 	LOG_ERR

static char	*logname = "/dev/log";
static char	*ctty = "/dev/console";

static struct _syslog {
	int	_LogFile;
	int	_LogStat;
	char	*_LogTag;
	int	_LogMask;
	struct 	sockaddr _SyslogAddr;
	char	*_SyslogHost;
	int	_LogFacility;
} *_syslog;
#define	LogFile (_syslog->_LogFile)
#define	LogStat (_syslog->_LogStat)
#define	LogTag (_syslog->_LogTag)
#define	LogMask (_syslog->_LogMask)
#define	SyslogAddr (_syslog->_SyslogAddr)
#define	SyslogHost (_syslog->_SyslogHost)
#define	LogFacility (_syslog->_LogFacility)

extern	int errno, sys_nerr;
extern	char *sys_errlist[];

extern char *calloc();
extern time_t time();
extern unsigned int alarm();

static int
allocstatic()
{
	_syslog = (struct _syslog *)calloc(1, sizeof (struct _syslog));
	if (_syslog == 0)
		return (0);	/* can't do it */
	LogFile = -1;		/* fd for log */
	LogStat	= 0;		/* status bits, set by openlog() */
	LogTag = "syslog";	/* string to tag the entry with */
	LogMask = 0xff;		/* mask of priorities to be logged */
	LogFacility = LOG_USER;	/* default facility code */
	return (1);
}

/*VARARGS2*/
syslog(pri, fmt, va_alist)
	int pri;
	char *fmt;
	va_dcl
{
	va_list ap;

	va_start(ap);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

vsyslog(pri, fmt, ap)
	int pri;
	char *fmt;
	va_list ap;
{
	char buf[MAXLINE + 1], outline[MAXLINE + 1];
	register char *b, *f, *o;
	register int c;
	long now;
	int pid, olderrno = errno;

	if (_syslog == 0 && !allocstatic())
		return;
	/* see if we should just throw out this message */
	if (pri <= 0 || PRIFAC(pri) >= LOG_NFACILITIES ||
	    (PRIMASK(pri) & LogMask) == 0)
		return;
	if (LogFile < 0)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* build the message */
	o = outline;
	(void)sprintf(o, "<%d>", pri);
	o += strlen(o);
	(void)time(&now);
	(void)sprintf(o, "%.15s ", ctime(&now) + 4);
	o += strlen(o);
	if (LogTag) {
		(void)strcpy(o, LogTag);
		o += strlen(o);
	}
	if (LogStat & LOG_PID) {
		(void)sprintf(o, "[%d]", getpid());
		o += strlen(o);
	}
	if (LogTag) {
		(void)strcpy(o, ": ");
		o += 2;
	}

	b = buf;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && b < &buf[MAXLINE]) {
		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}
		if ((unsigned)olderrno > sys_nerr)
			(void)sprintf(b, "error %d", olderrno);
		else
			(void)strcpy(b, sys_errlist[olderrno]);
		b += strlen(b);
	}
	*b++ = '\n';
	*b = '\0';
	(void)vsprintf(o, buf, ap);
	c = strlen(outline);
	if (c > MAXLINE)
		c = MAXLINE;

	/* output the message to the local logger */
	if (sendto(LogFile, outline, c, 0, &SyslogAddr, sizeof SyslogAddr) >= 0)
		return;
	if (!(LogStat & LOG_CONS))
		return;

	/* output the message to the console */
	pid = vfork();
	if (pid == -1)
		return;
	if (pid == 0) {
		int fd;

		(void)signal(SIGALRM, SIG_DFL);
		(void)sigsetmask(sigblock(0) & ~sigmask(SIGALRM));
		(void)alarm(5);
		fd = open(ctty, O_WRONLY);
		(void)alarm(0);
		(void)strcat(o, "\r");
		o = index(outline, '>') + 1;
		(void)write(fd, o, c + 1 - (o - outline));
		(void)close(fd);
		_exit(0);
	}
	if (!(LogStat & LOG_NOWAIT))
		while ((c = wait((int *)0)) > 0 && c != pid)
			;
}

/*
 * OPENLOG -- open system log
 */

openlog(ident, logstat, logfac)
	char *ident;
	int logstat, logfac;
{
	if (_syslog == 0 && !allocstatic())
		return;
	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0)
		LogFacility = logfac & LOG_FACMASK;
	if (LogFile >= 0)
		return;
	SyslogAddr.sa_family = AF_UNIX;
	(void)strncpy(SyslogAddr.sa_data, logname, sizeof SyslogAddr.sa_data);
	if (LogStat & LOG_NDELAY) {
		LogFile = socket(AF_UNIX, SOCK_DGRAM, 0);
		(void)fcntl(LogFile, F_SETFD, 1);
	}
}

/*
 * CLOSELOG -- close the system log
 */

closelog()
{

	if (_syslog == 0)
		return;
	(void) close(LogFile);
	LogFile = -1;
}

/*
 * SETLOGMASK -- set the log mask level
 */
setlogmask(pmask)
	int pmask;
{
	int omask;

	if (_syslog == 0 && !allocstatic())
		return (-1);
	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
