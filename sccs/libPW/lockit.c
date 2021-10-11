#ifndef lint
static	char sccsid[] = "@(#)lockit.c 1.1 92/07/30 SMI"; /* from System III 3.5 */
#endif

/*
	Process semaphore.
	Try repeatedly (`count' times) to create `lockfile' mode 444
	with the O_EXCL flag (so that if it already exists the create
	will fail).
	Sleep 10 seconds between tries.
	If `tempfile' is successfully created, write the process ID
	`pid' in `tempfile' (in binary), link `tempfile' to `lockfile',
	and return 0.
	If `lockfile' exists and it hasn't been modified within the last
	minute, and either the file is empty or the process ID contained
	in the file is not the process ID of any existing process,
	`lockfile' is removed and it tries again to make `lockfile'.
	After `count' tries, or if the reason for the create failing
	is something other than EEXIST, return xmsg().
 
	Unlockit will return 0 if the named lock exists, contains
	the given pid, and is successfully removed; -1 otherwise.
*/

# include	<sys/types.h>
# include	<sys/param.h>
# include	<sys/fcntl.h>
# include	"../hdr/macros.h"
# include	<errno.h>

lockit(lockfile,count,pid)
register char *lockfile;
register unsigned count;
unsigned pid;
{
	register int fd;
	int ret;
	unsigned opid;
	long ltime;
	long omtime;
	extern int errno;
	static char tempfile[MAXPATHLEN];
	char	dir_name[MAXPATHLEN];

	copy(lockfile,dir_name);
	sprintf(tempfile,"%s/%u.%ld",dname(dir_name),pid,time((long *)0));

	for (++count; --count; sleep(10)) {
		if (onelock(pid,tempfile,lockfile) == 0)
			return(0);
		if (!exists(lockfile))
			continue;
		omtime = Statbuf.st_mtime;
		if ((fd = open(lockfile,0)) < 0)
			continue;
		ret = read(fd,&opid,sizeof(opid));
		close(fd);
		if (ret != sizeof(pid) || ret != Statbuf.st_size) {
			unlink(lockfile);
			continue;
		}
		/* check for pid */
		if (kill(opid,0) == -1 && errno == ESRCH) {
			if (exists(lockfile) &&
				omtime == Statbuf.st_mtime) {
					unlink(lockfile);
					continue;
			}
		}
		if ((ltime = time((long *)0) - Statbuf.st_mtime) < 60L) {
			if (ltime >= 0 && ltime < 60)
				sleep(60 - ltime);
			else
				sleep(60);
		}
		continue;
	}
	return(-1);
}


unlockit(lockfile,pid)
register char *lockfile;
unsigned pid;
{
	register int fd, n;
	unsigned opid;

	if ((fd = open(lockfile,0)) < 0)
		return(-1);
	n = read(fd,&opid,sizeof(opid));
	close(fd);
	if (n == sizeof(opid) && opid == pid)
		return(unlink(lockfile));
	else
		return(-1);
}


onelock(pid,tempfile,lockfile)
unsigned pid;
char *tempfile;
char *lockfile;
{
	int	fd;
	extern int errno;

	if ((fd = open(tempfile,O_WRONLY|O_CREAT|O_EXCL,0444)) >= 0) {
		write(fd,&pid,sizeof(pid));
		close(fd);
		if (link(tempfile,lockfile) < 0) {
			unlink(tempfile);
			return(-1);
		}
		unlink(tempfile);
		return(0);
	}
	if (errno == ENFILE) {
		unlink(tempfile);
		return(-1);
	}
	if (errno != EEXIST)
		return(xmsg(tempfile,"lockit"));
	return(-1);
}


mylock(lockfile,pid)
register char *lockfile;
unsigned pid;
{
	register int fd, n;
	unsigned opid;

	if ((fd = open(lockfile,0)) < 0)
		return(0);
	n = read(fd,&opid,sizeof(opid));
	close(fd);
	if (n == sizeof(opid) && opid == pid)
		return(1);
	else
		return(0);
}
