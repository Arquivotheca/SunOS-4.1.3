#ifndef lint
static	char sccsid[] = "@(#)calendar.c 1.1 92/07/30 SMI"; /* from UCB 4.5 84/05/07 */
#endif
 
/* /usr/lib/calendar produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions
 
   used by calendar command
 
   Modified to take an argument so that it can serve a dual purpose. When
   no argument is present it generates the egrep file, when on is present
   it checks to see if it is readable and local and returns a exit status
   indicating such. Note : This makes calendar more network/NIS friendly
   but makes it dependent on how the UNIX implementation distinguishes
   local files from remote files.
*/
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vfs.h>
#define KERNEL
#include <sys/mount.h>
 
#define DAY (3600*24L)
 
char *month[] = {
	"[Jj]an",
	"[Ff]eb",
	"[Mm]ar",
	"[Aa]pr",
	"[Mm]ay",
	"[Jj]un",
	"[Jj]ul",
	"[Aa]ug",
	"[Ss]ep",
	"[Oo]ct",
	"[Nn]ov",
	"[Dd]ec"
};
struct tm *localtime();

tprint(t)
long t;
{
	struct tm *tm;
	tm = localtime(&t);
	printf("(^|[ \t(,;])(((%s[^ \t]*|\\*)[ \t]*|(0%d|%d|\\*)/)0*%d)([^0123456789]|$)\n",
		month[tm->tm_mon],
		tm->tm_mon + 1, tm->tm_mon + 1, tm->tm_mday);
}

main(argc,argv)

int     argc;
char    *argv[];
{
	long t;
	struct statfs   fsb;

	if (argc == 2)
	  exit(!(!(statfs(argv[1],&fsb)) && (fsb.f_fsid.val[1] == MOUNT_UFS)));
	time(&t);
	tprint(t);
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		tprint(t);
	case 6:
		t += DAY;
		tprint(t);
	default:
		t += DAY;
		tprint(t);
	}

	exit(0);
	/* NOTREACHED */
}
