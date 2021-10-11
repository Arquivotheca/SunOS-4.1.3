#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ttyname.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
/*
 * ttyname(f): return "/dev/ttyXX" which the the name of the
 * tty belonging to file f.
 *
 * This program works in two passes: the first pass tries to
 * find the device by matching device and inode numbers; if
 * that doesn't work, it tries a second time, this time doing a
 * stat on every file in /dev and trying to match device numbers
 * only. If that fails too, NULL is returned.
 */

#define	NULL	0
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>

extern int stat(), fstat(), isatty();
extern char *strcpy(), *strcat();

/* should be malloc()'d, but not until vi, etc fixed. */
static char rbuf[6+64], *dev="/dev/";

char *
ttyname(f)
int	f;
{
	struct stat fsb, tsb;
	register struct direct *db;
	register DIR *df;
	register int pass1;

	if(isatty(f) == 0)
		return(NULL);
	if(fstat(f, &fsb) < 0)
		return(NULL);
	if((fsb.st_mode & S_IFMT) != S_IFCHR)
		return(NULL);
	if((df = opendir(dev)) == NULL)
		return(NULL);
	pass1 = 1;
	do {
		while((db = readdir(df)) != NULL) {
			if(pass1 && db->d_ino != fsb.st_ino)
				continue;
			if (strlen(dev)+strlen(db->d_name) >  sizeof (rbuf)) {
				closedir(df);
				return (0);
			}
			(void) strcpy(rbuf, dev);
			(void) strcat(rbuf, db->d_name);
			if(stat(rbuf, &tsb) < 0)
				continue;
			if(tsb.st_rdev == fsb.st_rdev &&
			    (tsb.st_mode&S_IFMT) == S_IFCHR &&
			    (!pass1 || (tsb.st_dev == fsb.st_dev &&
					tsb.st_ino == fsb.st_ino))) {
				closedir(df);
				return(rbuf);
			}
		}
		rewinddir(df);
	} while(pass1--);
	closedir(df);
	return(NULL);
}
