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
static char sccsid[] = "%Z%%M% %I% %E% SMI"; /* from UCB 5.2 11/17/85 */
#endif not lint

/*
 *	lpr -- off line print
 *
 * Allows multiple printers and printers on remote machines by
 * using information from a printer data base.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <strings.h>
#include <varargs.h>
#include "lp.h"

char	*tfname;		/* tmp copy of cf before linking */
char	*cfname;		/* daemon control files, linked from tf's */
char	*dfname;		/* data files */

int	nact;			/* number of jobs to act on */
int	tfd;			/* control file descriptor */
int	mailflg;		/* send mail */
int	qflag;			/* q job, but don't exec daemon */
char	format = 'f';		/* format char for printing files */
int	rflag;			/* remove files upon completion */
int	sflag;			/* symbolic link flag */
int	inchar;			/* location to increment char in file names */
int	ncopies = 1;		/* # of copies to make */
int	iflag;			/* indentation wanted */
int	indent;			/* amount to indent */
int	silent;			/* Suppress messages from lp */
int	wrtflg;			/* write to user terminal when done */
int	hdr = 1;		/* print header or not (default is yes) */
int	userid;			/* user id */
char	*person;		/* user name */
char	*title;			/* pr'ing title */
int	banner;			/* print title on banner page */
char	*bannerttl;		/* title to appear on the banner page */
char	*fonts[4];		/* troff font names */
char	*width;			/* width for versatec printing */
char	host[MAXHOSTNAMELEN + 1]; /* host name */
char	*class = host;		/* class title on header page */
char	*jobname;		/* job name on header page */
char	*name;			/* program name */
char	*printer;		/* printer name */
struct	stat statb;

int	MX;			/* maximum number of blocks to copy */
int	MC;			/* maximum number of copies allowed */
int	DU;			/* daemon user-id */
char	*SD;			/* spool directory */
char	*LO;			/* lock file name */
char	*RG;			/* restrict group */
short	SC;			/* suppress multiple copies */
char	*LP;			/* line printer */
char	*AG;			/* comma seperated list of allowed groups */

int	grp_ok;			/* 1 == member of an allowed group */

char	*getenv();
char	*getwd();
char	*getlogin();
off_t	lseek();
void	exit();
void	free();

static void	cleanup(), lprarg(), lparg();
static void	card(), copy(), chkprinter(), fatal();
static int	mktemps(), nfile(), test();
static char	*mktempfile(), *linked(), *itoa();

