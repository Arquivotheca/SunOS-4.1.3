/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)printjob.c 1.1 92/07/30 SMI"; /* from UCB 5.2 9/17/85 */
#endif not lint

/*
 * printjob -- print jobs in the queue.
 *
 *	NOTE: the lock file is used to pass information to lpq and lprm.
 *	it does not need to be removed because file locks are dynamic.
 */

#include "lp.h"

#include <sys/termios.h>
#include <sys/ttold.h>

#define	DORETURN	0	/* absorb fork error */
#define	DOABORT		1	/* abort if dofork fails */

/*
 * Error tokens
 */
#define	REPRINT		-2
#define	ERROR		-1
#define	OK		0
#define	FATALERR	1
#define	NOACCT		2
#define	FILTERERR	3
#define	ACCESS		4

static char	title[80];		/* ``pr'' title */
static FILE	*cfp;			/* control file */
static int	pfd;			/* printer file descriptor */
static int	ofd;			/* output filter file descriptor */
static int	lfd;			/* lock file descriptor */
static int	pid;			/* pid of lpd process */
static int	prchild;		/* id of pr process */
static int	child;			/* id of any filters */
static int	ofilter;		/* id of output filter, if any */
static int	tof;			/* true if at top of form */
static int	remote;			/* true if sending files to remote */
static dev_t	fdev;			/* device of file pointed to by symlink */
static ino_t	fino;			/* inode of file pointed to by symlink */

static char	fromhost[MAXHOSTNAMELEN + 1]; /* user's host machine */
static char	logname[32];		/* user's login name */
static char	jobname[100];		/* job or file name */
static char	class[MAXHOSTNAMELEN + 1]; /* classification field */
#define	MAXDIGITS	11		/* max. length of decimal number */
#define	OPTSIZE		(MAXDIGITS + 3)	/* max. length of "-xN"-style option */
static char	width[OPTSIZE] = "-w";	/* page width in characters */
static char	length[OPTSIZE] = "-l";	/* page length in lines */
static char	pxwidth[OPTSIZE] = "-x"; /* page width in pixels */
static char	pxlength[OPTSIZE] = "-y"; /* page length in pixels */
static char	indent[OPTSIZE] = "-i0"; /* indentation size in characters */
static char	errfile[] = "errsXXXXXX"; /* file name for filter output */

static char *MS;
static struct termios termios_set;
static struct termios termios_clear;

static void print_filter_messages();
void free();

