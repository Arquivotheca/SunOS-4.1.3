/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)ulockf.c 1.1 92/07/30"	/* from SVR3.2 uucp:ulockf.c 2.7 */

#include "uucp.h"

#ifdef	V7
#define O_RDONLY	0
#endif

static void	stlock();
static int	onelock();

#ifdef ATTSVKILL
/*
 * create a lock file (file).
 * If one already exists, send a signal 0 to the process--if
 * it fails, then unlink it and make a new one.
 *
 * input:
 *	file - name of the lock file
 *	atime - is unused, but we keep it for lint compatibility with non-ATTSVKILL
 *
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */
ulockf(file, atime)
register char *file;
time_t atime;
{
	static	char pid[SIZEOFPID+2] = { '\0' }; /* +2 for '\n' and NULL */

	static char tempfile[MAXNAMESIZE];

#ifdef V8
	char *cp;
#endif

	if (pid[0] == '\0') {
		(void) sprintf(pid, "%*d\n", SIZEOFPID, getpid());
		(void) sprintf(tempfile, "%s/LTMP.%d", X_LOCKDIR, getpid());
	}

#ifdef V8	/* this wouldn't be a problem if we used lock directories */
		/* some day the truncation of system names will bite us */
	cp = rindex(file, '/');
	if (cp++ != CNULL)
	    if (strlen(cp) > DIRSIZ)
		*(cp+DIRSIZ) = NULLCHAR;
#endif /* V8 */
	if (onelock(pid, tempfile, file) == -1) {
		(void) unlink(tempfile);
		if (checkLock(file))
			return(FAIL);
		else {
		    if (onelock(pid, tempfile, file)) {
			(void) unlink(tempfile);
			DEBUG(4,"ulockf failed in onelock()\n","");
			return(FAIL);
		    }
		}
	}

	stlock(file);
	return(0);
}

/*
 * check to see if the lock file exists and is still active
 * - use kill(pid,0) - (this only works on ATTSV and some hacked
 * BSD systems at this time)
 * return:
 *	0	-> success (lock file removed - no longer active
 *	FAIL	-> lock file still active
 */
checkLock(file)
register char *file;
{
	register int ret;
	int lpid = -1;
	char alpid[SIZEOFPID+2];	/* +2 for '\n' and NULL */
	int fd;
	extern int errno;

	fd = open(file, O_RDONLY);
	DEBUG(4, "ulockf file %s\n", file);
	if (fd == -1) {
	    if (errno == ENOENT)  /* file does not exist -- OK */
		return(0);
	    DEBUG(4,"Lock File--can't read (errno %d) --remove it!\n", errno);
	    goto unlk;
	}
	ret = read(fd, (char *) alpid, SIZEOFPID+1); /* +1 for '\n' */
	(void) close(fd);
	if (ret != (SIZEOFPID+1)) {

	    DEBUG(4, "Lock File--bad format--remove it!\n", "");
	    goto unlk;
	}
	lpid = atoi(alpid);
	if ((ret=kill(lpid, 0)) == 0 || errno == EPERM) {
	    DEBUG(4, "Lock File--process still active--not removed\n","");
	    return(FAIL);
	}
	else { /* process no longer active */
	    DEBUG(4, "kill pid (%d), ", lpid);
	    DEBUG(4, "returned %d", ret);
	    DEBUG(4, "--ok to remove lock file (%s)\n", file);
	}
unlk:
	
	if (unlink(file) != 0) {
		DEBUG(4,"ulockf failed in unlink()\n","");
		return(FAIL);
	}
	return(0);
}
#else	/* !ATTSVKILL */

/*
 * check to see if the lock file exists and is still active
 * - consider the lock expired after SLCKTIME seconds.
 * return:
 *	0	-> success (lock file removed - no longer active
 *	FAIL	-> lock file still active
 */
