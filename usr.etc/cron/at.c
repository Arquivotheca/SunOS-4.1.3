/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)at.c 1.1 92/07/30 SMI"; /* from S5R3 1.12 */
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include "cron.h"

#define TMPFILE		"_at"	/* prefix for temporary files	*/
#define ATMODE		06444	/* Mode for creating files in ATDIR.
Setuid bit on so that if an owner of a file gives that file 
away to someone else, the setuid bit will no longer be set.  
If this happens, atrun will not execute the file	*/
#define ROOT		0	/* user-id of super-user */
#define LINSIZ		256	/* length of line buffer */
#define BUFSIZE		512	/* for copying files */
#define LINESIZE	130	/* for listing jobs */
#define	MAXTRYS		100	/* max trys to create at job file */

#define BADDATE		"bad date specification"
#define BADTIME		"bad time specification"
#define BADFIRST	"bad first argument"
#define BADHOURS	"hours field is too large"
#define BAD12HR		"hours field is invalid for a 12-hour clock"
#define BADMD		"bad month and day specification"
#define BADMINUTES	"minutes field is too large"
#define CANTCD		"can't change directory to the at directory"
#define CANTCHOWN	"can't change the owner of your job to you"
#define CANTCHMOD	"can't change the mode of your job"
#define CANTCREATE	"can't create a job for you"
#define INVALIDUSER	"you are not a valid user (no entry in /etc/passwd)"
#define NOTALLOWED	"you are not authorized to use at.  Sorry."
#define NOTHING		"nothing specified"
#define PAST		"it's past that time"

/*
	this data is used for parsing time
*/
#define	dysize(A)	(((A)%4) ? 365 : 366)
int	gmtflag = 0;
int	dflag = 0;
char	login[UNAMESIZE];
char	pname[80];
char	pname1[80];
char	argbuf[80];
time_t	when, timelocal(), timegm();
struct	timeval now;
struct	tm	*tp, at, rt;
char	*parsedtime();
char	*gethhmm();
int	lookup_token();
char	*convert_to_num();
int	mday[12] =
{
	31,38,31,
	30,31,30,
	31,31,30,
	31,30,31,
};
int	mtab[12] =
{
	0,   31,  59,
	90,  120, 151,
	181, 212, 243,
	273, 304, 334,
};
int     dmsize[12] = {
	31,28,31,30,31,30,31,31,30,31,30,31};

/* end of time parser */

short	jobtype = ATEVENT;		/* set to 1 if batch job */
int	user;
char	*tfname;
extern char *malloc();
extern char *xmalloc();
extern void  exit();
extern char *errmsg();
extern FILE *fopen_configfile();

int  shflag, cshflag, mailflag;

