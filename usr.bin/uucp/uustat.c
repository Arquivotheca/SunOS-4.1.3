/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)uustat.c 1.1 92/07/30"	/* from SVR3.2 uucp:uustat.c 2.5 */

#include "uucp.h"

#ifdef	V7
#define O_RDONLY	0
#endif

#define USAGE1	"[-a] [-q] or [-m] or [-kJOB] or [-rJOB] or [-p]"
#define USAGE2	"[-sSYSTEM] [-uUSER]"

#define STST_MAX	132
struct m {
	char	mach[15];		/* machine name */
	char	locked;
	int	ccount, xcount;
	int	count, type;
	long	retrytime;
	time_t lasttime;
	short	c_age;			/* age of oldest C. file */
	short	x_age;			/* age of oldest X. file */
	char	stst[STST_MAX];
} M[UUSTAT_TBL+2];

extern long atol();
extern void qsort();		/* qsort(3) and comparison test */
int sortcnt = -1;
extern int machcmp();
extern int _age();		/* find the age of a file */

extern char Jobid[];	/* jobid for status or kill option */
short Kill;		/*  == 1 if -k specified */
short Rejuvenate;	/*  == 1 for -r specified */
short Uopt;		/*  == 1 if -u option specified */
short Sysopt;		/*  == 1 if -s option specified */
short Summary;		/*  == 1 if -q or -m is specified */
short Queue;		/*  == 1 if -q option set - queue summary */
short Machines;		/*  == 1 if -m option set - machines summary */
short Psopt;		/*  == 1 if -p option set - output "ps" of LCK pids */

