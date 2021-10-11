#ifndef lint
static	char *sccsid = "@(#)lock.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Stuff to do version 7 style locking.
 */

#include "rcv.h"
#include <sys/stat.h>
#include <sys/file.h>

char	*maillock	= ".lock";		/* Lock suffix for mailname */
static	int		lockfd;			/* fd of mail file for flock */
static	char		curlock[PATHSIZE];	/* Last used name of lock */
static	int		locked;			/* To note that we locked it */
static	time_t		locktime;		/* time lock file was touched */

/*
 * Lock the specified mail file by setting the file mailfile.lock.
 * We must, of course, be careful to remove the lock file by a call
 * to unlock before we stop.  The algorithm used here is to see if
 * the lock exists, and if it does, to check its modify time.  If it
 * is older than 5 minutes, we assume error and set our own file.
 * Otherwise, we wait for 5 seconds and try again.
 */

lock(file)
char *file;
{
	register time_t t;
	struct stat sbuf;
	int statfailed;
	char locktmp[PATHSIZE];	/* Usable lock temporary */

	if (file == NOSTR) {
		printf("Locked = %d\n", locked);
		return(0);
	}
	if (locked)
		return(0);
	strcpy(curlock, file);
	strcat(curlock, maillock);
	strcpy(locktmp, file);
	strcat(locktmp, "XXXXXX");
	mktemp(locktmp);
	remove(locktmp);
	statfailed = 0;
	for (;;) {
		t = lock1(locktmp, curlock);
		if (t == 0) {
			locked = 1;
			locktime = time(0);
#ifndef USG
			if ((lockfd = open(file, 0)) >= 0)
				flock(lockfd, LOCK_EX);
#endif
			return(0);
		}
		if (stat(curlock, &sbuf) < 0) {
			if (statfailed++ > 5)
				return(-1);
			sleep(5);
			continue;
		}
		statfailed = 0;

		/*
		 * Compare the time of the temp file with the time
		 * of the lock file, rather than with the current
		 * time of day, since the files may reside on
		 * another machine whose time of day differs from
		 * ours.  If the lock file is less than 5 minutes
		 * old, keep trying.
		 */
		if (t < sbuf.st_ctime + 300) {
			sleep(5);
			continue;
		}
		remove(curlock);
	}
}

/*
 * Remove the mail lock, and note that we no longer
 * have it locked.
 */

unlock()
{

#ifndef USG
	flock(lockfd, LOCK_UN);
	close(lockfd);
	lockfd = -1;
#endif
	remove(curlock);
	locked = 0;
}

/*
 * Attempt to set the lock by creating the temporary file,
 * then doing a link/unlink.  If it succeeds, return 0,
 * else return a guess of the current time on the machine
 * holding the file.
 */

lock1(tempfile, name)
	char tempfile[], name[];
{
	register int fd;
	struct stat sbuf;

	fd = creat(tempfile, 0);
	if (fd < 0)
		return(time(0));
	fstat(fd, &sbuf);
	close(fd);
	if (link(tempfile, name) < 0) {
		remove(tempfile);
		return(sbuf.st_ctime);
	}
	remove(tempfile);
	return(0);
}

/*
 * Update the change time on the lock file so
 * others will know we're still using it.
 */

touchlock()
{
	struct stat sbuf;
	time_t t, tp[2];

	if (!locked)
		return;

	/* if it hasn't been at least 3 minutes, don't bother */
	if (time(&t) < locktime + 180)
		return;
	locktime = t;

	if (stat(curlock, &sbuf) < 0)
		return;
	/*
	 * Don't actually change the times, we just want the
	 * side effect that utime causes st_ctime to be set
	 * to the current time.
	 */
	tp[0] = sbuf.st_atime;
	tp[1] = sbuf.st_mtime;
	utime(curlock, tp);
}
