#ifndef lint
static	char sccsid[] = "@(#)date.c 1.1 92/07/30 SMI"; /* from UCB 4.5 83/07/01 */
#endif

/*
 * Date - print and set date
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <syslog.h>
#include <utmp.h>
#include <sysexits.h>
#define WTMP "/var/adm/wtmp"

struct	timeval tv;
char	*ap, *ep, *sp;
int	uflag;

#define	MONTH	itoa(tp->tm_mon+1,cp,2)
#define	DAY	itoa(tp->tm_mday,cp,2)
#define	YEAR	itoa(tp->tm_year,cp,2)
#define	HOUR	itoa(tp->tm_hour,cp,2)
#define	MINUTE	itoa(tp->tm_min,cp,2)
#define	SECOND	itoa(tp->tm_sec,cp,2)
#define	JULIAN	itoa(tp->tm_yday+1,cp,3)
#define	WEEKDAY	itoa(tp->tm_wday,cp,1)
#define	MODHOUR	itoa(h,cp,2)

#define isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

char	month[12][3] = {
	"Jan","Feb","Mar","Apr",
	"May","Jun","Jul","Aug",
	"Sep","Oct","Nov","Dec"
};

char	days[7][3] = {
	"Sun","Mon","Tue","Wed",
	"Thu","Fri","Sat"
};

void	exit();
char	*itoa();

#ifdef S5EMUL
static char *usage = "usage: date [-a sss.fff] [-u] [+format] [mmddhhmm[yy]]\n";
#else
static char *usage = "usage: date [-a sss.fff] [-u] [+format] [yymmddhhmm[.ss]]\n";
#endif

struct utmp wtmp[2] = {
	{ "|", "", "", 0 },
	{ "{", "", "", 0 }
};

struct tm *gtime();
time_t timelocal(), timegm();
char    *username, *getlogin();

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *aptr, *cp, c;
	int  h, hflag, i;
	long lseek();
	register struct tm *tp;
	int days_in_month;
	char buf[200];
	int wf, rc;
	extern int optind;
	extern char *optarg;

	openlog("date", LOG_ODELAY, LOG_AUTH);
	rc = 0;
	gettimeofday(&tv, (struct timezone *)0);
	while ((i = getopt(argc, argv, "a:u")) != -1)
		switch(i) {
		case 'a':
			get_adj(optarg, &tv);
			if (adjtime(&tv, 0) < 0) {
				perror("date: Failed to adjust date");
				exit(EX_OSERR);
			}
			exit(0);

		case 'u':
			uflag++;
			continue;

		case '?':
			printf(usage);
			exit(EX_USAGE);
		}

	if (optind < argc && *argv[optind] == '+') {
		hflag = 0;
		for (cp = buf; cp < &buf[200];)
			*cp++ = '\0';
		if (uflag)
			tp = gmtime(&tv.tv_sec);
		else
			tp = localtime(&tv.tv_sec);
		aptr = argv[optind];
		aptr++;
		cp = buf;
		while (c = *aptr++) {
			if (c == '%')
				switch(*aptr++) {
				case '%':
					*cp++ = '%';
					continue;
				case 'n':
					*cp++ = '\n';
					continue;
				case 't':
					*cp++ = '\t';
					continue;
				case 'm':
					cp = MONTH;
					continue;
				case 'd':
					cp = DAY;
					continue;
				case 'y':
					cp = YEAR;
					continue;
				case 'D':
					cp = MONTH;
					*cp++ = '/';
					cp = DAY;
					*cp++ = '/';
					cp = YEAR;
					continue;
				case 'H':
					cp = HOUR;
					continue;
				case 'M':
					cp = MINUTE;
					continue;
				case 'S':
					cp = SECOND;
					continue;
				case 'T':
					cp = HOUR;
					*cp++ = ':';
					cp = MINUTE;
					*cp++ = ':';
					cp = SECOND;
					continue;
				case 'j':
					cp = JULIAN;
					continue;
				case 'w':
					cp = WEEKDAY;
					continue;
				case 'r':
					if ((h = tp->tm_hour) >= 12)
						hflag++;
					if ((h %= 12) == 0)
						h = 12;
					cp = MODHOUR;
					*cp++ = ':';
					cp = MINUTE;
					*cp++ = ':';
					cp = SECOND;
					*cp++ = ' ';
					if (hflag)
						*cp++ = 'P';
					else
						*cp++ = 'A';
					*cp++ = 'M';
					continue;
				case 'h':
					for (i=0; i<3; i++)
						*cp++ = month[tp->tm_mon][i];
					continue;
				case 'a':
					for (i=0; i<3; i++)
						*cp++ = days[tp->tm_wday][i];
					continue;
				default:
					(void) fprintf(stderr, "date: bad format character - %c\n", *--aptr);
					exit(EX_USAGE);
				}
			*cp++ = c;
		}
		*cp = '\0';
		printf("%s\n", buf);
		exit(0);
	}
	if (optind < argc) {
		username = getlogin();  /* single-user or no tty */
		if (username == NULL || *username == '\0')
			username = "root";
		ap = argv[optind];
		wtmp[0].ut_time = tv.tv_sec;
		if ((tp = gtime()) == NULL) {
			printf(usage);
			exit(EX_USAGE);
		}
		days_in_month = dmsize[tp->tm_mon];
		if (tp->tm_mon == 1 && isleap(tp->tm_year + 1900))
			days_in_month = 29;	/* February in leap year */
		if (tp->tm_mon < 0 || tp->tm_mon > 11 ||
		    tp->tm_mday < 1 || tp->tm_mday > days_in_month ||
		    tp->tm_hour < 0 || tp->tm_hour > 23 ||
		    tp->tm_min < 0 || tp->tm_min > 59 ||
		    tp->tm_sec < 0 || tp->tm_sec > 59) {
			printf(usage);
			exit(EX_USAGE);
		}
		if (uflag == 0)
			/* convert to local time */
			tv.tv_sec = timelocal(tp);
		else
			/* convert to GMT */
			tv.tv_sec = timegm(tp);
		if (settimeofday(&tv, (struct timezone *)0) < 0) {
			rc++;
			perror("date: Failed to set date");
			exit(EX_NOPERM);
		} else if ((wf = open(WTMP, 1)) >= 0) {
			time(&wtmp[1].ut_time);
			lseek(wf, 0L, 2);
			write(wf, (char *)wtmp, sizeof(wtmp));
			close(wf);
		}
		syslog(LOG_NOTICE, "set by %s", username);
	}
	if (rc == 0)
		gettimeofday(&tv, (struct timezone *)0);
	if (uflag)
		tp = gmtime(&tv.tv_sec);
	else
		tp = localtime(&tv.tv_sec);
	ap = asctime(tp);
	printf("%.20s", ap);
	if (tp->tm_zone != NULL)
		printf("%s", tp->tm_zone);
	printf("%s", ap+19);
	exit(rc);
	/* NOTREACHED */
}

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
	if (*ap == 'p')
		newtime.tm_hour += 12;
	return (&newtime);
}