main(argc, argv, envp)
char *argv[];
char **envp;
{
	struct m *m, *machine();
	DIR *spooldir, *subdir;
	char *str, *rindex();
	char f[256], subf[256];
	char *c, lckdir[BUFSIZ];
	char buf[BUFSIZ];
	char chkname[MAXFULLNAME];
	char *vec[7];
	int i, chkid;


	User[0] = '\0';
	Rmtname[0] = '\0';
	Jobid[0] = '\0';
	Psopt=Machines=Summary=Queue=Kill=Rejuvenate=Uopt=Sysopt=0;
	(void) strcpy(Progname, "uustat");
	Uid = getuid();
	Euid = geteuid();
	guinfo(Uid, Loginuser);
	uucpname(Myname);
	while ((i = getopt(argc, argv, "ak:mpr:qs:u:x:")) != EOF) {
		switch(i){
		case 'a':
			Sysopt = 1;
			break;
		case 'k':
			(void) strncpy(Jobid, optarg, NAMESIZE);
			Jobid[NAMESIZE] = '\0';
			Kill = 1;
			break;
		case 'm':
			Machines = Summary = 1;
			break;
		case 'p':
			Psopt = 1;
			break;
		case 'r':
			(void) strncpy(Jobid, optarg, NAMESIZE);
			Jobid[NAMESIZE] = '\0';
			Rejuvenate = 1;
			break;
		case 'q':
			Queue = Summary = 1;
			break;
		case 's':
			(void) strncpy(Rmtname, optarg, MAXBASENAME);
			Rmtname[MAXBASENAME] = '\0';
			if (versys(Rmtname)) {
				fprintf(stderr, "Invalid system\n");
				exit(1);
			}
			Sysopt = 1;
			break;
		case 'u':
			(void) strncpy(User, optarg, 8);
			User[8] = '\0';
			if(gninfo(User, &chkid, chkname)) {
				fprintf(stderr, "Invalid user\n");
				exit(1);
			}
			Uopt = 1;
			break;
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0)
				Debug = 1;
			break;
		default:
			(void) fprintf(stderr, "\tusage: %s %s\n",
			    Progname, USAGE1);
			(void) fprintf(stderr, "or\n\tusage: %s %s\n",
			    Progname, USAGE2);
			exit(1);
		}
	}

	if (argc != optind) {
		(void) fprintf(stderr, "\tusage: %s %s\n", Progname, USAGE1);
		(void) fprintf(stderr, "or\n\tusage: %s %s\n",
		    Progname, USAGE2);
		exit(1);
	}

	DEBUG(9, "Progname (%s): STARTED\n", Progname);
	DEBUG(9, "User=%s, ", User);
	DEBUG(9, "Loginuser=%s, ", Loginuser);
	DEBUG(9, "Jobid=%s, ", Jobid);
	DEBUG(9, "Rmtname=%s\n", Rmtname);

	if ((Psopt + Machines + Queue + Kill + Rejuvenate + (Uopt|Sysopt)) >1) {
		/* only -u and -s can be used together */
		printf("\tusage: %s %s\n", Progname, USAGE1);
		printf("or\n\tusage: %s %s\n", Progname, USAGE2);
		exit(1);
	}
	if (  !(Kill | Rejuvenate | Uopt | Sysopt | Queue | Machines) ) {
		(void) strcpy(User, Loginuser);
		Uopt = 1;
	}

	if (Psopt) {
		/* do "ps -flp" or pids in LCK files */
		lckpid();
		/* lckpid will not return */
	}

	if (Summary) {
	    /*   Gather data for Summary option report  */
	    if (chdir(STATDIR) || (spooldir = opendir(STATDIR)) == NULL)
		exit(101);		/* good old code 101 */
	    while (gnamef(spooldir, f) == TRUE) {
		if (freopen(f, "r", stdin) == NULL)
			continue;
		m = machine(f);
		if (gets(buf) == NULL)
			continue;
		if (getargs(buf, vec, 5) < 5)
			continue;
		m->type = atoi(vec[0]);
		m->count = atoi(vec[1]);
		m->lasttime = atol(vec[2]);
		m->retrytime = atol(vec[3]);
		(void) strncpy(m->stst, vec[4], STST_MAX);
		str = rindex(m->stst, ' ');
		(void) machine(++str);	/* longer name? */
		*str = '\0';
			
	    }
	    closedir(spooldir);
	}


	if (Summary) {
	    /*  search for LCK machines  */

	    (void) strcpy(lckdir, LOCKPRE);
	    *strrchr(lckdir, '/') = '\0';
	    /* open lock directory */
	    if (chdir(lckdir) != 0 || (subdir = opendir(lckdir)) == NULL)
		exit(101);		/* good old code 101 */

	    while (gnamef(subdir, f) == TRUE) {
		/* XXX - this is disgusting... */
		if (EQUALSN("LCK..", f, 5)) {
		    if (!EQUALSN(f + 5, "cul", 3)
		     && !EQUALSN(f + 5, "cua", 3)
		     && !EQUALSN(f + 5, "tty", 3)
		     && !EQUALSN(f + 5, "dtsw", 4)
		     && !EQUALSN(f + 5, "vadic", 5)
		     && !EQUALSN(f + 5, "micom", 5))
			machine(f + 5)->locked++;
		}
	    }
	}

	if (chdir(SPOOL) != 0 || (spooldir = opendir(SPOOL)) == NULL)
		exit(101);		/* good old code 101 */
	while (gnamef(spooldir, f) == TRUE) {
	    if (EQUALSN("LCK..", f, 5))
		continue;

	    if (*Rmtname && !EQUALSN(Rmtname, f, SYSNSIZE))
		continue;

	    if ( (Kill || Rejuvenate)
	      && (!EQUALSN(f, Jobid, strlen(Jobid)-5)) )
		    continue;

	    if (!Machines && DIRECTORY(f) && (subdir = opendir(f))) {
		m = machine(f);
	        while (gnamef(subdir, subf) == TRUE)
		    if (subf[1] == '.') {
		        if (subf[0] == CMDPRE) {
				m->ccount++;
				if (Kill || Rejuvenate)
				    kprocessC(f, subf);
				else if (Uopt || Sysopt)
				    uprocessC(f, subf);
				else 	/* get the age of the C. file */
				    if ( (i = _age(f, subf))>m->c_age)
					m->c_age = i;
			}

			else if (subf[0] == XQTPRE) {
			        m->xcount++;
				if ( (i = _age(f, subf)) > m->x_age)
					m->x_age = i;
			}

		    }
		closedir(subdir);
	    }
	}
	/* for Kill or Rejuvenate - will not get here unless it failed */
	if (Kill || Rejuvenate) {
	    printf("Can't find Job %s; Not %s\n", Jobid,
		Kill ? "killed" : "rejuvenated");
	    exit(1);
	}

	/* Make sure the overflow entry is null since it may be incorrect */
	M[UUSTAT_TBL].mach[0] = NULLCHAR;
	if (Summary) {
	    for((sortcnt = 0, m = &M[0]);*(m->mach) != NULL;(sortcnt++,m++))
			;
	    qsort((char *)M, (unsigned int)sortcnt, sizeof(struct m), machcmp);
	    for (m = M; m->mach[0] != NULLCHAR; m++)
		printit(m);
	}
	exit(0);

	/* NOTREACHED */
}