checkLock(file)
register char *file;
{
	register int ret;
	int lpid = -1;
	int fd;
	extern int errno;
	struct stat stbuf;
	time_t ptime, time();

	fd = open(file, 0);
	DEBUG(4, "ulockf file %s\n", file);
	if (fd == -1) {
	    if (errno == ENOENT)  /* file does not exist -- OK */
		return(0);
	    DEBUG(4,"Lock File--can't read (errno %d) --remove it!\n", errno);
	    goto unlk;
	}
	ret = stat(file, &stbuf);
	if (ret != -1) {
		(void) time(&ptime);
		if ((ptime - stbuf.st_ctime) < SLCKTIME) {

			/*
			 * file not old enough to delete
			 */
			return(FAIL);
		}
	}
unlk:
	DEBUG(4, "--ok to remove lock file (%s)\n", file);
	
	if (unlink(file) != 0) {
		DEBUG(4,"ulockf failed in unlink()\n","");
		return(FAIL);
	}
	return(0);
}


/*
 * create a lock file (file).
 * If one already exists, the create time is checked for
 * older than the age time (atime).
 * If it is older, an attempt will be made to unlink it
 * and create a new one.
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */
ulockf(file, atime)
register char *file;
time_t atime;
{
	register int ret;
	struct stat stbuf;
	static	char pid[SIZEOFPID+2] = { '\0' }; /* +2 for '\n' and null */
	static char tempfile[MAXNAMESIZE];
	time_t ptime, time();
#ifdef V8
	char *cp;
#endif


	if (pid[0] == '\0') {
		(void) sprintf(pid, "%*d\n", SIZEOFPID, getpid());
		(void) sprintf(tempfile, "%s/LTMP.%d", X_LOCKDIR, getpid());
	}
#ifdef V8	/* this wouldn't be a problem if we used lock directories */
		/* some day the truncation of system names will bite us */
	cp = rindex(file, '/');
	if (cp++ != CNULL)
	    if (strlen(cp) > DIRSIZ)
		*(cp+DIRSIZ) = NULLCHAR;
#endif /* V8 */
	if (onelock(pid, tempfile, file) == -1) {

		/*
		 * lock file exists
		 * get status to check age of the lock file
		 */
		(void) unlink(tempfile);
		ret = stat(file, &stbuf);
		if (ret != -1) {
			(void) time(&ptime);
			if ((ptime - stbuf.st_ctime) < atime) {

				/*
				 * file not old enough to delete
				 */
				return(FAIL);
			}
		}
		ret = unlink(file);
		ret = onelock(pid, tempfile, file);
		if (ret != 0)
			return(FAIL);
	}
	stlock(file);
	return(0);
}
#endif	/* ATTSVKILL */

#define MAXLOCKS 10	/* maximum number of lock files */
char *Lockfile[MAXLOCKS];
int Nlocks = 0;

/*
 * put name in list of lock files
 * return:
 *	none
 */
static
void
stlock(name)
char *name;
{
	register int i;
	char *p;

	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			break;
	}
	ASSERT(i < MAXLOCKS, "TOO MANY LOCKS", "", i);
	if (i >= Nlocks)
		i = Nlocks++;
	p = calloc((unsigned) strlen(name) + 1, sizeof (char));
	ASSERT(p != NULL, "CAN NOT ALLOCATE FOR", name, 0);
	(void) strcpy(p, name);
	Lockfile[i] = p;
	return;
}

/*
 * remove all lock files in list
 * return:
 *	none
 */
void
rmlock(name)
register char *name;
{
	register int i;
#ifdef V8
	char *cp;

	cp = rindex(name, '/');
	if (cp++ != CNULL)
	    if (strlen(cp) > DIRSIZ)
		*(cp+DIRSIZ) = NULLCHAR;
#endif /* V8 */


	for (i = 0; i < Nlocks; i++) {
		if (Lockfile[i] == NULL)
			continue;
		if (name == NULL || EQUALS(name, Lockfile[i])) {
			(void) unlink(Lockfile[i]);
			(void) free(Lockfile[i]);
			Lockfile[i] = NULL;
		}
	}
	return;
}



/*
 * remove a lock file
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */
delock(s)
char *s;
{
	char ln[MAXNAMESIZE];

	(void) sprintf(ln, "%s.%s", LOCKPRE, s);
	BASENAME(ln, '/')[MAXBASENAME] = '\0';
	rmlock(ln);
}


/*
 * create system or device lock
 * return:
 *	0	-> success
 *	FAIL	-> failure
 */
