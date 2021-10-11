#ifndef lint
static	char sccsid[] = "@(#)touch.c 1.1 92/07/30 SMI"; /* from S5R3 1.7 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <time.h>
#include <errno.h>

#define	isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

struct	stat	stbuf;
int	status;
#ifdef S5EMUL
int dmsize[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
#endif S5EMUL

static char usage[] =
#ifdef S5EMUL
		"[-amc] [mmddhhmm[yy]]";
#else /*!S5EMUL*/
		"[-amcf]";
int	force = 0;
int	nowrite;
#endif /*!S5EMUL*/

int	mflg=1, aflg=1, cflg=0, nflg=0;
char	*prog;

#ifdef S5EMUL
char	*cbp;
#endif S5EMUL
time_t	time();
off_t	lseek();
time_t	timbuf, timelocal(), timegm();

#ifdef S5EMUL
struct tm *
gtime()
{
	static struct tm newtime;
	long nt;

	newtime.tm_mon = gpair() - 1;
	newtime.tm_mday = gpair();
	newtime.tm_hour = gpair();
	if (newtime.tm_hour == 24) {
		newtime.tm_hour = 0;
		newtime.tm_mday++;
	}
	newtime.tm_min = gpair();
	newtime.tm_sec = 0;
	newtime.tm_year = gpair();
	if (newtime.tm_year < 0) {
		(void) time(&nt);
		newtime.tm_year = localtime(&nt)->tm_year;
	}
	return (&newtime);
}

