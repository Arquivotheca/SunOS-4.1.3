/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)syslogd.c 1.1 92/07/30 SMI"; /* from UCB 5.18 2/23/87 */
#endif not lint

/*
 *  syslogd -- log system messages
 *
 * This program implements a system log. It takes a series of lines.
 * Each line may have a priority, signified as "<n>" as
 * the first characters of the line.  If this is
 * not present, a default priority is used.
 *
 * To kill syslogd, send a signal 15 (terminate).  A signal 1 (hup) will
 * cause it to reread its configuration file.
 *
 * Defined Constants:
 *
 * MAXLINE -- the maximimum line length that can be handled.
 * NLOGS   -- the maximum number of simultaneous log files.
 * DEFUPRI -- the default priority for user messages
 * DEFSPRI -- the default priority for kernel messages
 *
 * Author: Eric Allman
 * extensive changes by Ralph Campbell
 */

#define DEFUPRI 	(LOG_USER|LOG_NOTICE)
#define DEFSPRI 	(LOG_KERN|LOG_ERR)
#define PANICPRI 	(LOG_KERN|LOG_EMERG)
#define	NLOGS		20		/* max number of log files */
#define	MAXLINE		1024		/* maximum line length */
#define MARKCOUNT	/*10*/3		/* ratio of minor to major marks */

#include <errno.h>
#include <stdio.h>
#include <utmp.h>
#include <ctype.h>
#include <signal.h>
#include <sysexits.h>
#include <strings.h>
#include <values.h>

#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/msgbuf.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>

char	*LogName = "/dev/log";
char	*ConfFile = "/etc/syslog.conf";
char	*PidFile = "/etc/syslog.pid";
char	ctty[] = "/dev/console";
char	panic_str[] = "panic:";

#define FDMASK(fd)	(1 << (fd))

#define	dprintf		if (Debug) (void) printf

#define UNAMESZ		8	/* length of a login name */
#define MAXUNAMES	20	/* maximum number of user names */
#define MAXFNAME	200	/* max file pathname length */

#define NOPRI		0x10	/* the "no priority" priority */
#define	LOG_MARK	(LOG_NFACILITIES << 3)	/* mark "facility" */

/*
 * Flags to printline().
 */
#define ALL		0	/* all messages allowed */
#define USER_ONLY	1	/* force kernel messages to user */

/*
 * Flags to logmsg().
 */
#define IGN_CONS	0x001	/* don't print on console */
#define SYNC_FILE	0x002	/* do fsync on file after printing */
#define NOCOPY		0x004	/* don't suppress duplicate messages */
#define ADDDATE		0x008	/* add a date to the message */
#define MARK		0x010	/* this message is a mark */

/*
 * This structure represents the files that will have log
 * copies printed.
 */

struct filed {
	short	f_type;			/* entry type, see below */
	short	f_file;			/* file descriptor */
	time_t	f_time;			/* time this was last written */
	u_char	f_pmask[LOG_NFACILITIES+1];	/* priority mask */
	union {
		char	f_uname[MAXUNAMES][UNAMESZ+1];
		struct {
			char	f_hname[MAXHOSTNAMELEN+1];
			struct sockaddr_in	f_addr;
		} f_forw;		/* forwarding address */
		char	f_fname[MAXFNAME];
	} f_un;
};

/* values for f_type */
#define F_UNUSED	0		/* unused entry */
#define F_FILE		1		/* regular file */
#define F_TTY		2		/* terminal */
#define F_CONSOLE	3		/* console terminal */
#define F_FORW		4		/* remote machine */
#define F_USERS		5		/* list of users */
#define F_WALL		6		/* everyone logged on */

char	*TypeNames[7] = {
	"UNUSED",	"FILE",		"TTY",		"CONSOLE",
	"FORW",		"USERS",	"WALL"
};

struct filed	Files[NLOGS];

/* simple hash table cache for host names */
#define HASHSIZE 497

struct hashent {
	struct hashent *next;
	unsigned long addr;
	char name[MAXHOSTNAMELEN];
};

struct hashent *Table[HASHSIZE];

int	Debug;			/* debug flag */
char	LocalHostName[MAXHOSTNAMELEN+1];	/* our hostname */
char	*LocalDomain;		/* our local domain name */
int	InetInuse = 0;		/* non-zero if INET sockets are being used */
int	LogPort;		/* port number for INET connections */
char	PrevLine[MAXLINE + 1];	/* copy of last line to supress repeats */
char	PrevHost[MAXHOSTNAMELEN+1];		/* previous host */
int	PrevFlags;
int	PrevPri;
int	PrevCount = 0;		/* number of times seen */
int	FlushTimer;		/* timer for flushing messages */
int	Initialized = 0;	/* set when we have initialized ourselves */
int	MarkInterval = 20;	/* interval between marks in minutes */
int	Marking = 0;		/* non-zero if marking some file */
int	MarkTimer;		/* timer for marks */
int	defupri = DEFUPRI;	/* default user priority */
int	defspri = DEFSPRI;	/* default kernel priority */
void	init();

