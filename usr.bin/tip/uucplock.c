#ifndef lint
static	char sccsid[] = "@(#)uucplock.c 1.1 92/07/30 SMI"; /* from UCB 4.6 6/25/83 */
#endif
/*
 * defs that come from uucp.h
 */
#define NAMESIZE 40
#define FAIL -1
#define SAME 0
#define SLCKTIME 28800	/* system/device timeout (LCK.. files) in seconds (8 hours) */
#define ASSERT(e, f, v) if (!(e)) {\
	fprintf(stderr, "AERROR - (%s) ", "e");\
	fprintf(stderr, f, v);\
	finish(FAIL);\
}
#define	SIZEOFPID	10		/* maximum number of digits in a pid */

#define LOCKDIR "/usr/spool/locks"
#define LOCKPRE "LCK."

/*
 * This code is taken almost directly from uucp and follows the same
 * conventions.  This is important since uucp and tip should
 * respect each others locks.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

static void	stlock();
static int	onelock();
static int	checkLock();


/*******
 *	ulockf(file, atime)
 *	char *file;
 *	time_t atime;
 *
 *	ulockf  -  this routine will create a lock file (file).
 *	If one already exists, send a signal 0 to the process--if
 *	it fails, then unlink it and make a new one.
 *
 *	input:
 *		file - name of the lock file
 *		atime - is unused, but we keep it for lint compatibility
 *			with non-ATTSVKILL
 *
 *	return codes:  0  |  FAIL
 */

static
ulockf(file, atime)
	char *file;
	time_t atime;
{
	static char pid[SIZEOFPID+2] = { '\0' }; /* +2 for '\n' and NULL */
	static char tempfile[NAMESIZE];

	if (pid[0] == '\0') {
		(void) sprintf(pid, "%*d\n", SIZEOFPID, getpid());
		(void) sprintf(tempfile, "%s/LTMP.%d", LOCKDIR, getpid());
	}
	if (onelock(pid, tempfile, file) == -1) {
		/* lock file exists */
		(void) unlink(tempfile);
		if (checkLock(file))
			return (FAIL);
		else {
			if (onelock(pid, tempfile, file)) {
				(void) unlink(tempfile);
				return (FAIL);
			}
		}
	}
	stlock(file);
	return (0);
}

/*
 * check to see if the lock file exists and is still active
 * - use kill(pid,0) - (this only works on ATTSV and some hacked
 * BSD systems at this time)
 * return:
 *	0	-> success (lock file removed - no longer active)
 *	FAIL	-> lock file still active
 */
static int
checkLock(file)
register char *file;
{
	register int ret;
	int lpid = -1;
	char alpid[SIZEOFPID+2];	/* +2 for '\n' and NULL */
	int fd;
	extern int errno;

	fd = open(file, 0);
	if (fd == -1) {
		if (errno == ENOENT)  /* file does not exist -- OK */
			return (0);
		goto unlk;
	}
	ret = read(fd, (char *) alpid, SIZEOFPID+1); /* +1 for '\n' */
	(void) close(fd);
	if (ret != (SIZEOFPID+1))
		goto unlk;
	lpid = atoi(alpid);
	if ((ret=kill(lpid, 0)) == 0 || errno == EPERM)
		return (FAIL);

unlk:
	if (unlink(file) != 0)
		return (FAIL);
	return (0);
}

#define MAXLOCKS 10	/* maximum number of lock files */
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

/***
 *	stlock(name)	put name in list of lock files
 *	char *name;
 *
 *	return codes:  none
 */

static void
stlock(name)
	char *name;
{
	char *p;
	extern char *calloc();
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			break;
	}
	ASSERT(i < MAXLOCKS, "TOO MANY LOCKS %d", i);
	if (i >= Nlocks)
		i = Nlocks++;
	p = calloc(strlen(name) + 1, sizeof (char));
	ASSERT(p != NULL, "CAN NOT ALLOCATE FOR %s", name);
	strcpy(p, name);
	Lockfile[i] = p;
	return;
}

/***
 *	rmlock(name)	remove all lock files in list
 *	char *name;	or name
 *
 *	return codes: none
 */

static
rmlock(name)
	char *name;
{
	int i;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		if (name == NULL || strcmp(name, Lockfile[i]) == SAME) {
			unlink(Lockfile[i]);
			free(Lockfile[i]);
			Lockfile[i] = NULL;
		}
	}
}

static
onelock(pid, tempfile, name)
	char *pid, *tempfile, *name;
{
	int fd;
	static int first = 1;
	extern int errno;

	fd = creat(tempfile, 0444);
	if (fd < 0) {
		if (first) {
			if (errno == EACCES) {
				fprintf(stderr,
			  "tip: can't create files in lock file directory %s\n",
				    LOCKDIR);
			} else if (access(LOCKDIR, 0) < 0) {
				fprintf(stderr, "tip: lock file directory %s: ",
				    LOCKDIR);
				perror("");
			}
			first = 0;
		}
		if (errno == EMFILE || errno == ENFILE)
			(void) unlink(tempfile);
		return (-1);
	}
	write(fd, pid, SIZEOFPID+1);	/* +1 for '\n' */
	fchmod(fd, 0444);
	close(fd);
	if (link(tempfile, name) < 0) {
		unlink(tempfile);
		return (-1);
	}
	unlink(tempfile);
	return (0);
}

/***
 *	delock(s)	remove a lock file
 *	char *s;
 *
 *	return codes:  0  |  FAIL
 */

delock(s)
	char *s;
{
	char ln[NAMESIZE];

	sprintf(ln, "%s/%s.%s", LOCKDIR, LOCKPRE, s);
	rmlock(ln);
}

/***
 *	mlock(sys)	create system lock
 *	char *sys;
 *
 *	return codes:  0  |  FAIL
 */

mlock(sys)
	char *sys;
{
	char lname[NAMESIZE];
	sprintf(lname, "%s/%s.%s", LOCKDIR, LOCKPRE, sys);
	return (ulockf(lname, (time_t) SLCKTIME ) < 0 ? FAIL : 0);
}

/*
 * update access and modify times for lock files
 * return:
 *	none
 */
void
ultouch()
{
	register int i;
	time_t time();

	struct ut {
		time_t actime;
		time_t modtime;
	} ut;

	ut.actime = time(&ut.modtime);
	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		utime(Lockfile[i], &ut);
	}
}