/*
 * uprocessC - get information about C. file
 *
 */

uprocessC(dir, file)
char *file, *dir;
{
	struct stat s;
	register struct tm *tp;
	char fullname[MAXFULLNAME], buf[BUFSIZ], user[9];
	char xfullname[MAXFULLNAME];
	char file1[BUFSIZ], file2[BUFSIZ], file3[BUFSIZ], type[2], opt[256];
	FILE *fp, *xfp;
	short first = 1;
	extern long fsize();

	DEBUG(9, "uprocessC(%s, ", dir);
	DEBUG(9, "%s);\n", file);

	if (Jobid[0] != '\0' && (!EQUALS(Jobid, &file[2])) ) {
		/* kill job - not this one */
		return;
	}

	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &s) != 0) {
	     /* error - can't stat */
	    DEBUG(4, "Can't stat file (%s),", fullname);
	    DEBUG(4, " errno (%d) -- skip it!\n", errno);
	}

	fp = fopen(fullname, "r");
	if (fp == NULL) {
		DEBUG(4, "Can't open file (%s), ", fullname);
		DEBUG(4, "errno=%d -- skip it!\n", errno);
		return;
	}
	tp = localtime(&s.st_mtime);

	if (s.st_size == 0 && User[0] == '\0') { /* dummy D. for polling */
	    printf("%-12s  %2.2d/%2.2d-%2.2d:%2.2d:%2.2d  (POLL)\n",
		&file[2], tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		tp->tm_min, tp->tm_sec);
	}
	else while (fgets(buf, BUFSIZ, fp) != NULL) {
	    if (sscanf(buf,"%s%s%s%s%s%s", type, file1, file2,
	      user, opt, file3) <5) {
		DEBUG(4, "short line (%s)\n", buf);
		continue;
	    }
	    DEBUG(9, "type (%s), ", type);
	    DEBUG(9, "file1 (%s)", file1);
	    DEBUG(9, "file2 (%s)", file2);
	    DEBUG(9, "file3 (%s)", file3);
	    DEBUG(9, "user (%s)", user);

	    if (User[0] != '\0' && (!EQUALS(User, user)) )
		continue;

/*
	    if ( (*file2 != 'X')
	      && (*file1 != '/' && *file1 != '~')
	      && (*file2 != '/' && *file2!= '~') )
		continue;
*/

	    if (first)
	        printf("%-12s  %2.2d/%2.2d-%2.2d:%2.2d  ",
		    &file[2], tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		    tp->tm_min);
	    else
		printf("%-12s  %2.2d/%2.2d-%2.2d:%2.2d  ",
		    "", tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		    tp->tm_min);
	    first = 0;

	    printf("%s  %s  ", type, dir);
	    if (*type == 'R')
	        printf("%s  %s\n", user, file1);
	    else if (file2[0] != 'X')
		printf("%s %ld %s\n", user, fsize(dir, file3, file1), file1);
	    else if (*type == 'S' && file2[0] == 'X') {
		(void) sprintf(xfullname, "%s/%s", dir, file1);
		xfp = fopen(xfullname, "r");
		if (xfp == NULL) { /* program error */
		    DEBUG(4, "Can't read %s, ", xfullname);
		    DEBUG(4, "errno=%d -- skip it!\n", errno);
		    printf("%s  ????\n", user);
		}
		else {
		    char command[BUFSIZ], uline_u[BUFSIZ], uline_m[BUFSIZ];
		    char retaddr[BUFSIZ], *username;

		    *retaddr = *uline_u = *uline_m = '\0';
		    while (fgets(buf, BUFSIZ, xfp) != NULL) {
			switch(buf[0]) {
			case 'C':
				strcpy(command, buf + 2);
				break;
			case 'U':
				sscanf(buf + 2, "%s%s", uline_u, uline_m);
				break;
			case 'R':
				sscanf(buf+2, "%s", retaddr);
				break;
			}
		    }
		    username = user;
		    if (*uline_u != '\0')
			    username = uline_u;
		    if (*retaddr != '\0')
			username = retaddr;
		    if (!EQUALS(uline_m, Myname))
			printf("%s!", uline_m);
		    printf("%s  %s", username, command);
		}
		if (xfp != NULL)
		    fclose(xfp);
	    }
	}

	fclose(fp);
	return;
}