gpair()
{
	register int c, d;
	register char *cp;

	cp = cbp;
	if (*cp == 0)
		return (-1);
	c = (*cp++ - '0') * 10;
	if (c<0 || c>100)
		return (-1);
	if (*cp == 0)
		return (-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return (-1);
	cbp = cp;
	return (c+d);
}
#endif /*S5EMUL*/

main(argc, argv)
	int argc;
	char *argv[];
{
	register c;
#ifdef S5EMUL
	int days_in_month;
	struct tm *tp;
#endif S5EMUL

	int errflg=0, optc;
	extern char *optarg;
	extern int optind;
	extern int opterr;

	prog = argv[0];
	opterr = 0;			/* disable getopt() error msgs */
	while ((optc=getopt(argc, argv, "amcf")) != EOF)
		switch (optc) {
		case 'm':
			mflg++;
			aflg--;
			break;
		case 'a':
			aflg++;
			mflg--;
			break;
		case 'c':
			cflg++;
			break;
#ifndef S5EMUL
		case 'f':
			force++;	/* SysV version ignores -f */
			break;
#endif /*!S5EMUL*/
		case '?':
			errflg++;
		}

	if (((argc-optind) < 1) || errflg) {
		(void) fprintf(stderr, "usage: %s %s file ...\n", prog, usage);
		exit(2);
	}
	status = 0;

#ifdef S5EMUL
	if (!isnumber(argv[optind])) {	/* BSD version only sets Present */
#endif /*S5EMUL*/
		if ((aflg <= 0) || (mflg <= 0))
			timbuf = time((time_t *)0);
		else
			nflg++;		/* no -a, -m, or date seen */
#ifdef S5EMUL
	} else {			/* SysV version sets arbitrary date */
		cbp = (char *)argv[optind++];
		if ((tp = gtime()) == NULL) {
			(void) fprintf(stderr, "%s: bad date conversion\n",
			    prog);
			exit(2);
		}
		days_in_month = dmsize[tp->tm_mon];
		if (tp->tm_mon == 1 && isleap(tp->tm_year + 1900))
			days_in_month = 29;	/* February in leap year */
		if (tp->tm_mon < 0 || tp->tm_mon > 11 ||
		    tp->tm_mday < 1 || tp->tm_mday > days_in_month ||
		    tp->tm_hour < 0 || tp->tm_hour > 23 ||
		    tp->tm_min < 0 || tp->tm_min > 59 ||
		    tp->tm_sec < 0 || tp->tm_sec > 59) {
			(void) fprintf(stderr, "%s: bad date conversion\n",
			    prog);
			exit(2);
		}
		timbuf = timelocal(tp);
	}
#endif /*S5EMUL*/

	for (c = optind; c < argc; c++) {
		if (touch(argv[c]) < 0)
			status++;
	}
	exit(status);
	/* NOTREACHED */
}

int
touch(filename)
	char *filename;
{
	time_t times[2];
	register int fd;

	if (stat(filename, &stbuf)) {
		if (cflg) {
			return (-1);
		}
		else if ((fd = creat(filename, 0666)) < 0) {
			(void) fprintf(stderr, "%s: cannot create ", prog);
			perror(filename);
			return (-1);
		}
		else {
			(void) close(fd);
			if (stat(filename, &stbuf)) {
				(void) fprintf(stderr,"%s: cannot stat ", prog);
				perror(filename);
				return (-1);
			}
		}
		if (nflg)
			return (0);
	}

	times[0] = times[1] = timbuf;
	if (mflg <= 0)
		times[1] = stbuf.st_mtime;
	if (aflg <= 0)
		times[0] = stbuf.st_atime;

#ifndef S5EMUL
	/*
	 * Since utime() allows the owner to change file times without
	 * regard to access permission, enforce BSD semantics here
	 * (cannot touch if read-only and not -f).
	 */
	nowrite = access(filename, R_OK|W_OK);
	if (nowrite & !force) {
		(void) fprintf(stderr,
		    "%s: cannot touch %s: no write permission\n",
		    prog, filename);
		return (-1);
	}
#endif /*!S5EMUL*/

	if (utime(filename, nflg ? (time_t *)0 : times)) {
		if (nflg && (errno != EROFS) && (errno != EACCES)) {
			/*
			 * If utime() failed to set the Present, it
			 * could be a BSD server that is complaining.
			 * If that's the case, try the old read/write trick.
			 */
			return (oldtouch(filename, &stbuf));
		}
		(void) fprintf(stderr,"%s: cannot change times on ", prog);
		perror(filename);
		return (-1);
	}
	return (0);
}

int
oldtouch(filename, statp)
	char *filename;
	register struct stat *statp;
{
	int rwstatus;

	if ((statp->st_mode & S_IFMT) != S_IFREG) {
		(void) fprintf(stderr,
		    "%s: %s: only owner may touch special files on this filesystem\n",
		    prog, filename);
		return (-1);
	}

#ifndef S5EMUL
	if (nowrite && force) {
		if (chmod(filename, 0666)) {
			fprintf(stderr, "%s: could not chmod ", prog);
			perror(filename);
			return (-1);
		}
		rwstatus = readwrite(filename, statp->st_size);
		if (chmod(filename, (int)statp->st_mode)) {
			fprintf(stderr, "%s: could not chmod back ", prog);
			perror(filename);
			return (-1);
		}
		return (rwstatus);
	} else
#endif /*!S5EMUL*/
		return (readwrite(filename, statp->st_size));
}

int
readwrite(filename, size)
	char	*filename;
	off_t	size;
{
	int fd;
	char first;

	if (size) {
		if ((fd = open(filename, 2)) < 0)
			goto error;
		if (read(fd, &first, 1) != 1)
			goto closeerror;
		if (lseek(fd, 0L, 0) == -1)
			goto closeerror;
		if (write(fd, &first, 1) != 1)
			goto closeerror;
	} else {
		if ((fd = creat(filename, 0666)) < 0)
			goto error;
	}
	if (close(fd) < 0)
		goto error;
	return (0);

closeerror:
	(void) close(fd);
error:
	(void) fprintf(stderr, "%s: could not touch ", prog);
	perror(filename);
	return (-1);
}

#ifdef S5EMUL
isnumber(s)
	char *s;
{
	register c;

	while (c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}
#endif S5EMUL