main(argc,argv)
char **argv;
{
	struct passwd *pw;
	int fd,catch();
	struct stat buf;
	char *ptr,*job,pid[6];
	register char *pp;
	register char c;
	char *mkjobname(),*getuser();
	int alphasort();
	int execution();
	time_t t, time();
	int  st = 1;
	char *jobfile = NULL;	/* file containing job to be run */
	FILE *spoolfile;
	FILE *inputfile;
	int numjobs;
	struct direct **namelist;	/* names of jobs in spooling area */
	struct stat **statlist;	/* stat information on those jobs */
	register int i, j;
	int jobnum;
	int jobexists;

	pp = getuser((user=getuid()));
	if(pp == NULL)
		atabort(INVALIDUSER);
	strcpy(login,pp);
	if (!allowed(login,ATALLOW,ATDENY)) atabort(NOTALLOWED);

	if (argc < 2)
		usage();

	if (strcmp(argv[1],"-r")==0) {
		/* remove jobs that are specified */
		if (argc < 3)
			usage();
		numjobs = getjoblist(&namelist,&statlist,alphasort);
		for (i=2; i<argc; i++) {
			ptr = argv[i];
			jobnum = strtol(ptr, &ptr, 10);
			if (jobnum <= 0 || *ptr != '\0') {
				fprintf(stderr,"at: invalid job name %s\n",argv[i]);
				continue;
			}
			jobexists = 0;
			for (j = 0; j < numjobs; ++j) {
				/*
				 * If the inode number is 0, this entry
				 * was removed.
				 */
				if (statlist[j]->st_ino == 0)
					continue;
				/*
				 * The argument is a job #, so we compare it
				 * to the inode (job #) of the file.
				 */
				if (statlist[j]->st_ino == jobnum)
					break;
			}
			if (j != numjobs) {
				++jobexists;
				if ((user!=statlist[j]->st_uid) && (user!=ROOT))
					fprintf(stderr,"you don't own %s\n",argv[i]);
				else {
					sendmsg(DELETE,login,
					    namelist[j]->d_name);
					unlink(namelist[j]->d_name);
					/*
					 * If the entry is ultimately removed,
					 * don't try to remove it again later.
					 */
					statlist[j]->st_ino = 0;
				}
			}
			/*
			 * If a requested argument doesn't exist, print a
			 * message.
			 */
			if (!jobexists)
				fprintf(stderr,"at: %s does not exist\n",
				    argv[i]);
		}
		exit(0); 
	}

	if (strncmp(argv[1],"-l",2)==0) {
		/* list jobs for user */
		numjobs = getjoblist(&namelist,&statlist,execution);
		if (argc==2) {
			/* list all jobs for a user */
			for (j = 0; j < numjobs; ++j) {
				if ((user!=ROOT) && (statlist[j]->st_uid!=user))
					continue;
				ptr = namelist[j]->d_name;
				t = num(&ptr);
				if ((user==ROOT) && ((pw=getpwuid(statlist[j]->st_uid))!=NULL))
					printf("user = %s\t%10d %c\t%s",
					    pw->pw_name,
					    statlist[j]->st_ino,
					    *(strchr(namelist[j]->d_name, '.') + 1),
					    asctime(localtime(&t)));
				else
					printf("%10d %c\t%s",
					    statlist[j]->st_ino,
					    *(strchr(namelist[j]->d_name, '.') + 1),
					    asctime(localtime(&t)));
			}
		} else {
			/* list particular jobs for user */
			for (i=2; i<argc; i++) {
				ptr = argv[i];
				jobnum = strtol(ptr, &ptr, 10);
				if (jobnum <= 0 || *ptr != '\0') {
					fprintf(stderr,"at: invalid job name %s\n",argv[i]);
					continue;
				}
				jobexists = 0;
				for (j = 0; j < numjobs; ++j) {
					/*
					 * The argument is a job #, so we
					 * compare it to the inode (job #)
					 * of the file.
					 */
					if (statlist[j]->st_ino == jobnum)
						break;
				}
				if (j != numjobs) {
					++jobexists;
					ptr = namelist[j]->d_name;
					t = num(&ptr);
					if ((user!=statlist[j]->st_uid) && (user!=ROOT))
						fprintf(stderr,"at: you don't own %s\n",argv[i]);
					else 
						printf("%10d %c\t%s",
						    statlist[j]->st_ino,
						    *(strchr(namelist[j]->d_name, '.') + 1),
						    asctime(localtime(&t)));
				}
			}
			/*
			 * If a requested argument doesn't exist, print a
			 * message.
			 */
			if (!jobexists)
				fprintf(stderr,"at: %s does not exist\n",
				    argv[i]);
		}
		exit(0);
	}

	for(i=1; i<argc && argv[i][0] == '-'; i++) {
		if(strncmp(argv[i], "-q",2) == 0) {
			if(argv[i][2] == NULL)
				atabort("no queue specified");
			if (argv[i][2] < 'a' || argv[i][2] > 'z')
				atabort("illegal queue specified");
			jobtype = argv[i][2] - 'a';
			if (jobtype == CRONEVENT)
				atabort("queue 'c' may not be specified");
		} else {
			pp = &argv[i][1];
			while ((c = *pp++) != '\0') {
				switch (c) {

				case 'c':
					cshflag++;
					break;

				case 's':
					shflag++;
					break;

				case 'm':
					mailflag++;
					break;

				default:
					usage();
					break;
				}
			}
		}
	}

	if (shflag && cshflag)
		atabort("ambiguous shell request");

	st = i;

	/* figure out what time to run the job */

	if(st == argc && jobtype != BATCHEVENT)
		atabort(NOTHING);
	gettimeofday(&now, (struct timezone *)NULL);
	if(jobtype != BATCHEVENT) {	/* at job */
		tp = localtime(&now.tv_sec);
		mday[1] = 28 + leap(tp->tm_year);
		jobfile = parsedtime(st, argc, argv);
		atime(&at, &rt);
		if (gmtflag)
			when = timegm(&at);
		else
			when = timelocal(&at);
	} else {		/* batch job */
		when = now.tv_sec;
		jobfile = argv[st];
	}
	if(when < now.tv_sec)	/* time has already past */
		atabort("too late");

	sprintf(pid,"%-5d",getpid());
	tfname=xmalloc(strlen(ATDIR)+strlen(TMPFILE)+7);
	strcat(strcat(strcat(strcpy(tfname,ATDIR),"/"),TMPFILE),pid);
	/* catch SIGINT, HUP, and QUIT signals */
	if (signal(SIGINT, catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP, catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if((fd = open(tfname,O_CREAT|O_EXCL|O_WRONLY,ATMODE)) < 0)
		atabortperror(CANTCREATE);
	if (fchown(fd,user,getgid())==-1) {
		unlink(tfname);
		atabortperror(CANTCHOWN);
	}
	if((spoolfile = fdopen(fd, "w")) == NULL)
		atabortperror(CANTCREATE);
	sprintf(pname,"%s",PROTO);
	sprintf(pname1,"%s.%c",PROTO,'a'+jobtype);

	/*
	 * Open the input file with the user's permissions.
	 */
	if (jobfile != NULL) {
		(void) seteuid(user);
		if ((inputfile = fopen(jobfile, "r")) == NULL) {
			(void) seteuid(ROOT);
			unlink(tfname);
			atabortperror(jobfile);
			exit(1);
		}
		(void) seteuid(ROOT);
	} else {
		jobfile = "stdin";
		inputfile = stdin;
	}
		
	copy(jobfile, inputfile, spoolfile);
	if (rename(tfname,job=mkjobname(when))==-1) {
		unlink(tfname);
		atabortperror(CANTCREATE);
	}
	sendmsg(ADD,login,strrchr(job,'/')+1);
	if (stat(job,&buf)==-1)
		atabort("Job disappeared - couldn't stat!");
	fprintf(stderr,"job %ld at %.24s\n",buf.st_ino,ctime(&when));
	if (when-t-MINUTE < HOUR) fprintf(stderr,
	    "at: this job may not be executed at the proper time.\n");
	exit(0);
	/* NOTREACHED */
}

/*
 * Print usage info and exit.
 */
usage()
{
	fprintf(stderr,"usage: at [-csm] [-q<queue>] time [date] [filename]\n");
	fprintf(stderr,"       at -l\n");
	fprintf(stderr,"       at -r jobs\n");
	exit(1);
}

struct tokentable {
	char	*tt_name;
	int	tt_value;
};

#define	NOON		0
#define	MIDNIGHT	1
#define	NOW		2

struct tokentable timetokens[] = {
	{ "n",		NOON },
	{ "noon",	NOON },
	{ "m",		MIDNIGHT },
	{ "midnight",	MIDNIGHT },
	{ "now",	NOW },
	{ NULL,		-1 },
};

#define	AM	3
#define	PM	4
#define	ZULU	5

struct tokentable ampm[] = {
	{ "n",		NOON },
	{ "noon",	NOON },
	{ "m",		MIDNIGHT },
	{ "midnight",	MIDNIGHT },
	{ "a",		AM },
	{ "am",		AM },
	{ "p",		PM },
	{ "P",		PM },
	{ "pm",		PM },
	{ "zulu",	ZULU },
	{ NULL,		-1 }
};

struct tokentable months[] = {
	{ "jan",	0 },
	{ "january",	0 },
	{ "feb",	1 },
	{ "february",	1 },
	{ "mar",	2 },
	{ "march",	2 },
	{ "apr",	3 },
	{ "april",	3 },
	{ "may",	4 },
	{ "jun",	5 },
	{ "june",	5 },
	{ "jul",	6 },
	{ "july",	6 },
	{ "aug",	7 },
	{ "august",	7 },
	{ "sep",	8 },
	{ "september",	8 },
	{ "oct",	9 },
	{ "october",	9 },
	{ "nov",	10 },
	{ "november",	10 },
	{ "dec",	11 },
	{ "december",	11 },
	{ NULL,		-1 }
};

struct tokentable days_of_week[] = {
	{ "sun",	0 },
	{ "sunday",	0 },
	{ "mon",	1 },
	{ "monday",	1 },
	{ "tue",	2 },
	{ "tuesday",	2 },
	{ "wed",	3 },
	{ "wednesday",	3 },
	{ "thu",	4 },
	{ "thursday",	4 },
	{ "fri",	5 },
	{ "friday",	5 },
	{ "sat",	6 },
	{ "saturday",	6 },
	{ "today",	7 },
	{ "tomorrow",	8 },
	{ NULL,		-1 }
};

#define	MINUTES		0
#define	HOURS		1
#define	DAYS		2
#define	WEEKS		3
#define	MONTHS		4
#define	YEARS		5

struct tokentable units[] = {
	{ "min",	MINUTES },
	{ "mins",	MINUTES },
	{ "minute",	MINUTES },
	{ "minutes",	MINUTES },
	{ "hour",	HOURS },
	{ "hours",	HOURS },
	{ "day",	DAYS },
	{ "days",	DAYS },
	{ "week",	WEEKS },
	{ "weeks",	WEEKS },
	{ "month",	MONTHS },
	{ "months",	MONTHS },
	{ "year",	YEARS },
	{ "years",	YEARS },
	{ NULL,		-1 },
};

char *
parsedtime(st, argc, argv)
	int st;
	int argc;
	char **argv;
{
	register int i;
	register char *argp;
	register int suffix;
	register int saw_date;
	int month, dow;
	int incr;

	if (st == argc)
		atabort("no date/time specified");
	saw_date = 0;
	i = st;
	argp = argv[i++];
	switch (lookup_token(argp, timetokens)) {

	case NOON:
		at.tm_hour = 12;
		break;

	case MIDNIGHT:
		at.tm_hour = 0;
		break;

	case NOW:
		at.tm_hour = tp->tm_hour;
		at.tm_min = tp->tm_min;
		break;

	default:
		argp = gethhmm(argp, &at.tm_hour, &at.tm_min);
		if (*argp == '\0') {
			if (i == argc) {
				argp = NULL;
				goto found_filename;
			}
			argp = argv[i++];
			suffix = 1;
		} else
			suffix = 0;
		switch (lookup_token(argp, ampm)) {

		case AM:
			if (at.tm_hour < 1 || at.tm_hour > 12)
				atabort(BAD12HR);
			at.tm_hour %= 12;
			break;

		case PM:
			if (at.tm_hour < 1 || at.tm_hour > 12)
				atabort(BAD12HR);
			at.tm_hour %= 12;
			at.tm_hour += 12;
			break;

		case ZULU:
			if (at.tm_hour == 24 && at.tm_min != 0)
				atabort(BADHOURS);
			at.tm_hour %= 24;
			gmtflag = 1;
			break;

		case NOON:
			if (at.tm_hour != 12)
				atabort("noon must be 12n");
			break;

		case MIDNIGHT:
			if (at.tm_hour != 12)
				atabort("midnight must be 12m");
			at.tm_hour = 0;
			break;

		default:
			if (!suffix)
				atabort(BADTIME);
			i--;
			break;
		}
	}

	if (i < argc) {
		argp = argv[i++];
		if ((month = lookup_token(argp, months)) >= 0) {
			if (i == argc) {
				i--;
				goto found_filename;
			}
			at.tm_mon = month;
			argp = argv[i++];	/* day of month */
			if ((argp = convert_to_num(argp, &at.tm_mday)) == NULL)
				atabort(BADDATE);
			if (strcmp(argp, ",") == 0) {
				if (i == argc)
					atabort(BADDATE);
				argp = argv[i++];
				if ((argp = convert_to_num(argp, &at.tm_year)) == NULL
				    || *argp != '\0')
					atabort(BADDATE);
			} else if (*argp == '\0') {
				at.tm_year = tp->tm_year;
				if (at.tm_mon < tp->tm_mon)
					at.tm_year++;
			} else
				atabort(BADDATE);
			saw_date = 1;
		} else if ((dow = lookup_token(argp, days_of_week)) >= 0) {
			at.tm_mon = tp->tm_mon;
			at.tm_mday = tp->tm_mday;
			at.tm_year = tp->tm_year;
			if (dow < 7) {
				rt.tm_mday = dow - tp->tm_wday;
				if (rt.tm_mday < 0)
					rt.tm_mday += 7;
			} else if (dow == 8)
				rt.tm_mday += 1;
			saw_date = 1;
		} else
			i--;
	}

	if (i < argc) {
		argp = argv[i++];
		if (strcmp(argp, "next") == 0) {
			if (i == argc) {
				i--;
				goto found_filename;
			}
			argp = argv[i++];
			if ((dow = lookup_token(argp, days_of_week)) >= 0) {
				if (saw_date)
					atabort("bad date specification");
				at.tm_mon = tp->tm_mon;
				at.tm_mday = tp->tm_mday;
				at.tm_year = tp->tm_year;
				if (dow < 7) {
					rt.tm_mday = dow - tp->tm_wday;
					if (rt.tm_mday <= 0)
						rt.tm_mday += 7;
				} else
					atabort("bad date specification");
			} else {
				incr = 1;
				goto addincr;
			}
		} else if (*argp++ == '+') {
			if (*argp == '\0') {
				if (i == argc) {
					i--;
					goto found_filename;
				}
				argp = argv[i++];
			}
			if (isdigit(*argp)) {
				if ((argp = convert_to_num(argp, &incr)) == NULL
				    || *argp != '\0'
				    || i == argc)
					atabort(BADDATE);
				argp = argv[i++];
			}
		addincr:
			switch (lookup_token(argp, units)) {

			case MINUTES:
				rt.tm_min += incr;
				break;

			case HOURS:
				rt.tm_hour += incr;
				break;

			case DAYS:
				rt.tm_mday += incr;
				break;

			case WEEKS:
				rt.tm_mday += incr * 7;
				break;

			case MONTHS:
				rt.tm_mon += incr;
				break;

			case YEARS:
				rt.tm_year += incr;
				break;

			default:
				atabort("bad next specification");
			}
		} else
			i--;
	}

found_filename:
	if (!saw_date) {
		/*
		 * Default to today, unless the time is in the past;
		 * in that case, default to tomorrow.
		 */
		at.tm_mday = tp->tm_mday;
		at.tm_mon = tp->tm_mon;
		at.tm_year = tp->tm_year;
		if ((at.tm_hour < tp->tm_hour)
			|| ((at.tm_hour==tp->tm_hour)&&(at.tm_min<tp->tm_min)))
			rt.tm_mday++;
	}
	if (at.tm_min >= 60 || at.tm_hour >= 24)
		atabort("bad time");
	if (at.tm_mon >= 12 || at.tm_mday > mday[at.tm_mon])
		atabort("bad date");
	if (at.tm_year >= 100)
		at.tm_year -= 1900;
	if (at.tm_year < 70 || at.tm_year >= 100)
		atabort("bad year");
	return (argv[i]);
}

char *
gethhmm(argp, hourp, minp)
	register char *argp;
	int *hourp;
	int *minp;
{
	register char c;
	register int digit2, digit3;
	register int hour, min;

	c = *argp++;
	if (!isdigit(c))
		atabort(BADTIME);
	hour = c - '0';
	min = 0;
	c = *argp;
	if (c == ':') {
		argp++;	/* H: */
		c = *argp;
		if (isdigit(c)) {
			argp++;	/* H:M */
			min = c - '0';
			c = *argp;
			if (isdigit(c)) {
				argp++;	/* H:MM */
				min = min*10 + c - '0';
			}
		}
	} else {
		if (isdigit(c)) {
			/*
			 * HH or HM
			 */
			argp++;
			digit2 = c - '0';
			c = *argp;
			if (c == ':') {
				/*
				 * HH:
				 */
				argp++;
				hour = hour*10 + digit2;
				c = *argp;
				if (isdigit(c)) {
					/*
					 * HH:M
					 */
					argp++;
					min = c - '0';
					c = *argp;
					if (isdigit(c)) {
						/*
						 * HH:MM
						 */
						argp++;
						min = min*10 + c - '0';
					}
				}
			} else if (isdigit(c)) {
				/*
				 * HMM or HHMM
				 */
				argp++;
				digit3 = c - '0';
				c = *argp;
				if (isdigit(c)) {
					/*
					 * HHMM
					 */
					argp++;
					hour = hour*10 + digit2;
					min = digit3*10 + c - '0';
				} else {
					/*
					 * HMM
					 */
					min = digit2*10 + digit3;
				}
			} else {
				/*
				 * HH
				 */
				hour = hour*10 + digit2;
			}
		}
	}

	if (hour > 23)
		atabort(BADHOURS);
	if (min > 59)
		atabort(BADMINUTES);
	*hourp = hour;
	*minp = min;
	return (argp);
}

int
lookup_token(argp, tablep)
	char *argp;
	register struct tokentable *tablep;
{
	register char *p1, *p2;
	register char c;

	while ((p1 = tablep->tt_name) != NULL) {
		p2 = argp;
		do {
			c = *p2++;
			if (isupper(c))
				c = tolower(c);
			if (c != *p1++)
				goto nomatch;
		} while (c != '\0');
		return (tablep->tt_value);

	nomatch:
		tablep++;
	}
	return (-1);
}

char *
convert_to_num(argp, nump)
	register char *argp;
	int *nump;
{
	register int num;
	register char c;

	if (!isdigit(*argp))
		return (NULL);
	num = 0;
	while ((c = *argp++) != '\0' && isdigit(c))
		num = num*10 + c - '0';
	*nump = num;
	return (argp - 1);
}

/***************/
find(elem,table,tabsize)
/***************/
char *elem,**table;
int tabsize;
{
	register char *cp;
	register char c;
	register int i;

	while ((c = *cp) != '\0') {
		if (isupper(c))
			c = tolower(c);
		*cp++ = c;
	}
	for (i=0; i<tabsize; i++)
		if (strcmp(elem,table[i])==0) return(i);
		else if (strncmp(elem,table[i],3)==0) return(i);
	return(-1);
}


/****************/
char *mkjobname(t)
/****************/
time_t t;
{
	int i;
	char *name;
	struct  stat buf;
	name=xmalloc(200);
	for (i=0;i < MAXTRYS;i++) {
		sprintf(name,"%s/%ld.%c.%d",ATDIR,t,'a'+jobtype,i);
		if (stat(name,&buf)) 
			return(name);
	}
	atabort("queue full");
}


/****************/
catch()
/****************/
{
	unlink(tfname);
	exit(1);
}


/****************/
aterror(msg)
/****************/
char *msg;
{
	fprintf(stderr,"at: %s\n",msg);
}

/****************/
atperror(msg)
/****************/
char *msg;
{
	fprintf(stderr,"at: %s: %s\n", msg, errmsg(errno));
}

/****************/
atabort(msg)
/****************/
char *msg;
{
	aterror(msg);
	exit(1);
}

/****************/
atabortperror(msg)
/****************/
char *msg;
{
	atperror(msg);
	exit(1);
}

/*
 * add time structures logically
 */
atime(a, b)
register
struct tm *a, *b;
{
	if ((a->tm_sec += b->tm_sec) >= 60) {
		b->tm_min += a->tm_sec / 60;
		a->tm_sec %= 60;
	}
	if ((a->tm_min += b->tm_min) >= 60) {
		b->tm_hour += a->tm_min / 60;
		a->tm_min %= 60;
	}
	if ((a->tm_hour += b->tm_hour) >= 24) {
		b->tm_mday += a->tm_hour / 24;
		a->tm_hour %= 24;
	}
	a->tm_year += b->tm_year;
	if ((a->tm_mon += b->tm_mon) >= 12) {
		a->tm_year += a->tm_mon / 12;
		a->tm_mon %= 12;
	}
	a->tm_mday += b->tm_mday;
	while (a->tm_mday > mday[a->tm_mon]) {
		a->tm_mday -= mday[a->tm_mon++];
		if (a->tm_mon > 11) {
			a->tm_mon = 0;
			mday[1] = 28 + leap(++a->tm_year);
		}
	}
}

leap(year)
{
	return year % 4 == 0;
}

/*
 * make job file from proto + input file
 */
copy(jobfile, inputfile, spoolfile)
	char *jobfile;
	register FILE *inputfile;
	register FILE *spoolfile;
{
	register c;
	register FILE *pfp;
	register FILE *xfp;
	char	*shell;
	char	line[BUFSIZ];		/* a line from input file */
	char	dirbuf[MAXPATHLEN];
	register char **ep;
	register char *tmp;
	long ulimit();
	mode_t um, umask();
	extern char **environ;
	int pfd[2];
	int pid;
	unsigned short realusr;
	int ttyinput;

	/*
	 * Make sure we can get at the prototype file, first.
	 */
	if((pfp = fopen_configfile(pname1,"r",dirbuf)) == NULL
	   && (pfp = fopen_configfile(pname,"r",dirbuf)) == NULL)
		atabortperror("Can't open prototype file for queue");

	/*
	 * If the inputfile is from a tty, then turn on prompting, and
	 * put out a prompt now, instead of waiting for a lot of file
	 * activity to complete.
	 */
	ttyinput = isatty(fileno(inputfile));
	if (ttyinput) {
		fputs("at> ", stdout);
		fflush(stdout);
	}

	/*
	 * Determine what shell we should use to run the job. If the user
	 * didn't explicitly request that his/her current shell be over-
	 * ridden (shflag or cshflag), then we use the current shell.
	 */
	if (cshflag)
		shell = "/bin/csh";
	else if (shflag)
		shell = "/bin/sh";
	else if (getenv("SHELL") != NULL)
		shell = "$SHELL";
	else
		shell = "/bin/sh";

	fprintf(spoolfile, "# %s job\n",jobtype ? "batch" : "at");
	fprintf(spoolfile, "# owner: %.127s\n",login);
	fprintf(spoolfile, "# jobname: %.127s\n",jobfile);
	fprintf(spoolfile, "# shell: sh\n");
	fprintf(spoolfile, "# notify by mail: %s\n",(mailflag) ? "yes" : "no");
	fprintf(spoolfile, "\n");

	/*
	 * Copy the user's environment to the spoolfile in the syntax of the
	 * Bourne shell.  After the environment is set up, the proper shell
	 * will be invoked.
	 */
	for (ep=environ; *ep; ep++) {
		tmp = *ep;

		/*
		 * Don't copy invalid environment entries.
		 */
		if (strchr(tmp, '=') == NULL)
			continue;

		/*
		 * We don't want the termcap or terminal entry so skip them.
		 */
		if ((strncmp(tmp,"TERM=",5) == 0) ||
		    (strncmp(tmp,"TERMCAP=",8) == 0))
			continue;

		/*
		 * Set up the proper syntax.
		 */
		while ((c = *tmp++) != '=')
			fputc(c, spoolfile);
		fputc('=', spoolfile);
		fputc('\'' , spoolfile);

		/*
		 * Now copy the entry.
		 */
		while ((c = *tmp++) != '\0') {
			if (c == '\'')
				fputs("'\\''", spoolfile);
			else if (c == '\n')
				fputs("\\",spoolfile);
			else
				fputc(c, spoolfile);
		}
		fputc('\'' , spoolfile);

		/*
		 * We need to "export" environment settings.
		 */
		fprintf(spoolfile, "\nexport ");
		tmp = *ep;
		while ((c = *tmp++) != '=')
			fputc(c,spoolfile);
		fputc('\n',spoolfile);
	}

	/*
	 * Put in a line to run the proper shell using the rest of
	 * the file as input.  Note that 'exec'ing the shell will
	 * cause sh() to leave a /tmp/sh### file around.
	 */
	fprintf(spoolfile,
	    "%s << '...the rest of this file is shell input'\n", shell);

	um = umask(0);
	while ((c = getc(pfp)) != EOF) {
		if (c != '$')
			putc(c, spoolfile);
		else switch (c = getc(pfp)) {
		case EOF:
			goto out;
		case 'd':
			(void) seteuid(user);
			if (getwd(dirbuf) == NULL) {
				(void) seteuid(ROOT);
				unlink(tfname);
				atabort(dirbuf);
			}
			(void) seteuid(ROOT);
			fprintf(spoolfile, "%s", dirbuf);
			break;
		case 'l':
			fprintf(spoolfile, "%ld",ulimit(1,-1L));
			break;
		case 'm':
			fprintf(spoolfile, "%o", um);
			break;
		case '<':
			while (fgets(line, LINSIZ, inputfile) != NULL) {
				fputs(line, spoolfile);
				if (ttyinput)
					fputs("at> ", stdout);
			}
			if (ttyinput)
				fputs("<EOT>\n", stdout);	/* clean up the final output */
			break;
		case 't':
			fprintf(spoolfile, ":%lu", when);
			break;
		default:
			putc(c, spoolfile);
		}
	}
out:
	fclose(pfp);
	if (inputfile != stdin)
		fclose(inputfile);
	fflush(spoolfile);
	if (fchmod(fileno(spoolfile),ATMODE)==-1) {
		unlink(tfname);
		atabortperror(CANTCHMOD);
	}
	fclose(spoolfile);
}