/*
 * kprocessC - process kill or rejuvenate job
 */

kprocessC(dir, file)
char *file, *dir;
{
	struct stat s;
	register struct tm *tp;
	extern struct tm *localtime();
	extern int errno;
	char fullname[MAXFULLNAME], buf[BUFSIZ], user[9];
	char rfullname[MAXFULLNAME];
	char file1[BUFSIZ], file2[BUFSIZ], file3[BUFSIZ], type[2], opt[256];
	FILE *fp, *xfp;
 	time_t times[2];
	short ret;
	short first = 1;

	DEBUG(9, "kprocessC(%s, ", dir);
	DEBUG(9, "%s);\n", file);

	if ((!EQUALS(Jobid, &file[2])) ) {
		/* kill job - not this one */
		return;
	}

	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &s) != 0) {
	     /* error - can't stat */
	    fprintf(stderr, "Can't stat:%s, errno (%d)--can't %s it!\n",
		fullname, errno, Kill ? "kill" : "rejuvenate");
	    exit(1);
	}

	fp = fopen(fullname, "r");
	if (fp == NULL) {
	    fprintf(stderr, "Can't read:%s, errno (%d)--can't %s it!\n",
		fullname, errno, Kill ? "kill" : "rejuvenate");
	    exit(1);
	}

 	times[0] = times[1] = time((time_t *)NULL);
 
	while (fgets(buf, BUFSIZ, fp) != NULL) {
	    if (sscanf(buf,"%s%s%s%s%s%s", type, file1, file2,
	      user, opt, file3) <6) {
		fprintf(stderr, "Bad format:%s, errno (%d)--can't %s it!\n",
		    fullname, errno, Kill ? "kill" : "rejuvenate");
	        exit(1);
	    }
	    DEBUG(9, "type (%s), ", type);
	    DEBUG(9, "file1 (%s)", file1);
	    DEBUG(9, "file2 (%s)", file2);
	    DEBUG(9, "file3 (%s)", file3);
	    DEBUG(9, "user (%s)", user);


	    if (first) {
	        if (Uid != 0
		    && !PREFIX(Loginuser, user)
		    && !PREFIX(user, Loginuser) ) {
			/* not allowed - not owner or root */
			fprintf(stderr,
			    "Not owner or root - can't %s job %s\n",
    			    Kill ? "kill" : "rejuvenate", Jobid);
		    exit(1);
		}
		first = 0;
	    }

	    /* remove D. file */
	    (void) sprintf(rfullname, "%s/%s", dir, file3);
	    DEBUG(4, "Remove %s\n", rfullname);
	    if (Kill)
		ret = unlink(rfullname);
	    else /* Rejuvenate */
 		ret = utime(rfullname, times);
	    if (ret != 0 && errno != ENOENT) {
		/* program error?? */
		fprintf(stderr, "Error: Can't %s, File ( %s), errno (%d)\n",
		    Kill ? "kill" : "rejuvenate", rfullname, errno);
		exit(1);
	    }
	}

	DEBUG(4, "Remove %s\n", fullname);
	if (Kill)
	    ret = unlink(fullname);
	else /* Rejuvenate */
		ret = utime(fullname, times);
	
	if (ret != 0) {
	    /* program error?? */
		fprintf(stderr, "Error: Can't %s, File ( %s), errno (%d)\n",
		    Kill ? "kill" : "rejuvenate", fullname, errno);
	    exit(1);
	}
	fclose(fp);
	printf("Job: %s successfully %s\n", Jobid,
	    Kill ? "killed" : "rejuvenated");
	exit(0);
}