static int  argcnt;
static char **argvar;

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	struct passwd *pw;
	struct group *gptr;
	register char *arg, *cp;
	char buf[BUFSIZ];
	char *tty, *ttyname();
	int i, f;
	struct stat stb;
	int jobno;

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		(void) signal(SIGHUP, cleanup);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGINT, cleanup);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void) signal(SIGQUIT, cleanup);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void) signal(SIGTERM, cleanup);

	cp = rindex(argv[0], '/');
	name = (cp == NULL) ? argv[0] : cp + 1;

	argcnt = argc;
	argvar = argv;

	(void)gethostname(host, sizeof host);
	openlog("lpd", 0, LOG_LPR);

	/*
	 * call lparg if we have invoked the program as "lp", 
	 * and call lprarg otherwise.
	 */
	if (strcmp(name, "lp") == 0)
		lparg();
	else
		lprarg();

	if (SC && ncopies > 1)
		fatal("multiple copies are not allowed");
	if (MC > 0 && ncopies > MC)
		fatal("only %d copies are allowed", MC);
	/*
	 * Get the identity of the person doing the lpr using the same
	 * algorithm as lprm.
	 */
	if ((person = getlogin()) == NULL || (pw = getpwnam(person)) == NULL) {
		userid = getuid();
		if ((pw = getpwuid(userid)) == NULL)
			fatal("Who are you?");
		person = pw->pw_name;
	} else
		userid = pw->pw_uid;
	/*
	 * Check for restricted group access.
	 */
	if (RG != NULL) {
		if ((gptr = getgrnam(RG)) == NULL)
			fatal("Restricted group specified incorrectly");
		if (gptr->gr_gid != getgid()) {
			while (*gptr->gr_mem != NULL) {
				if ((strcmp(person, *gptr->gr_mem)) == 0)
					break;
				gptr->gr_mem++;
			}
			if (*gptr->gr_mem == NULL)
				fatal("Not a member of the restricted group");
		}
	}
	/*
	 * Check to make sure queuing is enabled if userid is not root.
	 */
	(void) sprintf(buf, "%s/%s", SD, LO);
	if (userid && stat(buf, &stb) == 0 && (stb.st_mode & 010))
		fatal("Printer queue is disabled");
	/*
	 * Check to see if the user is a member of an allowed group.
	 */
	grp_ok = 1;	/* Assume allowed to use printer. */
	if ( (AG != NULL) && (pw->pw_uid != 0) ) {
		char *cp, *ag;
		int ngrps, i;
		struct group *gp;
		int groups[NGROUPS];

		if ((ngrps = getgroups(NGROUPS, groups)) == -1) {
			perror("getgroups");
			grp_ok = 0;
			goto LABEL;
		}
		grp_ok = 0;
		ag = malloc(strlen(AG) +1);
		(void) strcpy(ag, AG);
		cp = strtok(ag, ",");
		for (; cp != NULL; cp = strtok(NULL, ",")) {
			gp = getgrnam(cp);
			if ( gp == NULL ) {
				continue; /* Skip nonexistent groups */
			}
			for (i = 0; i < ngrps; i++) {
				if (groups[i] == gp->gr_gid) {
					grp_ok = 1;
					goto LABEL;
				}
			}
		}
	}
    LABEL:
	if ( grp_ok == 0 ) {
		char line[256];
		sprintf(line, "You are not allowed to use printer %s", printer);		fatal(line);
	}
	/*
	 * Initialize the control file.
	 */
	jobno = mktemps();
	tfd = nfile(tfname);
	(void) fchown(tfd, DU, -1);	/* owned by daemon for protection */
	card('H', host);
	card('P', person);
	if (hdr) {
		if (jobname == NULL) {
			if (argcnt == 1)
				jobname = "stdin";
			else {
				arg = rindex(argvar[1], '/');
				jobname = (arg != NULL) ? arg + 1 : argvar[1];
			}
		}
		card('J', jobname);
		card('C', class);
		if (banner)
			card('B', bannerttl);
		card('L', person);
	}
	if (iflag)
		card('I', itoa(indent));
	if (mailflg)
		card('M', person);
	if (wrtflg && !silent)
		/*
		 * write to terminal if printer is local,
		 * send mail otherwise
		 */
		if (LP != NULL && *LP != '\0') {
			if ((tty = ttyname(2)) != NULL)
				card('R', tty);
		}
		else
			card('M', person);
	if (format == 't' || format == 'n' || format == 'd')
		for (i = 0; i < 4; i++)
			if (fonts[i] != NULL)
				card('1' + i, fonts[i]);
	if (width != NULL)
		card('W', width);

	/*
	 * Read the files and spool them.
	 */
	if (argcnt == 1)
		copy(0, "standard input");
	else while (--argcnt) {
		arg = *++argvar;
		/* '-' means read from standard input */
		if (strcmp(name, "lp") == 0 && strcmp(arg, "-") == 0) {
			copy(0, "standard input");
			continue;
		}
		if ((f = test(arg)) < 0)
			continue;	/* file unreasonable */

		if (sflag && (cp = linked(arg)) != NULL) {
			(void) sprintf(buf, "%u %ul",
					(u_short)statb.st_dev, statb.st_ino);
			card('S', buf);
			if (format == 'p')
				card('T', title ? title : arg);
			for (i = 0; i < ncopies; i++)
				card(format, &dfname[inchar-2]);
			card('U', &dfname[inchar-2]);
			if (f)
				card('U', cp);
			card('N', arg);
			dfname[inchar]++;
			nact++;
			continue;
		}
		if (sflag)
			printf("%s: %s: not linked, copying instead\n", name, arg);
		if ((i = open(arg, O_RDONLY)) < 0) {
			printf("%s: cannot open %s\n", name, arg);
			continue;
		}
		copy(i, arg);
		(void) close(i);
		if (f && unlink(arg) < 0)
			printf("%s: %s: not removed\n", name, arg);
	}

	if (nact) {
		(void) close(tfd);
		tfname[inchar]--;
		/*
		 * Touch the control file to fix position in the queue.
		 */
		if ((tfd = open(tfname, O_RDWR)) >= 0) {
			char c;

			if (read(tfd, &c, 1) == 1 &&
			    lseek(tfd, 0L, L_SET) == 0 &&
			    write(tfd, &c, 1) != 1) {
				printf("%s: cannot touch %s\n", name, tfname);
				tfname[inchar]++;
				cleanup();
			}
			(void) close(tfd);
		}
		if (link(tfname, cfname) < 0) {
			printf("%s: cannot rename %s\n", name, cfname);
			tfname[inchar]++;
			cleanup();
		}
		(void)unlink(tfname);
		if (qflag)		/* just q things up */
			exit(0);
		if (!startdaemon(printer))
			printf("jobs queued, but cannot start daemon.\n");
		/* if this is lp, return the request id to the user */
		if (strcmp(name, "lp") == 0 && !silent)
			printf("request id is %d\n", jobno);
		exit(0);
	}
	cleanup();
	/* NOTREACHED */
}

