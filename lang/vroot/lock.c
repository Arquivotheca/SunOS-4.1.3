#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/errno.h>

extern	int		errno;
extern	char		*sys_errlist[];
extern	int		sys_nerr;

static	void		file_lock_error();

/*
 * This code stolen from the NSE library and changed to not depend
 * upon any NSE routines or header files.
 *
 * Simple file locking.
 * Create a symlink to a file.  The "test and set" will be
 * atomic as creating the symlink provides both functions.
 *
 * The timeout value specifies how long to wait for stale locks
 * to disappear.  If the lock is more than 'timeout' seconds old
 * then it is ok to blow it away.  This part has a small window
 * of vunerability as the operations of testing the time,
 * removing the lock and creating a new one are not atomic.
 * It would be possible for two processes to both decide to blow
 * away the lock and then have process A remove the lock and establish
 * its own, and then then have process B remove the lock which accidentily
 * removes A's lock rather than the stale one.
 *
 * A further complication is with the NFS.  If the file in question is
 * being served by an NFS server, then its time is set by that server.
 * We can not use the time on the client machine to check for a stale
 * lock.  Therefore, a temp file on the server is created to get
 * the servers current time.
 *
 * Returns an error message.  NULL return means the lock was obtained.
 *
 */
char *
file_lock(name, lockname, timeout)
	char		*name;
	int		timeout;
{
	int		r;
	int		fd;
	struct	stat	statb;
	struct	stat	fs_statb;
	char		tmpname[MAXPATHLEN];
	static	char	msg[MAXPATHLEN];

	if (timeout <= 0) {
		timeout = 15;
	}
	for (;;) {
		r = symlink(name, lockname);
		if (r == 0) {
			return NULL;
		}
		if (errno != EEXIST) {
			file_lock_error(msg, name, "symlink(%s, %s)",
			    name, lockname);
			return msg;
		}
		for (;;) {
			sleep(1);
			r = lstat(lockname, &statb);
			if (r == -1) {
				/*
				 * The lock must have just gone away - try 
				 * again.
				 */
				break;
			}

			/*
			 * With the NFS the time given a file is the
			 * time on the file server.  This time may
			 * vary from the client's time.  Therefore,
			 * we create a tmpfile in the same directory
			 * to establish the time on the server and
			 * use this time to see if the lock has expired.
			 */
			sprintf(tmpname, "%s.XXXXXX", lockname);
			mktemp(tmpname);
			fd = creat(tmpname, 0666);
			if (fd != -1) {
				close(fd);
			} else {
				file_lock_error(msg, name, "creat(%s)",
				    tmpname);
				return msg;
			}
			if (stat(tmpname, &fs_statb) == -1) {
				file_lock_error(msg, name, "stat(%s)",
				    tmpname);
				return msg;
			}
			unlink(tmpname);
			if (statb.st_mtime + timeout < fs_statb.st_mtime) {
				/*
				 * The lock has expired - blow it away.
				 */
				unlink(lockname);
				break;
			}
		}
	}
	/* NOTREACHED */
}

/*
 * Format a message telling why the lock could not be created.
 */
static	void
file_lock_error(msg, file, str, arg1, arg2)
	char		*msg;
	char		*file;
	char		*str;
{
	int		len;

	sprintf(msg, "Could not lock file `%s'; ", file);
	len = strlen(msg);
	sprintf(&msg[len], str, arg1, arg2);
	strcat(msg, " failed - ");
	if (errno < sys_nerr) {
		strcat(msg, sys_errlist[errno]);
	} else {
		len = strlen(msg);
		sprintf(&msg[len], "errno %d", errno);
	}
}