extern	void exit();
extern	int sys_nerr;
extern	char *sys_errlist[];
extern	char *ctime(), *index();
extern	char *malloc();
extern	time_t time();

/* Lint does not seem to understand that errno is external. */
#ifdef	lint
	int errno;
#else	lint
extern	int errno;
#endif	lint

main(argc, argv)
	int argc;
	char **argv;
{
	register int i;
	register char *p;
	int funix, finet, inetm, fklog, klogm, len;
	struct sockaddr_un sunx, fromunix;
	struct sockaddr_in sin, frominet;
	FILE *fp;
	char line[MSG_BSIZE + 1];
	void die(), doalarm(), reapchild();

	bzero((char *)&sin, sizeof (struct sockaddr_in));
	bzero((char *)&sunx, sizeof (struct sockaddr_un));
	while (--argc > 0) {
		p = *++argv;
		if (p[0] != '-')
			usage();
		switch (p[1]) {
		case 'f':		/* configuration file */
			if (p[2] != '\0')
				ConfFile = &p[2];
			break;

		case 'd':		/* debug */
			Debug++;
			break;

		case 'p':		/* path */
			if (p[2] != '\0')
				LogName = &p[2];
			break;

		case 'm':		/* mark interval */
			if (p[2] != '\0')
				MarkInterval = atoi(&p[2]);
			break;

		default:
			usage();
		}
	}

	if (!Debug) {
		if (fork())
			exit(0);
		for (i = 0; i < 10; i++)
			(void) close(i);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		untty();
	} else
		setlinebuf(stdout);

	(void) gethostname(LocalHostName, sizeof LocalHostName);
	if (p = index(LocalHostName, '.')) {
		*p++ = '\0';
		LocalDomain = p;
	}
	else
		LocalDomain = "";
	(void) signal(SIGTERM, die);
	(void) signal(SIGINT, Debug ? die : SIG_IGN);
	(void) signal(SIGQUIT, Debug ? die : SIG_IGN);
	(void) signal(SIGCHLD, reapchild);
	(void) signal(SIGALRM, doalarm);
	(void) unlink(LogName);

	sunx.sun_family = AF_UNIX;
	(void) strncpy(sunx.sun_path, LogName, sizeof sunx.sun_path);
	funix = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (funix < 0 || bind(funix, (struct sockaddr *) &sunx,
	    sizeof(sunx.sun_family)+strlen(sunx.sun_path)) < 0 ||
	    chmod(LogName, 0666) < 0) {
		(void) sprintf(line, "cannot create %s", LogName);
		logerror(line);
		dprintf("cannot create %s (%d)\n", LogName, errno);
		die(0);
	}
	finet = socket(AF_INET, SOCK_DGRAM, 0);
	if (finet >= 0) {
		struct servent *sp;

		sp = getservbyname("syslog", "udp");
		if (sp == NULL) {
			errno = 0;
			logerror("syslog/udp: unknown service");
			die(0);
		}
		sin.sin_family = AF_INET;
		sin.sin_port = LogPort = sp->s_port;
		if (bind(finet, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			logerror("bind");
			if (!Debug)
				die(0);
		} else {
			inetm = FDMASK(finet);
			InetInuse = 1;
		}
	}
	if ((fklog = open("/dev/klog", O_RDONLY)) >= 0)
		klogm = FDMASK(fklog);
	else {
		dprintf("can't open /dev/klog (%d)\n", errno);
		klogm = 0;
	}

	/* tuck my process id away */
	fp = fopen(PidFile, "w");
	if (fp != NULL) {
		(void) fprintf(fp, "%d\n", getpid());
		(void) fclose(fp);
	}

	dprintf("off & running....\n");

	init();
	(void) signal(SIGHUP, init);

	for (;;) {
		int nfds, readfds = FDMASK(funix) | inetm | klogm;

		errno = 0;
		dprintf("readfds = %#x\n", readfds, funix, finet, fklog);
		nfds = select(20, (fd_set *) &readfds, (fd_set *) NULL,
				  (fd_set *) NULL, (struct timeval *) NULL);
		dprintf("got a message (%d, %#x)\n", nfds, readfds);
		if (nfds == 0)
			continue;
		if (nfds < 0) {
			if (errno != EINTR)
				logerror("select");
			continue;
		}
		if (readfds & klogm) {
			i = read(fklog, line, sizeof(line) - 1);
			if (i > 0) {
				line[i] = '\0';
				printsys(line);
			} else if (i < 0 && errno != EINTR) {
				logerror("klog");
				fklog = -1;
				klogm = 0;
			}
		}
		if (readfds & FDMASK(funix)) {
			len = sizeof fromunix;
			i = recvfrom(funix, line, MAXLINE, 0,
				     (struct sockaddr *) &fromunix, &len);
			if (i > 0) {
				line[i] = '\0';
				printline(LocalHostName, line, USER_ONLY);
			} else if (i < 0 && errno != EINTR)
				logerror("recvfrom unix");
		}
		if (readfds & inetm) {
			len = sizeof frominet;
			i = recvfrom(finet, line, MAXLINE, 0,
			    (struct sockaddr *)&frominet, &len);
			if (i > 0) {
				extern char *cvthname();

				line[i] = '\0';
				printline(cvthname(&frominet), line, ALL);
			} else if (i < 0 && errno != EINTR)
				logerror("recvfrom inet");
		} 
	}
}

usage()
{
	(void) fprintf(stderr,
	    "usage: syslogd [-d] [-mmarkinterval] [-ppath] [-fconffile]\n");
	exit(1);
}

untty()
{
	int i;

	if (!Debug) {
		i = open("/dev/tty", O_RDWR);
		if (i >= 0) {
			(void) ioctl(i, (int) TIOCNOTTY, (char *)0);
			(void) close(i);
		}
	}
}

/*
 * Take a raw input line, decode the message, and print the message
 * on the appropriate log files.
 */

printline(hname, msg, force_usr)
	char *hname;
	char *msg;
	int force_usr;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri;

	/* test for special codes */
	p = msg;
	if (*p == '<' && isdigit(*(p+1))) {
		pri = 0;
		while (isdigit(*++p))
			pri = 10 * pri + (*p - '0');
		if (*p == '>')
			++p;
		dprintf("printline pri= %o\n", pri);
		if (pri < 0 || pri >= (LOG_NFACILITIES << 3))
			pri = defupri;
	} else {
		pri = defupri;
	} 

	/* don't allow users to log kernel messages */
	if (force_usr  &&  (pri & LOG_PRIMASK) == LOG_KERN)
		pri |= LOG_USER;

	q = line;

	while ((c = *p++ & 0177) != '\0' && c != '\n' &&
	    q < &line[sizeof(line) - 1]) {
		if (iscntrl(c)) {
			*q++ = '^';
			*q++ = c ^ 0100;
		} else
			*q++ = c;
	}
	*q = '\0';
	logmsg(pri, line, hname, 0);
}