mlock(name)
char *name;
{
	char lname[MAXNAMESIZE];

	/*
	 * if name has a '/' in it, then it's a device name and it's
	 * not in /dev (i.e., it's a remotely-mounted device or it's
	 * in a subdirectory of /dev).  in either case, creating our normal 
	 * lockfile (/usr/spool/locks/LCK..<dev>) is going to bomb if
	 * <dev> is "/remote/dev/tty14" or "/dev/net/foo/clone", so never 
	 * mind.  since we're using advisory filelocks on the devices 
	 * themselves, it'll be safe.
	 *
	 * of course, programs and people who are used to looking at the
	 * lockfiles to find out what's going on are going to be a trifle
	 * misled.  we really need to re-consider the lockfile naming structure
	 * to accomodate devices in directories other than /dev ... maybe in
	 * the next release.
	 */
	if ( strchr(name, '/') != NULL )
		return(0);
	(void) sprintf(lname, "%s.%s", LOCKPRE, name);
	BASENAME(lname, '/')[MAXBASENAME] = '\0';
	return(ulockf(lname, SLCKTIME) < 0 ? FAIL : 0);
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
	return;
}

/*
 * makes a lock on behalf of pid.
 * input:
 *	pid - process id
 *	tempfile - name of a temporary in the same file system
 *	name - lock file name (full path name)
 * return:
 *	-1 - failed
 *	0  - lock made successfully
 */
static
onelock(pid,tempfile,name)
char *pid;
char *tempfile, *name;
{	
	register int fd;
	char	cb[100];

	fd=creat(tempfile, 0444);
	if(fd < 0){
		(void) sprintf(cb, "%s %s %d",tempfile, name, errno);
		logent("ULOCKC", cb);
		if((errno == EMFILE) || (errno == ENFILE))
			(void) unlink(tempfile);
		return(-1);
	}
	(void) write(fd, pid, SIZEOFPID+1);	/* +1 for '\n' */
	(void) chmod(tempfile,0444);
	(void) chown(tempfile, UUCPUID, UUCPGID);
	(void) close(fd);
	if(link(tempfile,name)<0){
		DEBUG(4, "%s: ", sys_errlist[errno]);
		DEBUG(4, "link(%s, ", tempfile);
		DEBUG(4, "%s)\n", name);
		if(unlink(tempfile)< 0){
			(void) sprintf(cb, "ULK err %s %d", tempfile,  errno);
			logent("ULOCKLNK", cb);
		}
		return(-1);
	}
	if(unlink(tempfile)<0){
		(void) sprintf(cb, "%s %d",tempfile,errno);
		logent("ULOCKF", cb);
	}
	return(0);
}

#ifdef ATTSV

/*
 * filelock(fd)	sets advisory file lock on file descriptor fd
 *
 * returns SUCCESS if all's well; else returns value returned by fcntl()
 *
 * needed to supplement /usr/spool/locks/LCK..<device> because can now
 * have remotely-mounted devices using RFS.  without filelock(), uucico's
 * or cu's on 2 different machines could access the same remotely-mounted 
 * device, since /usr/spool/locks is purely local. 
 *		
 */

int
filelock(fd)
int	fd;
{
	register int lockrtn, locktry = 0;
	struct flock lck;

	lck.l_type = F_WRLCK;	/* setting a write lock */
	lck.l_whence = 0;	/* offset l_start from beginning of file */
	lck.l_start = 0L;
	lck.l_len = 0L;		/* until the end of the file address space */

	/*	place advisory file locks	*/
	while ( (lockrtn = fcntl(fd, F_SETLK, &lck)) < 0 ) {

		DEBUG(7, "filelock: F_SETLK returns %d\n", lockrtn);

		switch (lockrtn) {
#ifdef EACCESS
		case EACCESS:	/* already locked */
				/* no break */
#endif /* EACCESS */
		case EAGAIN:	/* already locked */
			if ( locktry++ < MAX_LOCKTRY ) {
				sleep(2);
				continue;
			}
			logent("filelock","F_SETLK failed - lock exists");
			return(lockrtn);
			break;
		case ENOLCK:	/* no more locks available */
			logent("filelock","F_SETLK failed -- ENOLCK");
			return(lockrtn);
			break;
		case EFAULT:	/* &lck is outside program address space */
			logent("filelock","F_SETLK failed -- EFAULT");
			return(lockrtn);
			break;
		default:
			logent("filelock","F_SETLK failed");
			return(lockrtn);
			break;
		}
	}
	DEBUG(7, "filelock: ok\n", "");
	return(SUCCESS);
}
#endif /* ATTSV */