static void
lprarg()
{
	register char *arg;
	int i;

	while (argcnt > 1 && argvar[1][0] == '-') {
		argcnt--;
		arg = *++argvar;
		switch (arg[1]) {

		case 'P':		/* specifiy printer name */
			if (arg[2])
				printer = &arg[2];
			else if (argcnt > 1) {
				argcnt--;
				printer = *++argvar;
			}
			break;

		case 'C':		/* classification spec */
			hdr++;
			if (arg[2])
				class = &arg[2];
			else if (argcnt > 1) {
				argcnt--;
				class = *++argvar;
			}
			break;

		case 'J':		/* job name */
			hdr++;
			if (arg[2])
				jobname = &arg[2];
			else if (argcnt > 1) {
				argcnt--;
				jobname = *++argvar;
			}
			break;

		case 'T':		/* pr's title line */
			if (arg[2])
				title = &arg[2];
			else if (argcnt > 1) {
				argcnt--;
				title = *++argvar;
			}
			break;

		case 'l':		/* literal output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output (cat files) */
		case 'n':		/* print ditroff output */
		case 'd':		/* print tex output (dvi files) */
		case 'g':		/* print graph(1G) output */
		case 'c':		/* print cifplot output */
		case 'v':		/* print vplot output */
			format = arg[1];
			break;

		case 'f':		/* print fortran output */
			format = 'r';
			break;

		case '4':		/* troff fonts */
		case '3':
		case '2':
		case '1':
			if (argcnt > 1) {
				argcnt--;
				fonts[arg[1] - '1'] = *++argvar;
			}
			break;

		case 'w':		/* versatec page width */
			width = arg+2;
			break;

		case 'r':		/* remove file when done */
			rflag++;
			break;

		case 'm':		/* send mail when done */
			mailflg++;
			break;

		case 'h':		/* toggle want of header page */
			hdr = !hdr;
			break;

		case 's':		/* try to link files */
			sflag++;
			break;

		case 'q':		/* just q job */
			qflag++;
			break;

		case 'i':		/* indent output */
			iflag++;
			indent = arg[2] ? atoi(&arg[2]) : 8;
			break;

		case '#':		/* n copies */
			if (isdigit(arg[2])) {
				i = atoi(&arg[2]);
				if (i > 0)
					ncopies = i;
			}
		}
	}
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;
	chkprinter(printer);
}

static void
lparg()
{
	char *arg;
	int copyflg=0;
	int i;

	while (argcnt > 1 && argvar[1][0] == '-') {
		argcnt--;
		arg = *++argvar;
		switch (arg[1]) {
			case 'c': /* copy files instead of creating links */
				copyflg++;
				break;
			case 'd':		/* specify destination */
				if (arg[2])
					printer = &arg[2];
				else if (argcnt > 1) {
					argcnt--;
					printer = *++argvar;
				}
				break;
			case 'm':
				mailflg++;
				break;
			case 'n':		/* n copies */
				if (isdigit(arg[2])) {
					i = atoi(&arg[2]);
					if (i > 0)
						ncopies = i;
				}
				break;
			case 'o':	/* printer specific options, no
					  way to pass this to the printer */
				break;
			case 's':
				silent++;
				break;
			case 't':
				hdr++;
				banner++;
				if (arg[2])
					bannerttl = &arg[2];
				else if (argcnt > 1) {
					argcnt--;
					bannerttl = *++argvar;
				}
				break;
			case 'w':
				wrtflg++;
				break;
			default:
				if (strcmp(arg, "-") == 0) {  /* backtrack */
					argcnt++;
					argvar--;
				}
				else
					if (!silent)
						fatal("lp: [-c] [-ddest] [-m] [-nnumber] [-ooption] [-s] -[ttitle] [-w] files");

		}
	}
	if (printer == NULL)
		if (((printer = getenv("LPDEST")) == NULL) &&
				 ((printer = getenv("PRINTER")) == NULL))
			printer = DEFLP;
	chkprinter(printer);

	if (copyflg == 0 && LP != NULL && *LP != '\0')
		sflag++;
}
/*
 * Create the file n and copy from file descriptor f.
 */