/*
 * Take a raw input line from /dev/klog, split and format similar to syslog().
 */

printsys(msg)
	char *msg;
{
	register char *p, *q;
	register int c;
	char line[MAXLINE + 1];
	int pri, flags;
	char *lp;
	time_t now;

	(void) time(&now);
	(void) sprintf(line, "%.15s vmunix: ", ctime(&now) + 4);
	lp = line + strlen(line);
	for (p = msg; *p != '\0'; ) {
		flags = SYNC_FILE;	/* fsync file after write */
		if (*p == '<' && isdigit(*(p+1))) {
			pri = 0;
			while (isdigit(*++p))
				pri = 10 * pri + (*p - '0');
			if (*p == '>')
				++p;
			dprintf("printsys pri= %o\n", pri);
			if (pri < 0 || pri >= (LOG_NFACILITIES << 3))
				pri = defspri;
		} else if (*p == panic_str[0]  &&
			   bcmp(panic_str, p, sizeof(panic_str)-1) == 0) {
			/* kernel panic's come out on console */
			pri = PANICPRI;
			flags |= IGN_CONS;
		} else {
			/* kernel printf's come out on console */
			pri = defspri;
			flags |= IGN_CONS;
		}
		q = lp;
		while (*p != '\0' && (c = *p++) != '\n' &&
		    q < &line[MAXLINE])
			*q++ = c;
		*q = '\0';
		logmsg(pri, line, LocalHostName, flags);
	}
}

/*
 * Log a message to the appropriate log files, users, etc. based on
 * the priority.
 */