/*
 * fsize - return the size of f1 or f2 (if f1 does not exist)
 *	f1 is the local name
 *
 */

long
fsize(dir, f1, f2)
char *dir, *f1, *f2;
{
	struct stat s;
	char fullname[BUFSIZ];

	(void) sprintf(fullname, "%s/%s", dir, f1);
	if (stat(fullname, &s) == 0) {
	    return(s.st_size);
	}
	if (stat(f2, &s) == 0) {
	    return(s.st_size);
	}

	return(-99999);
}

cleanup(){}
void logent(){}		/* to load ulockf.c */
void systat(){}		/* to load utility.c */

struct m	*
machine(name)
char	*name;
{
	struct m *m;
	int	namelen;

	DEBUG(9, "machine(%s), ", name);
	namelen = strlen(name);
	for (m = M; m->mach[0] != NULLCHAR; m++)
		/* match on overlap? */
		if (EQUALSN(name, m->mach, SYSNSIZE)) {
			/* use longest name */
			if (namelen > strlen(m->mach))
				(void) strcpy(m->mach, name);
			return(m);
		}

	/*
	 * The table is set up with 2 extra entries
	 * When we go over by one, output error to errors log
	 * When more than one over, just reuse the previous entry
	 */
	DEBUG(9, "m-M=%d\n", m-M);
	if (m-M >= UUSTAT_TBL) {
	    if (m-M == UUSTAT_TBL) {
		errent("MACHINE TABLE FULL", "", UUSTAT_TBL,
		__FILE__, __LINE__);
		(void) fprintf(stderr,
		    "WARNING: Table Overflow--output not complete\n");
	    }
	    else
		/* use the last entry - overwrite it */
		m = &M[UUSTAT_TBL];
	}

	(void) strcpy(m->mach, name);
	m->c_age= m->x_age= m->lasttime= m->locked= m->ccount= m->xcount= 0;
	m->stst[0] = '\0';
	return(m);
}

printit(m)
struct m *m;
{
	register struct tm *tp;
	time_t	t;
	int	min;
	extern struct tm *localtime();

	if (m->ccount == 0
	 && m->xcount == 0
	 /*&& m->stst[0] == '\0'*/
	 && m->locked == 0
	 && Queue
	 && m->type == 0)
		return;
	printf("%-10s", m->mach);
	if (Queue) {
		if (m->ccount)
			printf("%3dC", m->ccount);
		else
			printf("    ");
		if (m->c_age)
			printf("(%d)", m->c_age);
		else
			printf("   ");
		if (m->xcount)
			printf("%3dX", m->xcount);
		else
			printf("    ");
		if (m->x_age)
			printf("(%d) ", m->x_age);
		else
			printf("    ");
	} else
		printf(" ");