static void
copy(f, n)
	int f;
	char n[];
{
	register int fd, i, nr = 0, nc = 0, first = 1;
	char *tn;
	char buf[BUFSIZ];

	tn = malloc((u_int)(strlen(dfname) - (inchar - 2) + 1));
	if (tn == NULL)
		fatal("out of memory");
	strcpy(tn, &dfname[inchar - 2]); /* save current temp file name */
	fd = nfile(dfname);
	while ((i = read(f, buf, BUFSIZ)) > 0) {
		if (first) {
			first = 0;
			if (i >= 2 && buf[0] == '\100' && buf[1] == '\357' &&
			    format != 't') {
				printf(
			"%s: %s is a troff file, assuming '-t'\n", name, n);
				format = 't';
			}
		}
		if (write(fd, buf, i) != i) {
			printf("%s: %s: temp file write error\n", name, n);
			break;
		}
		nc += i;
		if (nc >= BUFSIZ) {
			nc -= BUFSIZ;
			nr++;
			if (MX > 0 && nr > MX) {
				printf("%s: %s: copy file is too large\n", name, n);
				break;
			}
		}
	}
	(void) close(fd);
	if (nc==0 && nr==0) {
		printf("%s: %s: empty input file\n", name, n);
		free(tn);
		return;
	}
	nact++;
	if (format == 'p')
		card('T', title ? title : n);
	for (i = 0; i < ncopies; i++)
		card(format, tn);
	card('U', tn);
	card('N', n);
	free(tn);
}

/*
 * Try and link the file to dfname. Return a pointer to the full
 * path name if successful.
 */