logmsg(pri, msg, from, flags)
	int pri;
	char *msg, *from;
	int flags;
{
	register struct filed *f;
	register int l;
	int fac, prilev;
	time_t now;
	int omask;
	struct iovec iov[6];
	register struct iovec *v = iov;
	char line[MAXLINE + 1];

	dprintf("logmsg: pri %o, flags %x, from %s, msg %s\n", pri, flags,
	    from, msg);

	omask = sigblock(sigmask(SIGHUP)|sigmask(SIGALRM));

	/*
	 * Check to see if msg looks non-standard.
	 */
	if (strlen(msg) < 16 || msg[3] != ' ' || msg[6] != ' ' ||
	    msg[9] != ':' || msg[12] != ':' || msg[15] != ' ')
		flags |= ADDDATE;

	if (!(flags & NOCOPY)) {
		if (flags & (ADDDATE|MARK))
			flushmsg();
		else if (!strcmp(msg + 16, PrevLine + 16)) {
			/* we found a match, update the time */
			(void) strncpy(PrevLine, msg, 15);
			if (PrevCount == 0) {
				FlushTimer = MarkInterval * 60 / MARKCOUNT;
				setalarm(FlushTimer);
			}
			PrevCount++;
			(void) sigsetmask(omask);
			return;
		} else {
			/* new line, save it */
			flushmsg();
			(void) strcpy(PrevLine, msg);
			(void) strcpy(PrevHost, from);
			PrevFlags = flags;
			PrevPri = pri;
		}
	}

	(void) time(&now);
	if (flags & ADDDATE)
		v->iov_base = ctime(&now) + 4;
	else
		v->iov_base = msg;
	v->iov_len = 15;
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	v->iov_base = from;
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = " ";
	v->iov_len = 1;
	v++;
	if (flags & ADDDATE)
		v->iov_base = msg;
	else
		v->iov_base = msg + 16;
	v->iov_len = strlen(v->iov_base);
	v++;

	/* extract facility and priority level */
	fac = (pri & LOG_FACMASK) >> 3;
	if (flags & MARK)
		fac = LOG_NFACILITIES;
	prilev = pri & LOG_PRIMASK;

	/* log the message to the particular outputs */
	if (!Initialized) {
		int cfd = open(ctty, O_WRONLY);

		if (cfd >= 0) {
			v->iov_base = "\r\n";
			v->iov_len = 2;
			(void) writev(cfd, iov, 6);
			(void) close(cfd);
		}
		untty();
		(void) sigsetmask(omask);
		return;
	}
	for (f = Files; f < &Files[NLOGS]; f++) {
		/* skip messages that are incorrect priority */
		if (f->f_pmask[fac] < prilev || f->f_pmask[fac] == NOPRI)
			continue;

		/* don't output marks to recently written files */
		if ((flags & MARK) && (now - f->f_time) < (MarkInterval * 60 / 2))
			continue;

		dprintf("Logging to %s", TypeNames[f->f_type]);
		f->f_time = now;
		switch (f->f_type) {
		case F_UNUSED:
			dprintf("\n");
			break;

		case F_FORW:
			dprintf(" %s\n", f->f_un.f_forw.f_hname);
			(void) sprintf(line, "<%u>%.15s %s", pri,
				iov[0].iov_base, iov[4].iov_base);
			l = strlen(line);
			if (l > MAXLINE)
				l = MAXLINE;
			if (sendto(f->f_file, line, l, 0,
			    (struct sockaddr *)&f->f_un.f_forw.f_addr,
			    sizeof f->f_un.f_forw.f_addr) != l) {
				int e = errno;
				(void) close(f->f_file);
				f->f_type = F_UNUSED;
				errno = e;
				logerror("sendto");
			}
			break;

		case F_CONSOLE:
			if (flags & IGN_CONS) {
				dprintf(" (ignored)\n");
				break;
			}

		case F_TTY:
		case F_FILE:
			dprintf(" %s\n", f->f_un.f_fname);
			if (f->f_type != F_FILE) {
				v->iov_base = "\r\n";
				v->iov_len = 2;
			} else {
				v->iov_base = "\n";
				v->iov_len = 1;
			}
			if (writev(f->f_file, iov, 6) < 0) {
				int e = errno;
				(void) close(f->f_file);
				/*
				 * Check for EBADF on TTY's due to vhangup() XXX
				 */
				if (e == EBADF && f->f_type != F_FILE) {
					f->f_file = open(f->f_un.f_fname, O_WRONLY|O_APPEND);
					if (f->f_file < 0) {
						f->f_type = F_UNUSED;
						logerror(f->f_un.f_fname);
					}
					untty();
				} else {
					f->f_type = F_UNUSED;
					errno = e;
					logerror(f->f_un.f_fname);
				}
			} else if (flags & SYNC_FILE)
				(void) fsync(f->f_file);
			break;

		case F_USERS:
		case F_WALL:
			dprintf("\n");
			v->iov_base = "\r\n";
			v->iov_len = 2;
			wallmsg(f, iov);
			break;
		}
	}

	(void) sigsetmask(omask);
}


