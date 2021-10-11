#ifndef lint
static	char sccsid[] = "@(#)atfuncs.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/fcntl.h>
#include "cron.h"

#define CANTCD		"can't change directory to the at directory"
#define NOREADDIR	"can't read the at directory"

int
getjoblist(namelistp, statlistp,sortfunc)
	struct direct ***namelistp;
	struct stat ***statlistp;
	int (*sortfunc)();
{
	register int numjobs;
	register struct direct **namelist;
	register int i;
	register struct stat *statptr;	/* pointer to file stat structure */
	register struct stat **statlist;
	extern int alphasort();		/* sort jobs by date of execution */
	static int filewanted();	/* should a file be listed in queue? */

	if (chdir(ATDIR) < 0)
		atabortperror(CANTCD);

	/*
	 * Get a list of the files in the spooling area.
	 */
	if ((numjobs = scandir(".",namelistp,filewanted,sortfunc)) < 0)
		atabortperror(NOREADDIR);

	if ((statlist = (struct stat **) malloc(numjobs * sizeof (struct stat ***))) == NULL)
		atabort("Out of memory");

	namelist = *namelistp;

	/*
	 * Build an array of pointers to the file stats for all jobs in
	 * the spooling area.
	 */
	for (i = 0; i < numjobs; ++i) { 
		statptr = (struct stat *) malloc(sizeof(struct stat));
		if (statptr == NULL)
			atabort("Out of memory");
		if (stat(namelist[i]->d_name, statptr) < 0) {
			atperror("Can't stat", namelist[i]->d_name);
			continue;
		}
		statlist[i] = statptr;
	}

	*statlistp = statlist;
	return (numjobs);
}

/*
 * Do we want to include a file in the queue? (used by "scandir")
 */
int
filewanted(direntry)
	struct direct *direntry;
{
	char *p;
	char *q;
	register char c;
	extern time_t num();

	p = direntry->d_name;
	(void) num(&p);
	if (p == direntry->d_name)
		return (0);	/* didn't start with a number */
	if (*p++ != '.')
		return (0);	/* followed by a period */
	c = *p++;
	if (c < 'a' || c > 'z')
		return (0);	/* followed by a queue name */
	if (*p++ != '.')
		return (0);	/* followed by a period */
	q = p;
	(void) strtol(p, &p, 10);
	if (p == q)
		return (0);	/* followed by a number */
	if (*p++ != '\0')
		return (0);	/* followed by nothing */
	return (1);
}

/*
 * Sort files by queue, time of creation, and sequence. (used by "scandir")
 */
creation(d1, d2)
struct direct **d1, **d2;
{
	register char *p1, *p2;
	register int i;
	struct stat stbuf1, stbuf2;
	register int seq1, seq2;

	if ((p1 = index((*d1)->d_name, '.')) == NULL)
		return(0);
	if ((p2 = index((*d2)->d_name, '.')) == NULL)
		return(0);
	p1++;
	p2++;
	if ((i = *p1++ - *p2++) != 0)
		return(i);

	if (stat((*d1)->d_name,&stbuf1) < 0)
		return(0);

	if (stat((*d2)->d_name,&stbuf2) < 0)
		return(0);

	if (stbuf1.st_ctime < stbuf2.st_ctime)
		return(-1);
	else if (stbuf1.st_ctime > stbuf2.st_ctime)
		return(1);
	p1++;
	p2++;
	seq1 = atoi(p1);
	seq2 = atoi(p2);
	return(seq1 - seq2);
}
	
/*
 * Sort files by queue, time of execution, and sequence. (used by "scandir")
 */
execution(d1, d2)
struct direct **d1, **d2;
{
	register char *p1, *p2;
	register int i;
	char *name1, *name2;
	register time_t time1, time2;
	register int seq1, seq2;

	name1 = (*d1)->d_name;
	name2 = (*d2)->d_name;
	if ((p1 = index(name1, '.')) == NULL)
		return(1);
	if ((p2 = index(name2, '.')) == NULL)
		return(1);
	p1++;
	p2++;
	if ((i = *p1++ - *p2++) != 0)
		return(i);

	time1 = num(&name1);
	time2 = num(&name2);

	if (time1 < time2)
		return(-1);
	else if (time1 > time2)
		return(1);
	p1++;
	p2++;
	seq1 = atoi(p1);
	seq2 = atoi(p2);
	return(seq1 - seq2);
}

sendmsg(action,login,fname)
	char action;
	char *fname;
{
	static int msgfd = -2;
	struct message *pmsg;
	register int i;

	pmsg = &msgbuf;
	if (msgfd == -2) {
		if ((msgfd = open(FIFO,O_WRONLY|O_NDELAY)) < 0) {
			if (errno == ENXIO || errno == ENOENT)
				aterror("cron may not be running - call your system administrator\n");
			else
				atperror("error in message queue open");
			return;
		}
	}
	pmsg->etype = AT;
	pmsg->action = action;
	(void) strncpy(pmsg->fname, fname, FLEN);
	(void) strncpy(pmsg->logname, login, LLEN);
	if ((i = write(msgfd, pmsg, sizeof(struct message))) < 0)
		atperror("error in message send");
	else if (i != sizeof(struct message))
		aterror("error in message send: Premature EOF\n");
}