static char *
linked(file)
	register char *file;
{
	register char *cp;
	static char buf[MAXPATHLEN];

	if (*file != '/') {
		if (getwd(buf) == NULL)
			return NULL;
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':
				file += 2;
				continue;
			case '.':
				if (file[2] == '/') {
					if ((cp = rindex(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		strcat(buf, "/");
		strcat(buf, file);
		file = buf;
	}
	return (symlink(file, dfname) ? NULL : file);
}

/*
 * Put a line into the control file.
 */
static void
card(c, p2)
	register char c, *p2;
{
	char buf[BUFSIZ];
	register char *p1 = buf;
	register int len = 2;

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = (c == '\n') ? ' ' : c;
		len++;
	}
	*p1++ = '\n';
	(void)write(tfd, buf, len);
}

/*
 * Create a new file in the spool directory.
 */
static int
nfile(n)
	char *n;
{
	register int f;
	int oldumask = umask(0);		/* should block signals */

        f = open(n, O_WRONLY|O_CREAT|O_EXCL ,FILMOD);

	(void) umask(oldumask);
	if (f < 0) {
		printf("%s: cannot create %s\n", name, n);
		cleanup();
	}
	if (fchown(f, userid, -1) < 0) {
		printf("%s: cannot chown %s\n", name, n);
		cleanup();
	}
	if (++n[inchar] > 'z') {
		if (++n[inchar-2] == 't') {
			printf("too many files - break up the job\n");
			cleanup();
		}
		n[inchar] = 'A';
	} else if (n[inchar] == '[')
		n[inchar] = 'a';
	return f;
}

/*
 * Cleanup after interrupts and errors.
 */
static void
cleanup()
{
	register int i;

	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGTERM, SIG_IGN);
	i = inchar;
	if (tfname)
		do
			(void) unlink(tfname);
		while (tfname[i]-- != 'A');
	if (cfname)
		do
			(void) unlink(cfname);
		while (cfname[i]-- != 'A');
	if (dfname)
		do {
			do
				(void) unlink(dfname);
			while (dfname[i]-- != 'A');
			dfname[i] = 'z';
		} while (dfname[i-2]-- != 'd');
	exit(1);
}

/*
 * Test to see if this is a printable file.
 * Return -1 if it is not, 0 if its printable, and 1 if
 * we should remove it after printing.
 */
static int
test(file)
	char *file;
{
	struct exec execb;
	register int fd;
	register char *cp;

	if (access(file, R_OK) < 0) {
		printf("%s: cannot access %s\n", name, file);
		return (-1);
	}
	if (stat(file, &statb) < 0) {
		printf("%s: cannot stat %s\n", name, file);
		return (-1);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		printf("%s: %s is a directory\n", name, file);
		return (-1);
	}
	if (statb.st_size == 0) {
		printf("%s: %s is an empty file\n", name, file);
		return (-1);
 	}
	if ((fd = open(file, O_RDONLY)) < 0) {
		printf("%s: cannot open %s\n", name, file);
		return (-1);
	}
	if (read(fd, (char *)&execb, sizeof execb) == sizeof execb)
		switch (execb.a_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
#ifdef A_MAGIC4
		case A_MAGIC4:
#endif
			printf("%s: %s is an executable program", name, file);
			goto error1;

		case ARMAG:
			printf("%s: %s is an archive file", name, file);
			goto error1;

		default:
			cp = (char *)&execb;
			if (cp[0] == '\100' && cp[1] == '\357' && format != 't') {
				printf(
			"%s: %s is a troff file, assuming '-t'\n", name, file);
				format = 't';
			}
		}
	(void) close(fd);
	if (rflag) {
		if ((cp = rindex(file, '/')) == NULL) {
			if (access(".", W_OK) == 0)
				return (1);
		} else {
			*cp = '\0';
			fd = access(file, W_OK);
			*cp = '/';
			if (fd == 0)
				return (1);
		}
		printf("%s: %s: is not removable by you\n", name, file);
	}
	return (0);

error1:
	printf(" and is unprintable\n");
	(void) close(fd);
	return (-1);
}

/*
 * itoa - integer to string conversion
 */

#define MAXDIGITS 11

static char *
itoa(i)
	register int i;
{
	static char b[MAXDIGITS + 1];

	(void)sprintf(b, "%d", i);
	return b;
}

/*
 * Perform lookup for printer name or abbreviation --
 */

char pbuf[BUFSIZ/2], *bp;

static void
chkprinter(s)
	char *s;
{
	int status;
	char buf[BUFSIZ];
	extern char *pgetstr();

	if ((status = pgetent(buf, s)) < 0)
		fatal("cannot open printer description file");
	else if (status == 0)
		fatal("%s: unknown printer", s);
	bp = pbuf;
	AG = pgetstr("ag", &bp);
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	RG = pgetstr("rg", &bp);
	LP = pgetstr("lp", &bp);
	if ((MX = pgetnum("mx")) < 0)
		MX = DEFMX;
	if ((MC = pgetnum("mc")) < 0)
		MC = DEFMAXCOPIES;
	if ((DU = pgetnum("du")) < 0)
		DU = DEFUID;
	SC = pgetflag("sc");
}

/*
 * Make the temp files.
 */
static int
mktemps()
{
	register int len, fd, n;
	register char *cp;
	char buf[BUFSIZ];
	int id;

	(void) sprintf(buf, "%s/.seq", SD);
	if ((fd = open(buf, O_RDWR|O_CREAT, 0661)) < 0) {
		printf("%s: cannot create %s\n", name, buf);
		exit(1);
	}
	if (flock(fd, LOCK_EX)) {
		printf("%s: cannot lock %s\n", name, buf);
		exit(1);
	}
	n = 0;
	if ((len = read(fd, buf, sizeof buf)) > 0) {
		for (cp = buf; len--; ) {
			if (!isdigit(*cp))
				break;
			n = n * 10 + (*cp++ - '0');
		}
	}
	len = strlen(SD) + strlen(host) + 8;
	tfname = mktempfile("tf", n, len);
	cfname = mktempfile("cf", n, len);
	dfname = mktempfile("df", n, len);
	inchar = strlen(SD) + 3;
	id = n;
	n = (n + 1) % 1000;
	(void)lseek(fd, 0L, L_SET);
	sprintf(buf, "%03d\n", n);
	(void)write(fd, buf, strlen(buf));
	(void)close(fd);	/* unlocks as well */
	return id;
}

/*
 * Make a temp file name.
 */
static char *
mktempfile(id, num, len)
	char	*id;
	int	num, len;
{
	register char *s;
	extern char *malloc();

	if ((s = malloc((u_int)len)) == NULL)
		fatal("out of memory");
	(void) sprintf(s, "%s/%sA%03d%s", SD, id, num, host);
	return s;
}

/*VARARGS*/
static void
fatal(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	(void)printf("%s: ", name);
	va_start(args);
	fmt = va_arg(args, char *);
	(void)vprintf(fmt, args);
	va_end(args);
	putchar('\n');
	exit(1);
}