/*
 *  WALLMSG -- Write a message to the world at large
 *
 *	Write the specified message to either the entire
 *	world, or a list of approved users.
 */

wallmsg(f, iov)
	register struct filed *f;
	struct iovec *iov;
{
	register char *p;
	register int i;
	int ttyf, len;
	FILE *uf;
	static int reenter = 0;
	struct utmp ut;
	time_t now;
	char greetings[200];

	if (reenter++)
		return;

	/* open the user login file */
	if ((uf = fopen("/etc/utmp", "r")) == NULL) {
		logerror("/etc/utmp");
		reenter = 0;
		return;
	}

	(void) time(&now);
	(void) sprintf(greetings,
	    "\r\n\7Message from syslogd@%s at %.24s ...\r\n",
		iov[2].iov_base, ctime(&now));
	len = strlen(greetings);

	/* scan the user login file */
	while (fread((char *) &ut, sizeof ut, 1, uf) == 1) {
		/* is this slot used? */
		if (ut.ut_name[0] == '\0')
			continue;

		/* should we send the message to this user? */
		if (f->f_type == F_USERS) {
			for (i = 0; i < MAXUNAMES; i++) {
				if (!f->f_un.f_uname[i][0]) {
					i = MAXUNAMES;
					break;
				}
				if (strncmp(f->f_un.f_uname[i], ut.ut_name,
				    UNAMESZ) == 0)
					break;
			}
			if (i >= MAXUNAMES)
				continue;
		}

		/* compute the device name */
		p = "/dev/12345678";
		(void) strncpy(&p[5], ut.ut_line, UNAMESZ);

		/*
		 * Might as well fork instead of using nonblocking I/O
		 * and doing notty().
		 */
		if (fork() == 0) {
			if (f->f_type == F_WALL) {
				iov[0].iov_base = greetings;
				iov[0].iov_len = len;
				iov[1].iov_len = 0;
			}
			(void) signal(SIGALRM, SIG_DFL);
			(void) alarm(30);
			/* open the terminal */
			ttyf = open(p, O_WRONLY);
			if (ttyf >= 0) {
				struct stat statb;

				if (fstat(ttyf, &statb) == 0 &&
				    (statb.st_mode & S_IWRITE))
					(void) writev(ttyf, iov, 6);
			}
			exit(0);
		}
	}
	/* close the user login file */
	(void) fclose(uf);
	reenter = 0;
}

void
reapchild()
{
	union wait status;

	while (wait3(&status, WNOHANG, (struct rusage *) NULL) > 0)
		;
}

/*
 * Return a printable representation of a host address.
 */
char *
cvthname(f)
	struct sockaddr_in *f;
{
	register char *p;
	register struct hashent *h, *last;
	struct hostent *hp;
	unsigned long addr;
	extern char *inet_ntoa();

	dprintf("cvthname(%s)\n", inet_ntoa(f->sin_addr));

	if (f->sin_family != AF_INET) {
		dprintf("Malformed from address\n");
		return ("???");
	}

	/*
	 * First look in the hash table, and return if
	 * found.  Otherwise malloc a new entry, chain onto
	 * the end, and do a real host name lookup.  Returns
	 * a pointer to the string part of the table entry.
	 * Note the parameter must be treated as an UNSIGNED
	 * integer.
	 */
	addr = *(unsigned long *)(&f->sin_addr);
	h = Table[addr % HASHSIZE];
	last = NULL;
	while (h != NULL) {
		if (h->addr == addr)
			return (h->name);
		last = h;
		h = h->next;
	}
	h = (struct hashent *)malloc(sizeof (struct hashent));
	if (h == NULL)
		return (inet_ntoa(f->sin_addr));
	h->addr = addr;
	h->next = NULL;
	if (last == NULL)
		Table[addr % HASHSIZE] = h;
	else
		last->next = h;

	hp = gethostbyaddr((char *)&f->sin_addr, 4, AF_INET);
	if (hp == NULL) {
		dprintf("Host name for your address (%s) unknown\n",
			inet_ntoa(f->sin_addr));
		(void) strncpy(h->name, inet_ntoa(f->sin_addr),
		    sizeof (h->name));
	} else {
		if ((p = index(hp->h_name, '.')) &&
		    strcmp(p + 1, LocalDomain) == 0)
			*p = '\0';
		(void) strncpy(h->name, hp->h_name, sizeof (h->name));
	}
	return (h->name);
}

int	curalarm;		/* current alarm value */

/*
 * If the alarm is more than "secs" seconds in the future, set it to "secs"
 * seconds.  Adjust any timers by subtracting the time elapsed since the last
 * "alarm" call.
 */
