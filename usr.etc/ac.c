#ifndef lint
static	char *sccsid = "@(#)ac.c 1.1 92/07/30 SMI"; /* from UCB 4.7 */
#endif
/*
 * ac [ -w wtmp ] [ -d ] [ -p ] [ people ]
 */

#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#define NMAX sizeof(ibuf.ut_name)
#define LMAX sizeof(ibuf.ut_line)

/*
#define	TSIZE	1000
*/
#define TSIZE  6242
#define	USIZE	500
struct  utmp ibuf;

struct ubuf {
	char	uname[NMAX];
	long	utime;
} ubuf[USIZE];

struct tbuf {
	struct	ubuf	*userp;
	long	ttime;
} tbuf[TSIZE];

char	*wtmp;
int	pflag, byday;
long	dtime;
long	midnight;
long	lastime;
long	day	= 86400L;
int	pcount;
char	**pptr;

main(argc, argv) 
char **argv;
{
	int c, fl;
	register i;
	FILE *wf;

	wtmp = "/var/adm/wtmp";
	while (--argc > 0 && **++argv == '-')
	switch(*++*argv) {
	case 'd':
		byday++;
		continue;

	case 'w':
		if (--argc>0)
			wtmp = *++argv;
		continue;

	case 'p':
		pflag++;
		continue;
	}
	pcount = argc;
	pptr = argv;
	if ((wf = fopen(wtmp, "r")) == NULL) {
		printf("No %s\n", wtmp);
		exit(1);
	}
	for(;;) {
		if (fread((char *)&ibuf, sizeof(ibuf), 1, wf) != 1)
			break;
		fl = 0;
		for (i=0; i<NMAX; i++) {
			c = ibuf.ut_name[i];
			if (isprint(c) && c != ' ') {
				if (fl)
					goto skip;
				continue;
			}
			if (c==' ' || c=='\0') {
				fl++;
				ibuf.ut_name[i] = '\0';
			} else
				goto skip;
		}
		loop();
    skip:;
	}
	ibuf.ut_name[0] = '\0';
	ibuf.ut_line[0] = '~';
	time(&ibuf.ut_time);
	loop();
	print();
	exit(0);
	/* NOTREACHED */
}

loop()
{
	register i;
	register struct tbuf *tp;
	register struct ubuf *up;

	if(ibuf.ut_line[0] == '|') {
		dtime = ibuf.ut_time;
		return;
	}
	if(ibuf.ut_line[0] == '{') {
		if(dtime == 0)
			return;
		for(tp = tbuf; tp < &tbuf[TSIZE]; tp++)
			tp->ttime += ibuf.ut_time-dtime;
		dtime = 0;
		return;
	}
	if (lastime>ibuf.ut_time || lastime+(1.5*day)<ibuf.ut_time)
		midnight = 0;
	if (midnight==0)
		newday();
	lastime = ibuf.ut_time;
	if (byday && ibuf.ut_time > midnight) {
		upall(1);
		print();
		newday();
		for (up=ubuf; up < &ubuf[USIZE]; up++)
			up->utime = 0;
	}
	if (ibuf.ut_line[0] == '~') {
		ibuf.ut_name[0] = '\0';
		upall(0);
		return;
	}
	/*
	if (ibuf.ut_line[0]=='t')
		i = (ibuf.ut_line[3]-'0')*10 + (ibuf.ut_line[4]-'0');
	else
		i = TSIZE-1;
	if (i<0 || i>=TSIZE)
		i = TSIZE-1;
	*/

	/*
	 * Correction contributed by Phyllis Kantar @ Rand-unix
	 *
	 * Fixes long standing problem with tty names other than 00-99
	 */
	if (ibuf.ut_line[0]=='t') {
		i = (ibuf.ut_line[3]-'0');
		if(ibuf.ut_line[4])
			i = i*79 + (ibuf.ut_line[4]-'0');
	} else
		i = TSIZE-1;
	if (i<0 || i>=TSIZE) {
		i = TSIZE-1;
		printf("ac: Bad tty name: %s\n", ibuf.ut_line);
	}

	tp = &tbuf[i];
	update(tp, 0);
}

print()
{
	int i;
	long ttime, t;

	ttime = 0;
	for (i=0; i<USIZE; i++) {
		if(!among(i))
			continue;
		t = ubuf[i].utime;
		if (t>0)
			ttime += t;
		if (pflag && ubuf[i].utime > 0) {
			printf("\t%-*.*s%6.2f\n", NMAX, NMAX,
			    ubuf[i].uname, ubuf[i].utime/3600.);
		}
	}
	if (ttime > 0) {
		pdate();
		printf("\ttotal%9.2f\n", ttime/3600.);
	}
}

upall(f)
{
	register struct tbuf *tp;

	for (tp=tbuf; tp < &tbuf[TSIZE]; tp++)
		update(tp, f);
}

update(tp, f)
struct tbuf *tp;
{
	int j;
	struct ubuf *up;
	long t, t1;

	if (f)
		t = midnight;
	else
		t = ibuf.ut_time;
	if (tp->userp) {
		t1 = t - tp->ttime;
		if (t1 > 0)
			tp->userp->utime += t1;
	}
	tp->ttime = t;
	if (f)
		return;
	if (ibuf.ut_name[0]=='\0') {
		tp->userp = 0;
		return;
	}
	for (up=ubuf; up < &ubuf[USIZE]; up++) {
		if (up->uname[0] == '\0')
			break;
		for (j=0; j<NMAX && up->uname[j]==ibuf.ut_name[j]; j++);
		if (j>=NMAX)
			break;
	}
	for (j=0; j<NMAX; j++)
		up->uname[j] = ibuf.ut_name[j];
	tp->userp = up;
}

among(i)
{
	register j, k;
	register char *p;

	if (pcount==0)
		return(1);
	for (j=0; j<pcount; j++) {
		p = pptr[j];
		for (k=0; k<NMAX; k++) {
			if (*p == ubuf[i].uname[k]) {
				if (*p++ == '\0' || k == NMAX-1)
					return(1);
			} else
				break;
		}
	}
	return(0);
}

/*
 * Given the time "ibuf.ut_time", compute the time of the next midnight in
 * the future.  Do this by adding one day to "ibuf.ut_time", converting it
 * to a "struct tm", and cranking that "struct tm" back to the midnight of
 * that day.
 */
newday()
{
	long ttime;
	struct tm midnight_tm;
	extern struct tm *localtime();
	extern time_t timelocal();

	ttime = ibuf.ut_time + day;
	midnight_tm = *localtime(&ttime);
	midnight_tm.tm_hour = 0;
	midnight_tm.tm_min = 0;
	midnight_tm.tm_sec = 0;
	midnight = timelocal(&midnight_tm);
}

pdate()
{
	long x;
	char *ctime();

	if (byday==0)
		return;
	x = midnight-1;
	printf("%.6s", ctime(&x)+4);
}