printjob()
{
	struct stat stb;
	register struct queue *q, **qp;
	struct queue **queue;
	register int i, nitems;
	long pidoff;
	int count = 0;
	extern void abortpr();
	extern char *mktemp();
	extern off_t lseek();

	init();					/* set up capabilities */
	(void)write(1, "", 1);			/* ack that daemon is started */
	(void)close(2);				/* set up log file */
	if (open(LF, O_WRONLY|O_APPEND, 0664) < 0) {
		syslog(LOG_ERR, "%s: %m", LF);
		(void)open("/dev/null", O_WRONLY, 0);
	}
	setgid(getegid());
	pid = getpid();				/* for use with lprm */
	setpgrp(0, pid);
	signal(SIGHUP, abortpr);
	signal(SIGINT, abortpr);
	signal(SIGQUIT, abortpr);
	signal(SIGTERM, abortpr);

	(void) mktemp(errfile);

	/*
	 * uses short form file names
	 */
	if (chdir(SD) < 0) {
		syslog(LOG_ERR, "%s: %m", SD);
		exit(1);
	}
	lfd = open(LO, O_WRONLY|O_CREAT, 0644);
	if (lfd < 0) {
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	if (fstat(lfd, &stb) == -1) {
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	if (stb.st_mode & 0100)
		exit(0);		/* printing disabled */
	if (flock(lfd, LOCK_EX|LOCK_NB) < 0) {
		if (errno == EWOULDBLOCK)	/* active daemon present */
			exit(0);
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	(void) ftruncate(lfd, (off_t)0);
	/*
	 * write process id for others to know
	 */
	(void) sprintf(line, "%u\n", pid);
	pidoff = i = strlen(line);
	if (write(lfd, line, i) != i) {
		syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		exit(1);
	}
	/*
	 * search the spool directory for work and sort by queue order.
	 */
	if ((nitems = getq(&queue)) < 0) {
			syslog(LOG_ERR, "%s: can't scan %s", printer, SD);
			exit(1);
	}
	if (nitems == 0)		/* no work to do */
		exit(0);
	if (stb.st_mode & 01) {		/* reset queue flag */
		if (fchmod(lfd, (int)(stb.st_mode & 0776)) < 0)
			syslog(LOG_ERR, "%s: %s: %m", printer, LO);
	}
	openpr();			/* open printer or remote */
again:
	/*
	 * we found something to do now do it --
	 *    write the name of the current control file into the lock file
	 *    so the spool queue program can tell what we're working on
	 */
	for (qp = queue; nitems--; free((char *) q)) {
		q = *qp++;
		if (stat(q->q_name, &stb) < 0)
			continue;
	restart:
		(void) lseek(lfd, pidoff, 0);
		(void) sprintf(line, "%s\n", q->q_name);
		i = strlen(line);
		if (write(lfd, line, i) != i)
			syslog(LOG_ERR, "%s: %s: %m", printer, LO);
		if (!remote)
			i = printit(q->q_name);
		else
			i = sendit(q->q_name);
		/*
		 * Check to see if we are supposed to stop printing or
		 * if we are to rebuild the queue.
		 */
		if (fstat(lfd, &stb) == 0) {
			/* stop printing before starting next job? */
			if (stb.st_mode & 0100)
				goto done;
			/* rebuild queue (after lpc topq) */
			if (stb.st_mode & 01) {
				for (free((char *) q); nitems--; free((char *) q))
					q = *qp++;
				if (fchmod(lfd, (int)(stb.st_mode & 0776)) < 0)
					syslog(LOG_WARNING, "%s: %s: %m",
						printer, LO);
				break;
			}
		}
		if (i == OK)		/* file ok and printed */
			count++;
		else if (i == REPRINT) { /* try reprinting the job */
			syslog(LOG_INFO, "restarting %s", printer);
			if (ofilter > 0) {
				(void) kill(ofilter, SIGCONT);	/* to be sure */
				(void) close(ofd);
				do
					i = wait((union wait *)0);
				while (i > 0 && i != ofilter);
				ofilter = 0;
			}
			(void) close(pfd);	/* close printer */
			if (ftruncate(lfd, pidoff) < 0)
				syslog(LOG_WARNING, "%s: %s: %m", printer, LO);
			openpr();		/* try to reopen printer */
			goto restart;
		}
	}
	free((char *) queue);
	/*
	 * search the spool directory for more work.
	 */
	if ((nitems = getq(&queue)) < 0) {
		syslog(LOG_ERR, "%s: can't scan %s", printer, SD);
		exit(1);
	}
	if (nitems == 0) {		/* no more work to do */
	done:
		if (count > 0) {	/* Files actually printed */
			if (!SF && !tof)
				(void) write(ofd, FF, strlen(FF));
			if (TR != NULL)		/* output trailer */
				(void) write(ofd, TR, strlen(TR));
		}
		(void) unlink(errfile);
		exit(0);
	}
	goto again;
}

char	fonts[4][50];	/* fonts for troff */

static char ifonts[4][18] = {
	"/usr/lib/vfont/R",
	"/usr/lib/vfont/I",
	"/usr/lib/vfont/B",
	"/usr/lib/vfont/S"
};

/*
 * The remaining part is the reading of the control file (cf)
 * and performing the various actions.
 */
static int
printit(file)
	char *file;
{
	register int i;
	register int print_status = OK;
	register int notify_user = 0;

	/*
	 * open control file; ignore if no longer there.
	 */
	if ((cfp = fopen(file, "r")) == NULL) {
		syslog(LOG_INFO, "%s: %s: %m", printer, file);
		return (OK);
	}
	/*
	 * Reset things that change between jobs.
	 */
	for (i = 0; i != 4; i++)
		strcpy(fonts[i], ifonts[i]);
	(void) sprintf(&width[2], "%d", PW);
	strcpy(indent + 2, "0");

	/*
	 *	read the control file for work to do
	 *
	 *	file format -- first character in the line is a command
	 *	rest of the line is the argument.
	 *	valid commands are:
	 *
	 *		S -- "stat info" for symbolic link protection
	 *		J -- "job name" on banner page
	 *		C -- "class name" on banner page
	 *		L -- "literal" user's name to print on banner
	 *		T -- "title" for pr
	 *		H -- "host name" of machine where lpr was done
	 *		P -- "person" user's login name
	 *		I -- "indent" amount to indent output
	 *		f -- "file name" name of text file to print
	 *		l -- "file name" text file with control chars
	 *		p -- "file name" text file to print with pr(1)
	 *		t -- "file name" troff(1) file to print
	 *		n -- "file name" ditroff(1) file to print
	 *		d -- "file name" dvi file to print
	 *		g -- "file name" plot(1G) file to print
	 *		v -- "file name" plain raster file to print
	 *		c -- "file name" cifplot file to print
	 *		1 -- "R font file" for troff
	 *		2 -- "I font file" for troff
	 *		3 -- "B font file" for troff
	 *		4 -- "S font file" for troff
	 *		N -- "name" of file (used by lpq)
	 *		U -- "unlink" name of file to remove
	 *			(after we print it. (Pass 2 only)).
	 *		M -- "mail" to user when done printing
	 *
	 *	getline reads a line and expands tabs to blanks
	 */

	/* pass 1 */

	while (getline(cfp) && print_status == OK)
		switch (line[0]) {
		case 'H':
			strncpy(fromhost, line + 1, (sizeof fromhost) - 1);
			if (class[0] == '\0')
				strncpy(class, line + 1, (sizeof class) - 1);
			continue;

		case 'P':
			strncpy(logname, line + 1, (sizeof logname) - 1);
			if (RS && getpwnam(logname) == 0) { /* restricted */
				print_status = NOACCT;
				notify_user = 1;
				continue;
			}
			continue;

		case 'S':
			sscanf(line + 1, "%hu%lu", &fdev, &fino);
			continue;

		case 'J':
			if (line[1] != '\0')
				strncpy(jobname, line + 1, (sizeof jobname) - 1);
			else
				strcpy(jobname, " ");
			continue;

		case 'C':
			if (line[1] != '\0')
				strncpy(class, line + 1, (sizeof class) - 1);
			else if (class[0] == '\0')
				gethostname(class, (sizeof class));
			continue;

		case 'T':	/* header title for pr */
			strncpy(title, line + 1, (sizeof title) - 1);
			continue;

		case 'L':	/* identification line */
			if (!SH && !HL)
				banner(line + 1, jobname);
			continue;

		case '1':	/* troff fonts */
		case '2':
		case '3':
		case '4':
			if (line[1] != '\0')
				strcpy(fonts[line[0] - '1'], line + 1);
			continue;

		case 'W':	/* page width */
			strncpy(width + 2, line + 1, (sizeof width) - 3);
			continue;

		case 'I':	/* indent amount */
			strncpy(indent + 2, line + 1, (sizeof indent) - 3);
			continue;

		default:	/* some file to print */
			switch (print(line[0], line + 1)) {
			case ERROR:
				print_status = FATALERR;
				notify_user = 1;
				break;
			case REPRINT:
				(void) fclose(cfp);
				return (REPRINT);
			case ACCESS:
				print_status = ACCESS;
				notify_user = 1;
				break;
			}
			continue;

		case 'N':
		case 'U':
		case 'M':
			continue;
		}

	/* pass 2 */

	(void) fseek(cfp, 0L, 0);
	while (getline(cfp)) {
		switch (line[0]) {
		case 'L':	/* identification line */
			if (!SH && HL)
				banner(line + 1, jobname);
			continue;

		case 'M':
			notify_user = 1;
			continue;

		case 'U':
			if (!from_remote || safepath(line + 1))
				(void) unlink(line + 1);
			title[0] = '\0';
		}
	}

	print_filter_messages(stderr, (char *)NULL);
	if (notify_user)
		sendmail(logname, print_status);

	/*
	 * clean-up in case another control file exists
	 */
	(void) fclose(cfp);
	(void) unlink(file);
	title[0] = '\0';
	return (print_status == OK ? OK : ERROR);
}

/*
 * Print a file.
 * Set up the chain [ PR [ | {IF, OF} ] ] or {IF, RF, TF, NF, DF, CF, VF}.
 * Return -1 if a non-recoverable error occured,
 * 2 if the filter detected some errors (but printed the job anyway),
 * 1 if we should try to reprint this job and
 * 0 if all is well.
 * Note: all filters take stdin as the file, stdout as the printer,
 * stderr as the log file, and must not ignore SIGINT.
 */
static int
print(format, file)
	int format;
	char *file;
{
	register int n;
	register char *prog;
	int fi, fo;
	char *av[15], buf[BUFSIZ];
	int pid, p[2], stopped = 0;
	union wait status;
	struct stat stb;

	if (!safepath(file) || lstat(file, &stb) < 0 ||
			(fi = open(file, O_RDONLY)) < 0)
		return (ERROR);
	/*
	 * Check to see if data file is a symbolic link. If so, it should
	 * still point to the same file or someone is trying to print
	 * something he shouldn't.
	 */
	if ((stb.st_mode & S_IFMT) == S_IFLNK && fstat(fi, &stb) == 0 &&
	    (stb.st_dev != fdev || stb.st_ino != fino))
		return (ACCESS);
	if (!SF && !tof) {		/* start on a fresh page */
		(void) write(ofd, FF, strlen(FF));
		tof = 1;
	}
	if (IF == NULL && (format == 'f' || format == 'l')) {
		tof = 0;
		while ((n = read(fi, buf, BUFSIZ)) > 0)
			if (write(ofd, buf, n) != n) {
				(void) close(fi);
				return (REPRINT);
			}
		(void) close(fi);
		return (OK);
	}
	switch (format) {
	case 'p':	/* print file using 'pr' */
		if (IF == NULL) {	/* use output filter */
			prog = PR;
			av[0] = "pr";
			av[1] = width;
			av[2] = length;
			av[3] = "-h";
			av[4] = *title ? title : " ";
			av[5] = 0;
			fo = ofd;
			goto start;
		}
		if (pipe(p) == -1) {
			syslog(LOG_ERR, "%s: can't create pipe: %m", printer);
			return (ERROR);
		}
		if ((prchild = dofork(DORETURN)) == 0) {	/* child */
			(void) dup2(fi, 0);	/* file is stdin */
			(void) dup2(p[1], 1);	/* pipe is stdout */
			for (n = 3; n < NOFILE; n++)
				(void) close(n);
			execl(PR, "pr", width, length, "-h",
					*title ? title : " ", (char *)0);
			syslog(LOG_ERR, "print: can't execl %s: %m", PR);
			exit(2);
		}
		(void) close(p[1]);		/* close output side */
		(void) close(fi);
		if (prchild < 0) {
			prchild = 0;
			(void) close(p[0]);
			return (ERROR);
		}
		fi = p[0];			/* use pipe for input */
	case 'f':	/* print plain text file */
		prog = IF;
		av[1] = width;
		av[2] = length;
		av[3] = indent;
		n = 4;
		break;
	case 'l':	/* like 'f' but pass control characters */
		prog = IF;
		av[1] = "-c";
		av[2] = width;
		av[3] = length;
		av[4] = indent;
		n = 5;
		break;
	case 'r':	/* print a fortran text file */
		prog = RF;
		av[1] = width;
		av[2] = length;
		n = 3;
		break;
	case 't':	/* print troff output */
	case 'n':	/* print ditroff output */
	case 'd':	/* print tex output */
		(void) unlink(".railmag");
		if ((fo = creat(".railmag", FILMOD)) < 0) {
			syslog(LOG_ERR, "%s: cannot create .railmag: %m", printer);
			(void) unlink(".railmag");
		} else {
			for (n = 0; n < 4; n++) {
				if (fonts[n][0] != '/')
					(void) write(fo, "/usr/lib/vfont/", 15);
				(void) write(fo, fonts[n], strlen(fonts[n]));
				(void) write(fo, "\n", 1);
			}
			(void) close(fo);
		}
		prog = (format == 't') ? TF : (format == 'n') ? NF : DF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'c':	/* print cifplot output */
		prog = CF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'g':	/* print plot(1G) output */
		prog = GF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	case 'v':	/* print raster output */
		prog = VF;
		av[1] = pxwidth;
		av[2] = pxlength;
		n = 3;
		break;
	default:
		(void) close(fi);
		syslog(LOG_ERR, "%s: illegal format character '%c'",
			printer, format);
		return (ERROR);
	}
	if (prog == NULL) {
		(void) close(fi);
		syslog(LOG_ERR, "%s: no filter specified for format '%c'",
			printer, format);
		return (ERROR);
	}
	if ((av[0] = rindex(prog, '/')) != NULL)
		av[0]++;
	else
		av[0] = prog;
	av[n++] = "-n";
	av[n++] = logname;
	av[n++] = "-h";
	av[n++] = fromhost;
	av[n++] = AF;
	av[n] = 0;
	fo = pfd;
	if (ofilter > 0) {		/* stop output filter */
		(void) write(ofd, "\031\1", 2);
		do
			pid = wait3(&status, WUNTRACED, (struct rusage *)0);
		while (pid > 0 && pid != ofilter);
		if (status.w_stopval != WSTOPPED) {
			(void) close(fi);
			syslog(LOG_WARNING, "%s: output filter died (%d)",
				printer, status.w_retcode);
			return (REPRINT);
		}
		stopped++;
	}
start:
	if ((child = dofork(DORETURN)) == 0) {	/* child */
		(void) dup2(fi, 0);
		(void) dup2(fo, 1);
		n = open(errfile, O_WRONLY|O_CREAT|O_TRUNC, 0664);
		if (n >= 0)
			(void) dup2(n, 2);
		for (n = 3; n < NOFILE; n++)
			(void) close(n);
		execv(prog, av);
		syslog(LOG_ERR, "%s: can't execv %s: %m", printer, prog);
		exit(2);
	}
	(void) close(fi);
	if (child < 0)
		status.w_retcode = 100;
	else
		while ((pid = wait(&status)) > 0 && pid != child)
			;
	child = 0;
	prchild = 0;
	if (stopped) {		/* restart output filter */
		if (kill(ofilter, SIGCONT) < 0) {
			syslog(LOG_ERR, "%s: cannot restart output filter", printer);
			exit(1);
		}
	}
	tof = 0;
	if (!WIFEXITED(status)) {
		syslog(LOG_WARNING, "%s: Daemon filter '%c' terminated (%d)",
			printer, format, status.w_termsig);
		return (ERROR);
	}
	switch (status.w_retcode) {
	case 0:
		tof = 1;
		return (OK);
	case 1:
		return (REPRINT);
	case 2:
		return (ERROR);
	default:
		syslog(LOG_WARNING, "%s: Daemon filter '%c' exited (%d)",
			printer, format, status.w_retcode);
		return (ERROR);
	}
}

/*
 * Send the daemon control file (cf) and any data files.
 * Return -1 if a non-recoverable error occured, 1 if a recoverable error and
 * 0 if all is well.
 */
static int
sendit(file)
	char *file;
{
	register int i, err = OK;
	char last[BUFSIZ];

	/*
	 * open control file
	 */
	if ((cfp = fopen(file, "r")) == NULL)
		return (OK);
	/*
	 *	read the control file for work to do
	 *
	 *	file format -- first character in the line is a command
	 *	rest of the line is the argument.
	 *	commands of interest are:
	 *
	 *		a-z -- "file name" name of file to print
	 *		U -- "unlink" name of file to remove
	 *			(after we print it. (Pass 2 only)).
	 */

	/*
	 * pass 1
	 */
	while (getline(cfp)) {
	again:
		if (line[0] == 'S') {
			sscanf(line + 1, "%hu%lu", &fdev, &fino);
			continue;
		} else if (line[0] == 'P') {
			strncpy(logname, line + 1, (sizeof logname) - 1);
			continue;
		} else if (line[0] == 'H') {
			strncpy(fromhost, line + 1, (sizeof fromhost) - 1);
			continue;
		} else if (line[0] >= 'a' && line[0] <= 'z') {
			strcpy(last, line);
			while (i = getline(cfp))
				if (strcmp(last, line))
					break;
			switch (sendfile('\3', last + 1)) {
			case OK:
				if (i)
					goto again;
				break;
			case REPRINT:
				(void) fclose(cfp);
				return (REPRINT);
			case ACCESS:
				sendmail(logname, ACCESS);
			case ERROR:
				err = ERROR;
			}
			break;
		}
	}
	if (err == OK && sendfile('\2', file) > 0) {
		(void) fclose(cfp);
		return (REPRINT);
	}
	/*
	 * pass 2
	 */
	(void) fseek(cfp, 0L, 0);
	while (getline(cfp))
		if (line[0] == 'U')
			(void) unlink(line+1);
	/*
	 * clean-up in case another control file exists
	 */
	(void) fclose(cfp);
	(void) unlink(file);
	return (err);
}

/*
 * Send a data file to the remote machine and spool it.
 * Return positive if we should try resending.
 */
static int
sendfile(type, file)
	char type, *file;
{
	register int f, i, amt;
	struct stat stb;
	char buf[BUFSIZ];
	int sizerr, resp;

	if (!safepath(file) || lstat(file, &stb) < 0 ||
			(f = open(file, O_RDONLY)) < 0)
		return (ERROR);
	/*
	 * Check to see if data file is a symbolic link. If so, it should
	 * still point to the same file or someone is trying to print something
	 * he shouldn't.
	 */
	if ((stb.st_mode & S_IFMT) == S_IFLNK && fstat(f, &stb) == 0 &&
	    (stb.st_dev != fdev || stb.st_ino != fino))
		return (ACCESS);
	(void) sprintf(buf, "%c%d %s\n", type, stb.st_size, file);
	amt = strlen(buf);
	for (i = 0;  ; i++) {
		if (write(pfd, buf, amt) != amt ||
		    (resp = response()) < 0 || resp == '\1') {
			(void) close(f);
			return (REPRINT);
		} else if (resp == '\0')
			break;
		if (i == 0)
			status("no space on remote; waiting for queue to drain");
		if (i == 10)
			syslog(LOG_ALERT, "%s: can't send to %s; queue full",
				printer, RM);
		sleep(5 * 60);
	}
	if (i)
		status("sending to %s", RM);
	sizerr = 0;
	for (i = 0; i < stb.st_size; i += BUFSIZ) {
		amt = BUFSIZ;
		if (i + amt > stb.st_size)
			amt = stb.st_size - i;
		if (sizerr == 0 && read(f, buf, amt) != amt)
			sizerr = 1;
		if (write(pfd, buf, amt) != amt) {
			(void)close(f);
			return (REPRINT);
		}
	}
	(void) close(f);
	if (sizerr) {
		syslog(LOG_INFO, "%s: %s: changed size", printer, file);
		/* tell recvjob to ignore this file */
		(void)write(pfd, "\1", 1);
		return (ERROR);
	}
	if (write(pfd, "", 1) != 1 || response())
		return (REPRINT);
	return (OK);
}

/*
 * Check to make sure there have been no errors and that both programs
 * are in sync with eachother.
 * Return non-zero if the connection was lost.
 */
static int
response()
{
	char resp;

	if (read(pfd, &resp, 1) != 1) {
		syslog(LOG_INFO, "%s: lost connection", printer);
		return (-1);
	}
	return (resp);
}

/*
 * Banner printing stuff
 */
static
banner(name1, name2)
	char *name1, *name2;
{
	time_t tvec;
	extern time_t time();
	extern char *ctime();

#define	writestr(fd, s)	write(fd, s, sizeof (s) - 1)

	(void) time(&tvec);
	if (!SF && !tof)
		(void) write(ofd, FF, strlen(FF));
	if (SB) {	/* short banner only */
		if (class[0]) {
			(void) write(ofd, class, strlen(class));
			(void) writestr(ofd, ":");
		}
		(void) write(ofd, name1, strlen(name1));
		(void) writestr(ofd, "  Job: ");
		(void) write(ofd, name2, strlen(name2));
		(void) writestr(ofd, "  Date: ");
		(void) write(ofd, ctime(&tvec), 24);
		(void) writestr(ofd, "\n");
	} else {	/* normal banner */
		(void) writestr(ofd, "\n\n\n");
		scan_out(ofd, name1, '\0');
		(void) writestr(ofd, "\n\n");
		scan_out(ofd, name2, '\0');
		if (class[0]) {
			(void) writestr(ofd, "\n\n\n");
			scan_out(ofd, class, '\0');
		}
		(void) writestr(ofd, "\n\n\n\n\t\t\t\t\tJob:  ");
		(void) write(ofd, name2, strlen(name2));
		(void) writestr(ofd, "\n\t\t\t\t\tDate: ");
		(void) write(ofd, ctime(&tvec), 24);
		(void) writestr(ofd, "\n");
	}
	if (!SF)
		(void) write(ofd, FF, strlen(FF));
	tof = 1;
#undef writestr
}

static char *
scnline(key, p, c)
	register char key, *p;
	char c;
{
	register scnwidth;

	for (scnwidth = WIDTH; --scnwidth; ) {
		key <<= 1;
		*p++ = key & 0200 ? c : BACKGND;
	}
	return (p);
}

static
scan_out(scfd, scsp, dlm)
	int scfd;
	char *scsp, dlm;
{
	register char *strp, *sp, c;
	register int nchrs, j, d, scnhgt;
	u_int outwidth;
	char *outbuf, *string_end;
	extern char scnkey[][HEIGHT];	/* in lpdchar.c */
	extern char map_to_ascii[];

	string_end = index(scsp, dlm);
	if (string_end == NULL)
		string_end = scsp + strlen(scsp);

	nchrs = MIN(string_end - scsp, (PW + 1)/(WIDTH + 2));
	outwidth = MAX(nchrs * (WIDTH + 2), 1);

	outbuf = malloc(outwidth);
	if (outbuf == NULL) {
		syslog(LOG_ERR, "scan_out: %m");
		return;
	}

	for (scnhgt = 0; scnhgt++ < HEIGHT+DROP; ) {
		for (sp = scsp, strp = outbuf; sp != scsp + nchrs; sp++) {
			if (sp != scsp) {
				*strp++ = BACKGND;
				*strp++ = BACKGND;
			}
			c = map_to_ascii[(u_char)*sp];
			d = dropit(c);
			if ((!d && scnhgt > HEIGHT) || (scnhgt <= DROP && d))
				for (j = WIDTH; --j; )
					*strp++ = BACKGND;
			else
				strp = scnline(
				    scnkey[c - ' '][scnhgt - 1 - d], strp, c);
		}
		while (--strp >= outbuf && *strp == BACKGND)
			;
		strp++;
		*strp++ = '\n';
		(void) write(scfd, outbuf, strp - outbuf);
	}
	free(outbuf);
}

static
dropit(c)
	char c;
{
	switch (c) {
	case '_':
	case ';':
	case ',':
	case 'g':
	case 'j':
	case 'p':
	case 'q':
	case 'y':
		return (DROP);

	default:
		return (0);
	}
}

/*
 * sendmail ---
 *   tell people about job completion
 */
static
sendmail(user, job_status)
	char *user;
	int job_status;
{
	register int i, s;
	int p[2];
	register char *cp;
	char *buf;

	if (pipe(p) == -1) {
		syslog(LOG_ERR, "%s: can't create pipe: %m", printer);
		return;
	}

	if ((s = dofork(DORETURN)) == 0) {		/* child */
		(void) dup2(p[0], 0);
		for (i = 3; i < NOFILE; i++)
			(void) close(i);
		if ((cp = rindex(MAIL, '/')) != NULL)
			cp++;
		else
			cp = MAIL;
		buf = malloc((u_int)(strlen(user) + strlen(fromhost) + 2));
		if (buf == NULL) {
			syslog(LOG_WARNING, "%s: sendmail: %m", printer);
			exit(1);
		}
		(void) sprintf(buf, "%s@%s", user, fromhost);
		execl(MAIL, cp, buf, (char *)0);
		syslog(LOG_WARNING, "%s: can't execl %s: %m", printer, MAIL);
		exit(1);
	} else if (s > 0) {				/* parent */
		(void) dup2(p[1], 1);
		printf("To: %s@%s\n", user, fromhost);
		printf("Subject: printer job\n\n");
		printf("Your printer job ");
		if (*jobname)
			printf("(%s) ", jobname);
		switch (job_status) {
		case OK:
			printf("\ncompleted successfully\n");
			break;
		case FATALERR:
			printf("\ncould not be printed\n");
			break;
		case NOACCT:
			printf("\ncould not be printed without an account on %s\n", host);
			break;
		case ACCESS:
			printf("\nwas not printed because it was not linked to the original file\n");
		default:
			syslog(LOG_WARNING, "sendmail: user %s, status %d",
				user, job_status);
			printf("\nfinished with status %d\n", job_status);
			break;
		}
		print_filter_messages(stdout,
			"\nMessages from output filter:\n");
		fflush(stdout);
		(void) close(1);
	}
	(void) close(p[0]);
	(void) close(p[1]);
	(void) wait((union wait *) 0);
}

static void
print_filter_messages(out, header)
	FILE *out;
	char *header;
{
	struct stat stb;
	register FILE *fp;
	register int c;

	if (stat(errfile, &stb) < 0 || stb.st_size == 0 ||
	    (fp = fopen(errfile, "r")) == NULL)
		return;

	if (header != NULL)
		fputs(header, out);
	while ((c = getc(fp)) != EOF)
		putc(c, out);
	(void) fclose(fp);
}

/*
 * dofork - fork with retries on failure
 */
static
dofork(action)
	int action;
{
	register int i, pid;

	for (i = 0; i < 20; i++) {
		if ((pid = fork()) < 0) {
			sleep((unsigned)(i * i));
			continue;
		}
		/*
		 * Child should run as daemon instead of root
		 */
		if (pid == 0)
			setuid(DU);
		return (pid);
	}
	syslog(LOG_ERR, "can't fork");

	switch (action) {
	case DORETURN:
		return (-1);
	default:
		syslog(LOG_ERR, "bad action (%d) to dofork", action);
		/* FALL THRU */
	case DOABORT:
		exit(1);
	}
	/*NOTREACHED*/
}

/*
 * Kill child processes to abort current job.
 */
static void
abortpr()
{
	(void) unlink(errfile);
	(void) kill(0, SIGINT);
	if (ofilter > 0)
		(void) kill(ofilter, SIGCONT);
	while (wait((union wait *)0) > 0)
		;
	exit(0);
}

static
init()
{
	int status;

	if ((status = pgetent(line, printer)) < 0) {
		syslog(LOG_ERR, "can't open printer description file");
		exit(1);
	} else if (status == 0) {
		syslog(LOG_ERR, "unknown printer: %s", printer);
		exit(1);
	}
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((DU = pgetnum("du")) < 0)
		DU = DEFUID;
	if ((FF = pgetstr("ff", &bp)) == NULL)
		FF = DEFFF;
	if ((PW = pgetnum("pw")) < 0)
		PW = DEFWIDTH;
	(void) sprintf(&width[2], "%d", PW);
	if ((PL = pgetnum("pl")) < 0)
		PL = DEFLENGTH;
	(void) sprintf(&length[2], "%d", PL);
	if ((PX = pgetnum("px")) < 0)
		PX = 0;
	(void) sprintf(&pxwidth[2], "%d", PX);
	if ((PY = pgetnum("py")) < 0)
		PY = 0;
	(void) sprintf(&pxlength[2], "%d", PY);
	RM = pgetstr("rm", &bp);
	if (RM != NULL && *RM == '\0')
		RM = NULL;
	/*
	 * Figure out whether the local machine is the same as the remote
	 * machine entry (if it exists).  If not, then ignore the local
	 * queue information.
	 */
	if (RM != NULL) {
		char name[MAXHOSTNAMELEN + 1];
		struct hostent *hp;

		/* get the standard network name of the local host */
		gethostname(name, sizeof name);
		name[(sizeof name) - 1] = '\0';
		hp = gethostbyname(name);
		if (hp == (struct hostent *)NULL) {
			syslog(LOG_ERR,
			    "unable to get network name for local machine %s",
			    name);
			goto localcheck_done;
		}
		strcpy(name, hp->h_name);

		/* get the standard network name of RM */
		hp = gethostbyname(RM);
		if (hp == (struct hostent *)NULL) {
			syslog(LOG_ERR,
			    "unable to get hostname for remote machine %s",
			    RM);
			goto localcheck_done;
		}

		/* if printer is not on local machine, ignore LP */
		if (strcmp(name, hp->h_name) != 0)
			*LP = '\0';
	}
localcheck_done:

	AF = pgetstr("af", &bp);
	OF = pgetstr("of", &bp);
	IF = pgetstr("if", &bp);
	RF = pgetstr("rf", &bp);
	TF = pgetstr("tf", &bp);
	NF = pgetstr("nf", &bp);
	DF = pgetstr("df", &bp);
	GF = pgetstr("gf", &bp);
	VF = pgetstr("vf", &bp);
	CF = pgetstr("cf", &bp);
	TR = pgetstr("tr", &bp);
	RS = pgetflag("rs");
	SF = pgetflag("sf");
	SH = pgetflag("sh");
	SB = pgetflag("sb");
	HL = pgetflag("hl");
	RW = pgetflag("rw");
	BR = pgetnum("br");
	if ((FC = pgetnum("fc")) < 0)
		FC = 0;
	if ((FS = pgetnum("fs")) < 0)
		FS = 0;
	if ((XC = pgetnum("xc")) < 0)
		XC = 0;
	if ((XS = pgetnum("xs")) < 0)
		XS = 0;
	if ((MS = pgetstr("ms", &bp)) != NULL)
		parse_modes(MS);
	tof = !pgetflag("fo");
}

struct mds {
	char	*string;
	unsigned long	set;
	unsigned long	reset;
};
						/* Control Modes */
struct mds cmodes[] = {
	"-parity", CS8, PARENB|CSIZE,
	"-evenp", CS8, PARENB|CSIZE,
	"-oddp", CS8, PARENB|PARODD|CSIZE,
	"parity", PARENB|CS7, PARODD|CSIZE,
	"evenp", PARENB|CS7, PARODD|CSIZE,
	"oddp", PARENB|PARODD|CS7, CSIZE,
	"parenb", PARENB, 0,
	"-parenb", 0, PARENB,
	"parodd", PARODD, 0,
	"-parodd", 0, PARODD,
	"cs8", CS8, CSIZE,
	"cs7", CS7, CSIZE,
	"cs6", CS6, CSIZE,
	"cs5", CS5, CSIZE,
	"cstopb", CSTOPB, 0,
	"-cstopb", 0, CSTOPB,
	"stopb", CSTOPB, 0,
	"-stopb", 0, CSTOPB,
	"hupcl", HUPCL, 0,
	"hup", HUPCL, 0,
	"-hupcl", 0, HUPCL,
	"-hup", 0, HUPCL,
	"clocal", CLOCAL, 0,
	"-clocal", 0, CLOCAL,
	"nohang", CLOCAL, 0,
	"-nohang", 0, CLOCAL,
#if 0		/* this bit isn't supported */
	"loblk", LOBLK, 0,
	"-loblk", 0, LOBLK,
#endif
	"cread", CREAD, 0,
	"-cread", 0, CREAD,
	"crtscts", CRTSCTS, 0,
	"-crtscts", 0, CRTSCTS,
	"litout", CS8, (CSIZE|PARENB),
	"-litout", (CS7|PARENB), CSIZE,
	"pass8", CS8, (CSIZE|PARENB),
	"-pass8", (CS7|PARENB), CSIZE,
	"raw", CS8, (CSIZE|PARENB),
	"-raw", (CS7|PARENB), CSIZE,
	"cooked", (CS7|PARENB), CSIZE,
	"sane", (CS7|PARENB|CREAD), (CSIZE|PARODD|CLOCAL),
	0
};
						/* Input Modes */
struct mds imodes[] = {
	"ignbrk", IGNBRK, 0,
	"-ignbrk", 0, IGNBRK,
	"brkint", BRKINT, 0,
	"-brkint", 0, BRKINT,
	"ignpar", IGNPAR, 0,
	"-ignpar", 0, IGNPAR,
	"parmrk", PARMRK, 0,
	"-parmrk", 0, PARMRK,
	"inpck", INPCK, 0,
	"-inpck", 0, INPCK,
	"istrip", ISTRIP, 0,
	"-istrip", 0, ISTRIP,
	"inlcr", INLCR, 0,
	"-inlcr", 0, INLCR,
	"igncr", IGNCR, 0,
	"-igncr", 0, IGNCR,
	"icrnl", ICRNL, 0,
	"-icrnl", 0, ICRNL,
	"-nl", ICRNL, (INLCR|IGNCR),
	"nl", 0, ICRNL,
	"iuclc", IUCLC, 0,
	"-iuclc", 0, IUCLC,
	"lcase", IUCLC, 0,
	"-lcase", 0, IUCLC,
	"LCASE", IUCLC, 0,
	"-LCASE", 0, IUCLC,
	"ixon", IXON, 0,
	"-ixon", 0, IXON,
	"ixany", IXANY, 0,
	"-ixany", 0, IXANY,
	"decctlq", 0, IXANY,
	"-decctlq", IXANY, 0,
	"ixoff", IXOFF, 0,
	"-ixoff", 0, IXOFF,
	"tandem", IXOFF, 0,
	"-tandem", 0, IXOFF,
	"imaxbel", IMAXBEL, 0,
	"-imaxbel", 0, IMAXBEL,
	"pass8", 0, ISTRIP,
	"-pass8", ISTRIP, 0,
	"raw", 0, -1,
	"-raw", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IMAXBEL), 0,
	"cooked", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON), 0,
	"sane", (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IMAXBEL),
		(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF),
	0
};
						/* Local Modes */
struct mds lmodes[] = {
	"isig", ISIG, 0,
	"-isig", 0, ISIG,
	"noisig", 0, ISIG,
	"-noisig", ISIG, 0,
	"iexten", IEXTEN, 0,
	"-iexten", 0, IEXTEN,
	"icanon", ICANON, 0,
	"-icanon", 0, ICANON,
	"cbreak", 0, ICANON,
	"-cbreak", ICANON, 0,
	"xcase", XCASE, 0,
	"-xcase", 0, XCASE,
	"lcase", XCASE, 0,
	"-lcase", 0, XCASE,
	"LCASE", XCASE, 0,
	"-LCASE", 0, XCASE,
	"echo", ECHO, 0,
	"-echo", 0, ECHO,
	"echoe", ECHOE, 0,
	"-echoe", 0, ECHOE,
	"crterase", ECHOE, 0,
	"-crterase", 0, ECHOE,
	"echok", ECHOK, 0,
	"-echok", 0, ECHOK,
	"lfkc", ECHOK, 0,
	"-lfkc", 0, ECHOK,
	"echonl", ECHONL, 0,
	"-echonl", 0, ECHONL,
	"noflsh", NOFLSH, 0,
	"-noflsh", 0, NOFLSH,
	"tostop", TOSTOP, 0,
	"-tostop", 0, TOSTOP,
	"echoctl", ECHOCTL, 0,
	"-echoctl", 0, ECHOCTL,
	"ctlecho", ECHOCTL, 0,
	"-ctlecho", 0, ECHOCTL,
	"echoprt", ECHOPRT, 0,
	"-echoprt", 0, ECHOPRT,
	"prterase", ECHOPRT, 0,
	"-prterase", 0, ECHOPRT,
	"echoke", ECHOKE, 0,
	"-echoke", 0, ECHOKE,
	"crtkill", ECHOKE, 0,
	"-crtkill", 0, ECHOKE,
#if 0		/* this bit isn't supported yet */
	"defecho", DEFECHO, 0,
	"-defecho", 0, DEFECHO,
#endif
	"raw", 0, (ISIG|ICANON|XCASE|IEXTEN),
	"-raw", (ISIG|ICANON|IEXTEN), 0,
 	"cooked", (ISIG|ICANON), 0,
	"sane", (ISIG|ICANON|ECHO|ECHOE|ECHOK|ECHOCTL|ECHOKE),
		(XCASE|ECHOPRT|ECHONL|NOFLSH),
	0,
};
						/* Output Modes */
struct mds omodes[] = {
	"opost", OPOST, 0,
	"-opost", 0, OPOST,
	"nopost", 0, OPOST,
	"-nopost", OPOST, 0,
	"olcuc", OLCUC, 0,
	"-olcuc", 0, OLCUC,
	"lcase", OLCUC, 0,
	"-lcase", 0, OLCUC,
	"LCASE", OLCUC, 0,
	"-LCASE", 0, OLCUC,
	"onlcr", ONLCR, 0,
	"-onlcr", 0, ONLCR,
	"-nl", ONLCR, (OCRNL|ONLRET),
	"nl", 0, ONLCR,
	"ocrnl", OCRNL, 0,
	"-ocrnl", 0, OCRNL,
	"onocr", ONOCR, 0,
	"-onocr", 0, ONOCR,
	"onlret", ONLRET, 0,
	"-onlret", 0, ONLRET,
	"fill", OFILL, OFDEL,
	"-fill", 0, OFILL|OFDEL,
	"nul-fill", OFILL, OFDEL,
	"del-fill", OFILL|OFDEL, 0,
	"ofill", OFILL, 0,
	"-ofill", 0, OFILL,
	"ofdel", OFDEL, 0,
	"-ofdel", 0, OFDEL,
	"cr0", CR0, CRDLY,
	"cr1", CR1, CRDLY,
	"cr2", CR2, CRDLY,
	"cr3", CR3, CRDLY,
	"tab0", TAB0, TABDLY,
	"tabs", TAB0, TABDLY,
	"tab1", TAB1, TABDLY,
	"tab2", TAB2, TABDLY,
	"-tabs", XTABS, TABDLY,
	"tab3", XTABS, TABDLY,
	"nl0", NL0, NLDLY,
	"nl1", NL1, NLDLY,
	"ff0", FF0, FFDLY,
	"ff1", FF1, FFDLY,
	"vt0", VT0, VTDLY,
	"vt1", VT1, VTDLY,
	"bs0", BS0, BSDLY,
	"bs1", BS1, BSDLY,
#if 0		/* these bits aren't supported yet */
	"pageout", PAGEOUT, 0,
	"-pageout", 0, PAGEOUT,
	"wrap", WRAP, 0,
	"-wrap", 0, WRAP,
#endif
	"litout", 0, OPOST,
	"-litout", OPOST, 0,
	"raw", 0, OPOST,
	"-raw", OPOST, 0,
	"cooked", OPOST, 0,
	"33", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tty33", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tn", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tn300", CR1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"ti", CR2, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"ti700", CR2, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"05", NL1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"vt05", NL1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"tek", FF1, (CRDLY|TABDLY|NLDLY|FFDLY|VTDLY|BSDLY),
	"37", (FF1|VT1|CR2|TAB1|NL1), (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"tty37", (FF1|VT1|CR2|TAB1|NL1), (NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	"sane", (OPOST|ONLCR), (OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
			NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY),
	0,
};

/*
 * Parse a set of modes.
 */
static
parse_modes(modes)
	char *modes;
{
	register char *curtoken;
	register int match;
	register int i;

	termios_clear.c_iflag = 0;
	termios_clear.c_oflag = 0;
	termios_clear.c_cflag = 0;
	termios_clear.c_lflag = 0;
	termios_set.c_iflag = 0;
	termios_set.c_oflag = 0;
	termios_set.c_cflag = 0;
	termios_set.c_lflag = 0;

	curtoken = strtok(modes, ",");
	while (curtoken != NULL) {
		match = 0;
		for (i = 0; imodes[i].string != NULL; i++) {
			if (strcmp(curtoken, imodes[i].string) == 0) {
				match++;
				termios_clear.c_iflag |= imodes[i].reset;
				termios_set.c_iflag |= imodes[i].set;
			}
		}
		for (i = 0; omodes[i].string != NULL; i++) {
			if (strcmp(curtoken, omodes[i].string) == 0) {
				match++;
				termios_clear.c_oflag |= omodes[i].reset;
				termios_set.c_oflag |= omodes[i].set;
			}
		}
		for (i = 0; cmodes[i].string != NULL; i++) {
			if (strcmp(curtoken, cmodes[i].string) == 0) {
				match++;
				termios_clear.c_cflag |= cmodes[i].reset;
				termios_set.c_cflag |= cmodes[i].set;
			}
		}
		for (i = 0; lmodes[i].string != NULL; i++) {
			if (strcmp(curtoken, lmodes[i].string) == 0) {
				match++;
				termios_clear.c_lflag |= lmodes[i].reset;
				termios_set.c_lflag |= lmodes[i].set;
			}
		}
		if (!match) {
			syslog(LOG_ERR, "%s: unknown mode %s",
			    printer, curtoken);
			exit(1);
		}
		curtoken = strtok((char *)NULL, ",");
	}
}

/*
 * Acquire line printer or remote connection.
 */
static
openpr()
{
	register int i, n;
	int resp;

	if (*LP) {
		for (i = 1; ; i = MAX(2 * i, 32)) {
			pfd = open(LP, RW ? O_RDWR : O_WRONLY);
			if (pfd >= 0)
				break;
			if (errno == ENOENT) {
				syslog(LOG_ERR, "%s: %m", LP);
				exit(1);
			}
			if (i == 1)
				status("waiting for %s to become ready (offline ?)", printer);
			sleep((unsigned)i);
		}
		/*
		 * Use file locking in addition to tty exclusive use locking
		 * to prevent races between open above and TIOCEXCL below.
		 */
		if ((i = flock(pfd, LOCK_EX | LOCK_NB)) < 0) {
			if (errno == EWOULDBLOCK) {
				status("waiting for lock on %s", LP);
				i = flock(pfd, LOCK_EX);
				/* XXX - should redo open here? */
			}
			if (i < 0) {
				syslog(LOG_ERR, "%s: broken lock: %m", LP);
				exit(1);
			}
		}
		if (isatty(pfd))
			setty();
		status("%s is ready and printing", printer);
	} else if (RM != NULL) {
		for (i = 1; ; i = MAX(2 * i, 256)) {
			resp = -1;
			pfd = getport(RM);
			if (pfd >= 0) {
				(void) sprintf(line, "\2%s\n", RP);
				n = strlen(line);
				if (write(pfd, line, n) == n &&
				    (resp = response()) == '\0')
					break;
				(void) close(pfd);
			}
			if (i == 1) {
				if (resp < 0)
					status("waiting for %s to come up", RM);
				else {
					status("waiting for queue to be enabled on %s", RM);
					i = 256;
				}
			}
			sleep((unsigned)i);
		}
		status("sending to %s", RM);
		remote = 1;
	} else {
		syslog(LOG_ERR, "%s: no line printer device or host name",
			printer);
		exit(1);
	}
	/*
	 * Start up an output filter, if needed.
	 */
	if (OF && *LP) {
		int p[2];
		char *cp;

		if (pipe(p) == -1) {
			syslog(LOG_ERR, "%s: can't create pipe: %m", printer);
			return;
		}
		if ((ofilter = dofork(DOABORT)) == 0) {	/* child */
			(void) dup2(p[0], 0);	/* pipe is std in */
			(void) dup2(pfd, 1);		/* printer is std out */
			for (i = 3; i < NOFILE; i++)
				(void) close(i);
			if ((cp = rindex(OF, '/')) == NULL)
				cp = OF;
			else
				cp++;
			execl(OF, cp, width, length, (char *)0);
			syslog(LOG_ERR, "%s: %s: %m", printer, OF);
			exit(1);
		}
		(void) close(p[0]);		/* close input side */
		ofd = p[1];			/* use pipe for output */
	} else {
		ofd = pfd;
		ofilter = 0;
	}
}

struct bauds {
	int	baud;
	int	speed;
} bauds[] = {
	50,	B50,
	75,	B75,
	110,	B110,
	134,	B134,
	150,	B150,
	200,	B200,
	300,	B300,
	600,	B600,
	1200,	B1200,
	1800,	B1800,
	2400,	B2400,
	4800,	B4800,
	9600,	B9600,
	19200,	EXTA,
	38400,	EXTB,
	0,	0
};

/*
 * setup tty lines.
 */
static
setty()
{
	struct sgttyb ttybuf;
	register struct bauds *bp;
	struct termios termios;

	if (ioctl(pfd, TIOCEXCL, (char *)0) < 0) {
		syslog(LOG_ERR, "%s: ioctl(TIOCEXCL): %m", printer);
		exit(1);
	}
	if (ioctl(pfd, TIOCGETP, (char *)&ttybuf) < 0) {
		syslog(LOG_ERR, "%s: ioctl(TIOCGETP): %m", printer);
		exit(1);
	}
	if (BR > 0) {
		for (bp = bauds; bp->baud; bp++)
			if (BR == bp->baud)
				break;
		if (!bp->baud) {
			syslog(LOG_ERR, "%s: illegal baud rate %d", printer, BR);
			exit(1);
		}
		ttybuf.sg_ispeed = ttybuf.sg_ospeed = bp->speed;
	}
	ttybuf.sg_flags &= ~FC;
	ttybuf.sg_flags |= FS;
	if (ioctl(pfd, TIOCSETP, (char *)&ttybuf) < 0) {
		syslog(LOG_ERR, "%s: ioctl(TIOCSETP): %m", printer);
		exit(1);
	}
	if (XC || XS) {
		int ldisc = NTTYDISC;

		if (ioctl(pfd, TIOCSETD, &ldisc) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TIOCSETD): %m", printer);
			exit(1);
		}
	}
	if (XC) {
		if (ioctl(pfd, TIOCLBIC, &XC) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TIOCLBIC): %m", printer);
			exit(1);
		}
	}
	if (XS) {
		if (ioctl(pfd, TIOCLBIS, &XS) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TIOCLBIS): %m", printer);
			exit(1);
		}
	}
	if (MS) {
		if (ioctl(pfd, TCGETS, &termios) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TCGETS): %m", printer);
			exit(1);
		}
		termios.c_iflag &= ~termios_clear.c_iflag;
		termios.c_iflag |= termios_set.c_iflag;
		termios.c_oflag &= ~termios_clear.c_oflag;
		termios.c_oflag |= termios_set.c_oflag;
		termios.c_cflag &= ~termios_clear.c_cflag;
		termios.c_cflag |= termios_set.c_cflag;
		termios.c_lflag &= ~termios_clear.c_lflag;
		termios.c_lflag |= termios_set.c_lflag;
		if (ioctl(pfd, TCSETSF, &termios) < 0) {
			syslog(LOG_ERR, "%s: ioctl(TCSETSF): %m", printer);
			exit(1);
		}
	}
}

/*VARARGS1*/
static
status(msg, a1, a2, a3)
	char *msg;
{
	register int fd;
	char buf[BUFSIZ];

	(void) umask(0);
	fd = open(ST, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		syslog(LOG_ERR, "%s: %s: %m", printer, ST);
		exit(1);
	}
	(void) ftruncate(fd, (off_t)0);
	(void) sprintf(buf, msg, a1, a2, a3);
	(void) strcat(buf, "\n");
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);
}