setalarm(secs)
	int secs;
{
	register int alarmval;
	register int elapsed;

	alarmval = alarm((unsigned)0);
	elapsed = curalarm - alarmval;
	dprintf("setalarm: curalarm %d alarmval %d\n", curalarm, alarmval);
	if (PrevCount > 0)
		FlushTimer -= elapsed;
	if (Marking)
		MarkTimer -= elapsed;
	if (secs < alarmval || alarmval == 0)
		curalarm = secs;
	else
		curalarm = alarmval;
	(void) alarm((unsigned)curalarm);
	dprintf("Next alarm in %d seconds\n", curalarm);
}

/*
 * SIGALRM catcher: adjust the timers, call the appropriate timeout routines,
 * and set up the next alarm.
 */
void
doalarm()
{
	dprintf("doalarm: FlushTimer %d MarkTimer %d curalarm %d\n",
	    FlushTimer, MarkTimer, curalarm);
	if (PrevCount > 0) {
		FlushTimer -= curalarm;
		if (FlushTimer <= 0)
			flushmsg();
	}
	if (Marking) {
		MarkTimer -= curalarm;
		if (MarkTimer <= 0) {
			logmsg(LOG_INFO, "-- MARK --", LocalHostName,
			    ADDDATE|MARK);
			MarkTimer = MarkInterval * 60;
		}
	}
	curalarm = 0;
	if (FlushTimer > 0)
		curalarm = FlushTimer;
	if (Marking && MarkTimer > 0
	    && (MarkTimer < curalarm || curalarm == 0))
		curalarm = MarkTimer;
	(void) alarm((unsigned)curalarm);
	dprintf("Next alarm in %d seconds\n", curalarm);
}

flushmsg()
{
	FlushTimer = 0;
	if (PrevCount == 0)
		return;
	if (PrevCount > 1)
		(void) sprintf(PrevLine+16, "last message repeated %d times",
		    PrevCount);
	PrevCount = 0;
	logmsg(PrevPri, PrevLine, PrevHost, PrevFlags|NOCOPY);
	PrevLine[0] = '\0';
}

/*
 * Print syslogd errors some place.
 */
logerror(type)
	char *type;
{
	char buf[100];

	if (errno == 0)
		(void) sprintf(buf, "syslogd: %s", type);
	else if ((unsigned) errno > sys_nerr)
		(void) sprintf(buf, "syslogd: %s: error %d", type, errno);
	else
		(void) sprintf(buf, "syslogd: %s: %s", type, sys_errlist[errno]);
	errno = 0;
	dprintf("%s\n", buf);
	logmsg(LOG_SYSLOG|LOG_ERR, buf, LocalHostName, ADDDATE);
}

void
die(sig)
{
	char buf[100];

	if (sig) {
		dprintf("syslogd: going down on signal %d\n", sig);
		flushmsg();
		(void) sprintf(buf, "going down on signal %d", sig);
		errno = 0;
		logerror(buf);
	}
	(void) unlink(LogName);
	exit(0);
}

/*
 *  INIT -- Initialize syslogd from configuration table
 */