gpair()
{
	register int c, d;
	register char *cp;

	cp = ap;
	if (*cp == 0)
		return (-1);
	c = (*cp++ - '0') * 10;
	if (c < 0 || c > 100)
		return (-1);
	if (*cp == 0)
		return (-1);
	if ((d = *cp++ - '0') < 0 || d > 9)
		return (-1);
	ap = cp;
	return (c+d);
}

#else

struct tm *
gtime()
{
	static struct tm newtime;
	register struct tm *L;
	register char x;

	ep = ap;
	while(*ep) ep++;
	sp = ap;
	while(sp < ep) {
		x = *sp;
		*sp++ = *--ep;
		*ep = x;
	}
	sp = ap;
	gettimeofday(&tv, (struct timezone *)0);
	L = localtime(&tv.tv_sec);
	newtime.tm_sec = gp(-1);
	if (*sp != '.') {
		newtime.tm_min = newtime.tm_sec;
		newtime.tm_sec = 0;
	} else {
		sp++;
		newtime.tm_min = gp(-1);
	}
	newtime.tm_hour = gp(-1);
	newtime.tm_mday = gp(L->tm_mday);
	newtime.tm_mon = gp(L->tm_mon + 1) - 1;
	newtime.tm_year = gp(L->tm_year);
	if (*sp)
		return (NULL);
	if (newtime.tm_hour == 24) {
		newtime.tm_hour = 0;
		newtime.tm_mday++;
	}
	return (&newtime);
}

gp(dfault)
{
	register int c, d;

	if (*sp == 0)
		return (dfault);
	c = (*sp++) - '0';
	d = (*sp ? (*sp++) - '0' : 0);
	if (c < 0 || c > 9 || d < 0 || d > 9)
		return (-1);
	return (c+10*d);
}

#endif

char *
itoa(i,ptr,dig)
register  int	i;
register  int	dig;
register  char	*ptr;
{
	switch(dig)	{
		case 3:
			*ptr++ = i/100 + '0';
			i = i - i / 100 * 100;
		case 2:
			*ptr++ = i / 10 + '0';
		case 1:	
			*ptr++ = i % 10 + '0';
	}
	return (ptr);
}

get_adj(cp, tp)
	char *cp;
	struct timeval *tp;
{
	register int mult;
	int sign;

	tp->tv_sec = tp->tv_usec = 0;
	if (*cp == '-') {
		sign = -1;
		cp++;
	} else {
		sign = 1;
	}
	while (*cp >= '0' && *cp <= '9') {
		tp->tv_sec *= 10;
		tp->tv_sec += *cp++ - '0';
	}
	if (*cp == '.') {
		cp++;
		mult = 100000;
		while (*cp >= '0' && *cp <= '9') {
			tp->tv_usec += (*cp++ - '0') * mult;
			mult /= 10;
		}
	}
	if (*cp) {
		printf(usage);
		exit(EX_USAGE);
	}
	tp->tv_sec *= sign;
	tp->tv_usec *= sign;
}