	if (m->lasttime) {
	    tp = localtime(&m->lasttime);
	    printf("%2.2d/%2.2d-%2.2d:%2.2d ",
		tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		tp->tm_min);
	}
/*	if (m->locked && m->type != SS_INPROGRESS) */
	if (m->locked)
		printf("Locked ");
	if (m->stst[0] != '\0') {
		printf("%s", m->stst);
		switch (m->type) {
		case SS_SEQBAD:
		case SS_LOGIN_FAILED:
		case SS_DIAL_FAILED:
		case SS_BAD_LOG_MCH:
		case SS_BADSYSTEM:
		case SS_CANT_ACCESS_DEVICE:
		case SS_DEVICE_FAILED:
		case SS_WRONG_MCH:
		case SS_RLOCKED:
		case SS_RUNKNOWN:
		case SS_RLOGIN:
		case SS_UNKNOWN_RESPONSE:
		case SS_STARTUP:
		case SS_CHAT_FAILED:
			(void) time(&t);
			t = m->retrytime - (t - m->lasttime);
			if (t > 0) {
				min = (t + 59) / 60;
				printf("Retry: %d:%2.2d", min/60, min%60);
			}
			if (m->count > 1)
				printf(" Count: %d", m->count);
		}
	}
	putchar('\n');
}

#define MAXLOCKS 100	/* Maximum number of lock files this will handle */

lckpid()
{
    register i;
    int fd;
    long pid, ret;
    char alpid[SIZEOFPID+2];	/* +2 for '\n' and null */
    long list[MAXLOCKS];
    char buf[BUFSIZ], f[MAXBASENAME];
    char *c, lckdir[BUFSIZ];
    DIR *dir;

    DEBUG(9, "lckpid() - entered\n", "");
    for (i=0; i<MAXLOCKS; i++)
	list[i] = -1;
    (void) strcpy(lckdir, LOCKPRE);
    *strrchr(lckdir, '/') = '\0';
    DEBUG(9, "lockdir (%s)\n", lckdir);

    /* open lock directory */
    if (chdir(lckdir) != 0 || (dir = opendir(lckdir)) == NULL)
		exit(101);		/* good old code 101 */
    while (gnamef(dir, f) == TRUE) {
	/* find all lock files */
	DEBUG(9, "f (%s)\n", f);
	if (EQUALSN("LCK.", f, 4)) {
	    /* read LCK file */
	    fd = open(f, O_RDONLY);
	    printf("%s: ", f);
	    ret = read(fd, alpid, SIZEOFPID+2); /* +2 for '\n' and null */
	    pid = atoi(alpid);
	    (void) close(fd);
	    if (ret != -1) {
		printf("%d\n", pid);
		for(i=0; i<MAXLOCKS; i++) {
		    if (list[i] == pid)
			break;
		    if (list[i] == -1) {
		        list[i] = pid;
		        break;
		    }
		}
	    }
	    else
		printf("????\n");
	}
    }
    fflush(stdout);
    *buf = NULLCHAR;
    for (i=0; i<MAXLOCKS; i++) {
	if( list[i] == -1)
		break;
	(void) sprintf(&buf[strlen(buf)], "%d ", list[i]);
    }

    if (i > 0)
#ifdef V7
	execl("/bin/ps", "uustat-ps", buf, (char *) 0);
#else
	execl("/bin/ps", "ps", "-flp", buf, (char *) 0);
#endif
    exit(0);
}

int machcmp(a,b)
char *a,*b;
{
	return(strcmp(((struct m *) a)->mach,((struct m *) b)->mach));
}

static long _sec_per_day = 86400L;

/*
 * _age - find the age of "file" in days
 * return:
 *	age of file
 *	0 - if stat fails
 */

int
_age(dir, file)
char * file;	/* the file name */
char * dir;	/* system spool directory */
{
	char fullname[MAXFULLNAME];
	static time_t ptime = 0;
	time_t time();
	struct stat stbuf;

	if (!ptime)
		(void) time(&ptime);
	(void) sprintf(fullname, "%s/%s", dir, file);
	if (stat(fullname, &stbuf) != -1) {
		return ((int)((ptime - stbuf.st_mtime)/_sec_per_day));
	}
	else
		return(0);
}