void
init()
{
	register int i;
	register FILE *cf;
	register struct filed *f;
	register char *p;
	char cline[BUFSIZ];

	dprintf("init\n");

	/* flush any pending output */
	flushmsg();

	/*
	 *  Close all open log files.
	 */
	Initialized = 0;
	for (f = Files; f < &Files[NLOGS]; f++) {
		switch (f->f_type) {
		case F_FILE:
		case F_TTY:
		case F_FORW:
		case F_CONSOLE:
			(void) close(f->f_file);
			f->f_type = F_UNUSED;
			break;
		}
	}

	/* get the list of my addresses */
	getmyaddr();

	/* open the configuration file */
	if ((cf = fopen(ConfFile, "r")) == NULL) {
nofile:
		dprintf("cannot open %s\n", ConfFile);
		cfline("*.ERR\t/dev/console", 0, &Files[0]);
		cfline("*.PANIC\t*", 0, &Files[1]);
		return;
	}

	/*
	 * Run the configuration file through m4 to handle any ifdefs.
	 */
	(void) fclose(cf);
	(void) sprintf(cline, "echo '%s' | /usr/bin/m4 - %s",
	    ismyaddr(gethostbyname("loghost")) ? "define(LOGHOST, 1)" : "",
	    ConfFile);
	if ((cf = popen(cline, "r")) == NULL) {
		goto nofile;
	}

	/*
	 *  Foreach line in the conf table, open that file.
	 */
	f = Files;
	i = 0;
	while (fgets(cline, sizeof cline, cf) != NULL && f < &Files[NLOGS]) {
		i++;
		/* check for end-of-section */
		if (cline[0] == '\n' || cline[0] == '#')
			continue;

		/* strip off newline character */
		p = index(cline, '\n');
		if (p)
			*p = '\0';

		cfline(cline, i, f++);
	}

	/* close the configuration file */
	(void) fclose(cf);

	Initialized = 1;

	if (Debug) {
		for (f = Files; f < &Files[NLOGS]; f++) {
			for (i = 0; i <= LOG_NFACILITIES; i++)
				if (f->f_pmask[i] == NOPRI)
					(void) printf("X ");
				else
					(void) printf("%d ", f->f_pmask[i]);
			(void) printf("%s: ", TypeNames[f->f_type]);
			switch (f->f_type) {
			case F_FILE:
			case F_TTY:
			case F_CONSOLE:
				(void) printf("%s", f->f_un.f_fname);
				break;

			case F_FORW:
				(void) printf("%s", f->f_un.f_forw.f_hname);
				break;

			case F_USERS:
				for (i = 0; i < MAXUNAMES && *f->f_un.f_uname[i]; i++)
					(void) printf("%s, ",
					    f->f_un.f_uname[i]);
				break;
			}
			(void) printf("\n");
		}
	}

	/*
	 * See if marks are to be written to any files.  If so, set up a
	 * timeout for marks.
	 */
	Marking = 0;
	for (f = Files; f < &Files[NLOGS]; f++) {
		if (f->f_type != F_UNUSED
		    && f->f_pmask[LOG_NFACILITIES] != NOPRI)
			Marking = 1;
	}
	if (Marking) {
		i = sigblock(sigmask(SIGALRM));
		setalarm(MarkInterval * 60);
		(void) sigsetmask(i);
	}
	logmsg(LOG_SYSLOG|LOG_INFO, "syslogd: restart", LocalHostName, ADDDATE);
	dprintf("syslogd: restarted\n");
}

/*
 * Crack a configuration file line
 */

struct code {
	char	*c_name;
	int	c_val;
};

struct code	PriNames[] = {
	"panic",	LOG_EMERG,
	"emerg",	LOG_EMERG,
	"alert",	LOG_ALERT,
	"crit",		LOG_CRIT,
	"err",		LOG_ERR,
	"error",	LOG_ERR,
	"warn",		LOG_WARNING,
	"warning",	LOG_WARNING,
	"notice",	LOG_NOTICE,
	"info",		LOG_INFO,
	"debug",	LOG_DEBUG,
	"none",		NOPRI,
	NULL,		-1
};

struct code	FacNames[] = {
	"kern",		LOG_KERN,
	"user",		LOG_USER,
	"mail",		LOG_MAIL,
	"daemon",	LOG_DAEMON,
	"auth",		LOG_AUTH,
	"security",	LOG_AUTH,
	"mark",		LOG_MARK,
	"syslog",	LOG_SYSLOG,
	"lpr",		LOG_LPR,
	"news",		LOG_NEWS,
	"uucp",		LOG_UUCP,
	"cron",		LOG_CRON,
	"local0",	LOG_LOCAL0,
	"local1",	LOG_LOCAL1,
	"local2",	LOG_LOCAL2,
	"local3",	LOG_LOCAL3,
	"local4",	LOG_LOCAL4,
	"local5",	LOG_LOCAL5,
	"local6",	LOG_LOCAL6,
	"local7",	LOG_LOCAL7,
	NULL,		-1
};

