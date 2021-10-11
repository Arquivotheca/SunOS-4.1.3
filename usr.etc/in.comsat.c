/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char *sccsid = "@(#)in.comsat.c 1.1 92/07/30 SMI"; /* from UCB 5.5 10/24/85 */
#endif not lint

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <stdio.h>
#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>

/*
 * comsat
 */
int	debug = 0;
#define	dprintf	if (debug) printf

struct	sockaddr_in sin = { AF_INET };
extern	errno;

char	hostname[MAXHOSTNAMELEN];
struct	utmp *utmp = NULL;
int	nutmp;
int	uf;
unsigned utmpmtime = 0;			/* last modification time for utmp */
unsigned utmpsize = 0;			/* last malloced size for utmp */
int	onalrm();
int	reapchildren();
long	lastmsgtime;
char 	*malloc(), *realloc();

#define	MAXIDLE	120

main(argc, argv)
	int argc;
	char *argv[];
{
	register int cc;
	char msgbuf[BUFSIZ];
	struct sockaddr_in from;
	int fromlen;

	/* verify proper invocation */
	fromlen = sizeof (from);
	if (getsockname(0, &from, &fromlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getsockname");
		_exit(1);
	}
	chdir("/var/spool/mail");
	if ((uf = open("/etc/utmp",0)) < 0) {
		openlog("comsat", 0, LOG_DAEMON);
		syslog(LOG_ERR, "/etc/utmp: %m");
		(void) recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		exit(1);
	}
	lastmsgtime = time(0);
	gethostname(hostname, sizeof (hostname));
	onalrm();
	signal(SIGALRM, onalrm);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGCHLD, reapchildren);
	for (;;) {
		cc = recv(0, msgbuf, sizeof (msgbuf) - 1, 0);
		if (cc <= 0) {
			if (errno != EINTR)
				sleep(1);
			errno = 0;
			continue;
		}
		if (nutmp == 0)			/* no users (yet) */
			continue;
		sigblock(sigmask(SIGALRM));
		msgbuf[cc] = 0;
		lastmsgtime = time(0);
		mailfor(msgbuf);
		sigsetmask(0);
	}
}

reapchildren()
{

	while (wait3((struct wait *)0, WNOHANG, (struct rusage *)0) > 0)
		;
}

onalrm()
{
	struct stat statbf;

	if (time(0) - lastmsgtime >= MAXIDLE)
		exit(0);
	dprintf("alarm\n");
	alarm(15);
	fstat(uf, &statbf);
	if (statbf.st_mtime > utmpmtime) {
		dprintf(" changed\n");
		utmpmtime = statbf.st_mtime;
		if (statbf.st_size > utmpsize) {
			utmpsize = statbf.st_size + 10 * sizeof(struct utmp);
			if (utmp)
				utmp = (struct utmp *)realloc(utmp, utmpsize);
			else
				utmp = (struct utmp *)malloc(utmpsize);
			if (! utmp) {
				dprintf("malloc failed\n");
				exit(1);
			}
		}
		lseek(uf, 0, 0);
		nutmp = read(uf,utmp,statbf.st_size)/sizeof(struct utmp);
	} else
		dprintf(" ok\n");
}

mailfor(name)
	char *name;
{
	register struct utmp *utp = &utmp[nutmp];
	register char *cp;
	char *rindex();
	int offset;

	  /*
	   * Don't bother doing anything if nobody has every
	   * logged into the system.
	   */
	if (utmp == NULL || nutmp == 0)
		return;
	dprintf("mailfor %s\n", name);
	cp = name;
	while (*cp && *cp != '@')
		cp++;
	if (*cp == 0) {
		dprintf("bad format\n");
		return;
	}
	*cp = 0;
	offset = atoi(cp+1);
	while (--utp >= utmp)
		if (!strncmp(utp->ut_name, name, sizeof(utmp[0].ut_name)))
			notify(utp, offset);
}

char	*cr;

notify(utp, offset)
	register struct utmp *utp;
{
	FILE *tp;
	struct sgttyb gttybuf;
	char tty[20], name[sizeof (utmp[0].ut_name) + 1];
	struct stat stb;
	time_t timep[2];
	struct passwd *pwd;

	strcpy(tty, "/dev/");
	strncat(tty, utp->ut_line, sizeof(utp->ut_line));
	dprintf("notify %s on %s\n", utp->ut_name, tty);
	if (stat(tty, &stb) == 0 && (stb.st_mode & 0100) == 0) {
		dprintf("wrong mode\n");
		return;
	}
	if (fork())
		return;
	signal(SIGALRM, SIG_DFL);
	alarm(30);
	/* 
	 * Do all operations that check protections as the user who
	 * will be getting the biff.
	 */
	if ((pwd = getpwnam(utp->ut_name)) == (struct passwd *) -1) {
		dprintf("getpwnam failed\n");
		exit(1);
	}
	if (setuid(pwd->pw_uid) == -1) {
		dprintf("setuid failed\n");
		exit(1);
	}
	if ((tp = fopen(tty,"w")) == 0) {
		dprintf("fopen failed\n");
		exit(-1);
	}
	ioctl(fileno(tp), TIOCGETP, &gttybuf);
	cr = (gttybuf.sg_flags&CRMOD) && !(gttybuf.sg_flags&RAW) ? "" : "\r";
	strncpy(name, utp->ut_name, sizeof (utp->ut_name));
	name[sizeof (name) - 1] = '\0';
	fprintf(tp,"%s\n\007New mail for %s@%.*s\007 has arrived:%s\n",
	    cr, name, sizeof (hostname), hostname, cr);
	fprintf(tp,"----%s\n", cr);
	stat(name, &stb);
	timep[0] = stb.st_atime;
	timep[1] = stb.st_mtime;
	jkfprintf(tp, name, offset);
	utime(name, timep);
	exit(0);
}

jkfprintf(tp, name, offset)
	register FILE *tp;
{
	register FILE *fi;
	register int linecnt, charcnt;
	char line[BUFSIZ];
	int inheader;

	dprintf("HERE %s's mail starting at %d\n",
	    name, offset);
	if ((fi = fopen(name,"r")) == NULL) {
		dprintf("Cant read the mail\n");
		return;
	}
	fseek(fi, offset, L_SET);
	/* 
	 * Print the first 7 lines or 560 characters of the new mail
	 * (whichever comes first).  Skip header crap other than
	 * From, Subject, To, and Date.
	 */
	linecnt = 7;
	charcnt = 560;
	inheader = 1;
	while (fgets(line, sizeof (line), fi) != NULL) {
		register char *cp;
		char *index();
		int cnt;

		if (linecnt <= 0 || charcnt <= 0) {  
			fprintf(tp,"...more...%s\n", cr);
			return;
		}
		if (strncmp(line, "From ", 5) == 0)
			continue;
		if (inheader && (line[0] == ' ' || line[0] == '\t'))
			continue;
		cp = index(line, ':');
		if (cp == 0 || (index(line, ' ') && index(line, ' ') < cp))
			inheader = 0;
		else
			cnt = cp - line;
		if (inheader &&
		    strncmp(line, "Date", cnt) &&
		    strncmp(line, "From", cnt) &&
		    strncmp(line, "Subject", cnt) &&
		    strncmp(line, "To", cnt))
			continue;
		cp = index(line, '\n');
		if (cp)
			*cp = '\0';
		fprintf(tp,"%s%s\n", line, cr);
		linecnt--, charcnt -= strlen(line);
	}
	fprintf(tp,"----%s\n", cr);
}
