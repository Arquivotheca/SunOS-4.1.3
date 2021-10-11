/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)recvjob.c 1.1 92/07/30 SMI"; /* from UCB 5.4 6/6/86 */
#endif not lint

/*
 * Receive printer jobs from the network, queue them and
 * start the printer daemon.
 */

#include "lp.h"
#include <sys/vfs.h>

char	*sp = "";
#define ack()	(void) write(1, sp, 1);

static char    tfname[MAXFILENAMELEN];	/* tmp copy of cf before linking */
static char    dfname[MAXFILENAMELEN];	/* data files */
static int	minfree;		/* keep at least minfree blocks available */
static int	dfd;			/* file system device descriptor */

recvjob()
{
	struct stat stb;
	char *bp = pbuf;
	int status;
	void rcleanup();

	/*
	 * Perform lookup for printer name or abbreviation
	 */
	if ((status = pgetent(line, printer)) < 0)
		frecverr("cannot open printer description file");
	else if (status == 0)
		frecverr("unknown printer %s", printer);
	if ((LF = pgetstr("lf", &bp)) == NULL)
		LF = DEFLOGF;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;

	(void) close(2);			/* set up log file */
	if (open(LF, O_WRONLY|O_APPEND, 0664) < 0) {
		syslog(LOG_ERR, "%s: %m", LF);
		(void) open("/dev/null", O_WRONLY);
	}

	if (chdir(SD) < 0)
		frecverr("%s: %s: %m", printer, SD);
	if (stat(LO, &stb) == 0) {
		if (stb.st_mode & 010) {
			/* queue is disabled */
			putchar('\1');		/* return error code */
			exit(1);
		}
	}

	/*
	 * Open the spooling directory, so that we have a descriptor on which
	 * to do an "fstatfs" to find out how much space is left on the file
	 * system containing that directory.
	 */
	if ((dfd = open(".", O_RDONLY)) < 0)
		frecverr("%s: %s: %m", printer, SD);
	minfree = read_number("minfree");
	signal(SIGTERM, rcleanup);
	signal(SIGPIPE, rcleanup);

	if (readjob())
		printjob();
}

/*
 * Read printer jobs sent by lpd and copy them to the spooling directory.
 * Return the number of jobs successfully transfered.
 */
static
readjob()
{
	register int size, nfiles;
	register char *cp;

	ack();
	nfiles = 0;
	for (;;) {
		/*
		 * Read a command to tell us what to do
		 */
		cp = line;
		do {
			if (cp >= &line[sizeof(line) - 1])
				fatal("Command line too long");
			if ((size = read(1, cp, 1)) != 1) {
				if (size < 0)
					frecverr("%s: Lost connection", printer);
				return nfiles;
			}
		} while (*cp++ != '\n');
		*--cp = '\0';
		cp = line;
		switch (*cp++) {
		case '\1':	/* cleanup because data sent was bad */
			rcleanup();
			continue;

		case '\2':	/* read cf file */
			size = 0;
			while (isdigit(*cp))
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			/*
			 * host name has been authenticated, we use our
			 * view of the host name since we may be passed
			 * something different than what gethostbyaddr()
			 * returns
			 */
			strcpy(cp + PREFIXLEN, from);
			strcpy(tfname, cp);
			tfname[0] = 't';
			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			}
			if (!safepath(tfname)) {
				syslog(LOG_ERR, "bad cf filename %s", tfname);
				continue;
			}
			if (!readfile(tfname, size)) {
				rcleanup();
				continue;
			}
			if (link(tfname, cp) < 0)
				frecverr("%s: %m", tfname);
			(void) unlink(tfname);
			tfname[0] = '\0';
			nfiles++;
			continue;

		case '\3':	/* read df file */
			size = 0;
			while (isdigit(*cp))
				size = size * 10 + (*cp++ - '0');
			if (*cp++ != ' ')
				break;
			if (!chksize(size)) {
				(void) write(1, "\2", 1);
				continue;
			}
			strcpy(dfname, cp);
			if (!safepath(dfname)) {
				syslog(LOG_ERR, "bad df filename %s", dfname);
				continue;
			}
			(void) readfile(dfname, size);
			continue;
		}
		frecverr("protocol screwup");
	}
}

/*
 * Read files send by lpd and copy them to the spooling directory.
 */
static
readfile(file, size)
	char *file;
	int size;
{
	register char *cp;
	char buf[BUFSIZ];
	register int i, j, amt;
	int fd, err;

	fd = open(file, O_WRONLY|O_CREAT|O_EXCL, FILMOD);
	if (fd < 0)
		frecverr("%s: %m", file);
	ack();
	err = 0;
	for (i = 0; i < size; i += BUFSIZ) {
		amt = BUFSIZ;
		cp = buf;
		if (i + amt > size)
			amt = size - i;
		do {
			j = read(1, cp, amt);
			if (j <= 0)
				frecverr("Lost connection");
			amt -= j;
			cp += j;
		} while (amt > 0);
		amt = BUFSIZ;
		if (i + amt > size)
			amt = size - i;
		if (write(fd, buf, amt) != amt) {
			err++;
			break;
		}
	}
	(void) close(fd);
	if (err)
		frecverr("%s: write error", file);
	if (noresponse()) {		/* file sent had bad data in it */
		(void) unlink(file);
		return 0;
	}
	ack();
	return 1;
}

static
noresponse()
{
	char resp;

	if (read(1, &resp, 1) != 1)
		frecverr("Lost connection");
	return (resp != '\0');
}

/*
 * Check to see if there is enough space on the file system for size bytes.
 * 1 == OK, 0 == Not OK.
 */

#define MINFREEUNIT	1024

chksize(size)
	int size;
{
	int spacefree;
	struct statfs statfs;

	if (fstatfs(dfd, &statfs) < 0)	/* XXX */
		return 1;
	spacefree = howmany(statfs.f_bavail * statfs.f_bsize, MINFREEUNIT);
	size = howmany(size, MINFREEUNIT);
	return (spacefree >= minfree + size);
}

read_number(fn)
	char *fn;
{
	char lin[80];
	register FILE *fp;

	if ((fp = fopen(fn, "r")) == NULL)
		return (0);
	if (fgets(lin, 80, fp) == NULL) {
		fclose(fp);
		return (0);
	}
	fclose(fp);
	return (atoi(lin));
}

/*
 * Remove all the files associated with the current job being transfered.
 */
static void
rcleanup()
{

	if (tfname[0] && safepath(tfname))
		(void) unlink(tfname);
	if (dfname[0])
		do {
			do
				if (safepath(dfname))
					(void) unlink(dfname);
			while (dfname[2]-- != 'A');
			dfname[2] = 'z';
		} while (dfname[0]-- != 'd');
	dfname[0] = '\0';
}

/*VARARGS1*/
static
frecverr(msg, a1, a2)
	char *msg;
{
	rcleanup();
	syslog(LOG_ERR, msg, a1, a2);
	putchar('\1');		/* return error code */
	exit(1);
}