cfline(line, lineno, f)
	char *line;
	int lineno;
	register struct filed *f;
{
	register char *p;
	register char *q;
	register int i;
	char *bp;
	int pri;
	struct hostent *hp;
	char buf[MAXLINE];
	char xbuf[200];

	dprintf("cfline(%s)\n", line);

	errno = 0;	/* keep sys_errlist stuff out of logerror messages */

	/* clear out file entry */
	bzero((char *) f, sizeof *f);
	for (i = 0; i <= LOG_NFACILITIES; i++)
		f->f_pmask[i] = NOPRI;

	/* scan through the list of selectors */
	for (p = line; *p && *p != '\t';) {

		/* find the end of this facility name list */
		for (q = p; *q && *q != '\t' && *q++ != '.'; )
			continue;

		/* collect priority name */
		for (bp = buf; *q && !index("\t,;", *q); )
			*bp++ = *q++;
		*bp = '\0';

		/* skip cruft */
		while (index(", ;", *q))
			q++;

		/* decode priority name */
		pri = decode(buf, PriNames);
		if (pri < 0) {
			(void) sprintf(xbuf, "line %d: unknown priority name \"%s\"",
			    lineno,  buf);
			logerror(xbuf);
			return;
		}

		/* scan facilities */
		while (*p && !index("\t.;", *p)) {
			for (bp = buf; *p && !index("\t,;.", *p); )
				*bp++ = *p++;
			*bp = '\0';
			if (*buf == '*')
				for (i = 0; i < LOG_NFACILITIES; i++)
					f->f_pmask[i] = pri;
			else {
				i = decode(buf, FacNames);
				if (i < 0) {
					(void) sprintf(xbuf, "line %d: unknown facility name \"%s\"",
					    lineno, buf);
					logerror(xbuf);
					return;
				}
				f->f_pmask[i >> 3] = pri;
			}
			while (*p == ',' || *p == ' ')
				p++;
		}

		p = q;
	}

	/* skip to action part */
	while (*p == '\t' || *p == ' ')
		p++;

	switch (*p)
	{
	case '\0':
		(void) sprintf(xbuf, "line %d: no action part", lineno);
		errno = 0;
		logerror(xbuf);
		break;

	case '@':
		if (!InetInuse)
			break;
		(void) strcpy(f->f_un.f_forw.f_hname, ++p);
		hp = gethostbyname(p);
		if (hp == NULL) {
			(void) sprintf(xbuf, "line %d: unknown host %s",
			    lineno, p);
			errno = 0;
			logerror(xbuf);
			break;
		}
		if (ismyaddr(hp)) {
			(void) sprintf(xbuf,
			    "line %d: host %s is this host - logging loop",
			    lineno, p);
			errno = 0;
			logerror(xbuf);
			break;
		}
		bzero((char *) &f->f_un.f_forw.f_addr,
			 sizeof f->f_un.f_forw.f_addr);
		f->f_un.f_forw.f_addr.sin_family = AF_INET;
		f->f_un.f_forw.f_addr.sin_port = LogPort;
		bcopy(hp->h_addr, (char *) &f->f_un.f_forw.f_addr.sin_addr, hp->h_length);
		f->f_file = socket(AF_INET, SOCK_DGRAM, 0);
		if (f->f_file < 0) {
			logerror("socket");
			break;
		}
		f->f_type = F_FORW;
		break;

	case '/':
		(void) strcpy(f->f_un.f_fname, p);
		if ((f->f_file = open(p, O_WRONLY|O_APPEND)) < 0) {
			logerror(p);
			break;
		}
		if (isatty(f->f_file)) {
			f->f_type = F_TTY;
			untty();
		}
		else
			f->f_type = F_FILE;
		if (strcmp(p, ctty) == 0)
			f->f_type = F_CONSOLE;
		break;

	case '*':
		f->f_type = F_WALL;
		break;

	default:
		for (i = 0; i < MAXUNAMES && *p; i++) {
			for (q = p; *q && *q != ','; )
				q++;
			(void) strncpy(f->f_un.f_uname[i], p, UNAMESZ);
			if ((q - p) > UNAMESZ)
				f->f_un.f_uname[i][UNAMESZ] = '\0';
			else
				f->f_un.f_uname[i][q - p] = '\0';
			while (*q == ',' || *q == ' ')
				q++;
			p = q;
		}
		f->f_type = F_USERS;
		break;
	}
}


/*
 *  Decode a symbolic name to a numeric value
 */

decode(name, codetab)
	char *name;
	struct code *codetab;
{
	register struct code *c;
	register char *p;
	char buf[40];

	if (isdigit(*name))
		return (atoi(name));

	(void) strcpy(buf, name);
	for (p = buf; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	for (c = codetab; c->c_name; c++)
		if (!strcmp(buf, c->c_name))
			return (c->c_val);

	return (-1);
}

char addr_buf[BUFSIZ];
struct ifconf ifc;

getmyaddr()
{
	int s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		logerror("socket");
		return;
	}
	ifc.ifc_len = sizeof (addr_buf);
	ifc.ifc_buf = addr_buf;
	if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
		logerror("ioctl (get interface configuration)");
		return;
	}
	(void) close(s);
}

ismyaddr(hp)
	register struct hostent *hp;
{
	register struct ifreq *ifr;
	register char **ap;
	register int len;

	if (hp == NULL)
		return (0);
	ifr = ifc.ifc_req;
	for (len = ifc.ifc_len; len; len -= sizeof (struct ifreq)) {
		if (hp->h_addrtype == ifr->ifr_addr.sa_family &&
		    hp->h_addrtype == AF_INET) {
			for (ap = hp->h_addr_list; *ap; ap++) {
				register struct sockaddr_in *ifin;

				ifin = (struct sockaddr_in *)&ifr->ifr_addr;
				if (bcmp(*ap, (char *)&ifin->sin_addr,
				    hp->h_length) == 0)
					return (1);
			}
		}
		ifr++;
	}
	return (0);
}
