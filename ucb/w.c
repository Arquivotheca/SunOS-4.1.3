/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)w.c 1.1 92/07/30 SMI"; /* from UCB 5.3 2/23/86 */
#endif not lint
/*
 * w, uptime - print system status (who and what)
 *
 * This program is similar to the systat command on Tenex/Tops 10/20
 * It needs read permission on /dev/mem, /dev/kmem, and /dev/drum.
 */
#include <sys/param.h>
#include <nlist.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <kvm.h>
#include <fcntl.h>
#include <locale.h>

#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)

#define ARGWIDTH	33	/* # chars left on 80 col crt for args */

struct pr {
	short	w_pid;			/* proc.p_pid */
	char	w_flag;			/* proc.p_flag */
	short	w_size;			/* proc.p_size */
	int	w_igintr;		/* INTR+3*QUIT, 0=die, 1=ign, 2=catch */
	time_t	w_time;			/* CPU time used by this process */
	time_t	w_ctime;		/* CPU time used by children */
	dev_t	w_tty;			/* tty device of process */
	int	w_uid;			/* uid of process */
	char	w_comm[15];		/* user.u_comm, null terminated */
	char	w_args[ARGWIDTH+1];	/* args if interesting process */
} *pr;
int	nproc;

struct	nlist nl[] = {
	{ "_avenrun" },
#define	X_AVENRUN	0
	{ "_boottime" },
#define	X_BOOTTIME	1
	{ "_nproc" },
#define	X_NPROC		2
#ifdef sun
        { "_rconsdev" },	/* added to access the console information */
#define	X_NRCONSDEV	3
#endif
	{ "" },
};

FILE	*ps;
FILE	*ut;
FILE	*bootfd;
kvm_t	*kd;			/* kernel descriptor for Kernel VM calls */
dev_t	tty;
int	uid;
char	doing[520];		/* process attached to terminal */
time_t	proctime;		/* cpu time of process in doing */
long	avenrun[3];
#ifdef sun
dev_t   rconsdev;
#endif

#define	DIV60(t)	((t+30)/60)    /* x/60 rounded */ 
#define	TTYEQ		(tty == pr[i].w_tty && (uid == 0 || uid == pr[i].w_uid))
			/* match if same tty and tty owner same as process or */
			/*   tty owned by root (for ptys used for windows) */
#define IGINT		(1+3*1)		/* ignoring both SIGINT & SIGQUIT */

char	*getargs();
char	*fread();
char	*ctime();
char	*rindex();
FILE	*popen();
struct	tm *localtime();
time_t	findidle();

char	*prog;			/* pointer to invocation name */
int	debug;			/* true if -d flag: debugging output */
int	header = 1;		/* true if -h flag: don't print heading */
int	lflag = 1;		/* true if -l flag: long style output */
int	login;			/* true if invoked as login shell */
time_t	idle;			/* number of minutes user is idle */
int	nusers;			/* number of users logged in now */
char *	sel_user;		/* login of particular user selected */
char firstchar;			/* first char of name of prog invoked as */
time_t	jobtime;		/* total cpu time visible */
time_t	now;			/* the current time of day */
struct	timeval boottime;
time_t	uptime;			/* time of last reboot & elapsed time since */
int	np;			/* number of processes currently active */
struct	utmp utmp;
#define	upsize	roundup(sizeof(struct user), DEV_BSIZE)

main(argc, argv)
	char **argv;
{
	int days, hrs, mins;
	register int i, j;
	register int curpid, empty;
	char *cp;

	setlocale (LC_ALL, "");
	login = (argv[0][0] == '-');
	cp = rindex(argv[0], '/');
	firstchar = login ? argv[0][1] : (cp==0) ? argv[0][0] : cp[1];
	prog = argv[0];	/* for Usage */

	while (argc > 1) {
		if (argv[1][0] == '-') {
			for (i=1; argv[1][i]; i++) {
				switch(argv[1][i]) {

				case 'd':
					debug++;
					break;

				case 'h':
					header = 0;
					break;

				case 'l':
					lflag++;
					break;

				case 's':
					lflag = 0;
					break;

				case 'u':
				case 'w':
					firstchar = argv[1][i];
					break;

				default:
					fprintf(stderr, "%s: bad flag %s\n",
					    prog, argv[1]);
					exit(1);
				}
			}
		} else {
			if (!isalnum(argv[1][0]) || argc > 2) {
				printf("Usage: %s [ -hlsuw ] [ user ]\n", prog);
				exit(1);
			} else
				sel_user = argv[1];
		}
		argc--; argv++;
	}

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, prog)) == NULL) {
		exit(1);
	}
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr, "%s: symbols missing from namelist\n", prog);
		exit(1);
	}

	if (firstchar != 'u')
		readpr();

	ut = fopen("/etc/utmp","r");
	time(&now);
	if (header) {
		/* Print time of day */
		prtat(&now);

		/*
		 * Print how long system has been up.
		 * (Found by looking for "boottime" in kernel)
		 */
		readsym(X_BOOTTIME, &boottime, sizeof (boottime));

		uptime = now - boottime.tv_sec;
		uptime += 30;
		days = uptime / (60*60*24);
		uptime %= (60*60*24);
		hrs = uptime / (60*60);
		uptime %= (60*60);
		mins = uptime / 60;

		printf("  up");
		if (days > 0)
			printf(" %d day%s,", days, days>1?"s":"");
		if (hrs > 0 && mins > 0) {
			printf(" %2d:%02d,", hrs, mins);
		} else {
			if (hrs > 0)
				printf(" %d hr%s,", hrs, hrs>1?"s":"");
			if (mins > 0)
				printf(" %d min%s,", mins, mins>1?"s":"");
		}

		/* Print number of users logged in to system */
		while (fread(&utmp, sizeof(utmp), 1, ut)) {
			if (utmp.ut_name[0] != '\0'&& !nonuser(utmp))
				nusers++;
		}
		rewind(ut);
		printf("  %d user%s", nusers, nusers>1?"s":"");

		/*
		 * Print 1, 5, and 15 minute load averages.
		 * (Found by looking in kernel for avenrun).
		 */
		printf(",  load average:");
		readsym(X_AVENRUN, avenrun, sizeof(avenrun));
		for (i = 0; i < (sizeof(avenrun)/sizeof(avenrun[0])); i++) {
			if (i > 0)
				printf(",");
			printf(" %.2f", (double)avenrun[i]/FSCALE);
		}
		printf("\n");
		if (firstchar == 'u')
			exit(0);

		/* Headers for rest of output */
		if (lflag)
			printf("User     tty       login@  idle   JCPU   PCPU  what\n");
		else
			printf("User    tty  idle  what\n");
		fflush(stdout);
	}


	for (;;) {	/* for each entry in utmp */
		if (fread(&utmp, sizeof(utmp), 1, ut) == NULL) {
			fclose(ut);
			exit(0);
		}
		if (utmp.ut_name[0] == '\0')
			continue;	/* that tty is free */
		if (sel_user && strncmp(utmp.ut_name, sel_user, NMAX) != 0)
			continue;	/* we wanted only somebody else */

		gettty();
		jobtime = 0;
		proctime = 0;
		strcpy(doing, "-");	/* default act: normally never prints */
		empty = 1;
		curpid = -1;
		idle = findidle();
		for (i=0; i<np; i++) {	/* for each process on this tty */
			if (!(TTYEQ))
				continue;
			jobtime += pr[i].w_time + pr[i].w_ctime;
			proctime += pr[i].w_time;
			/* 
			 * Meaning of debug fields following proc name is:
			 * & by itself: ignoring both SIGINT and QUIT.
			 *		(==> this proc is not a candidate.)
			 * & <i> <q>:   i is SIGINT status, q is quit.
			 *		0 == DFL, 1 == IGN, 2 == caught.
			 * *:		proc pgrp == tty pgrp.
			 */
			if (debug) {
				printf("\t\t%d\t%s", pr[i].w_pid, pr[i].w_args);
				if ((j=pr[i].w_igintr) > 0)
					if (j==IGINT)
						printf(" &");
					else
						printf(" & %d %d", j%3, j/3);
				printf("\n");
			}
			if (empty && pr[i].w_igintr!=IGINT) {
				empty = 0;
				curpid = -1;
			}
			if(pr[i].w_pid>curpid && (pr[i].w_igintr!=IGINT || empty)){
				curpid = pr[i].w_pid;
				strcpy(doing, lflag ? pr[i].w_args : pr[i].w_comm);
#ifdef notdef
				if (doing[0]==0 || doing[0]=='-' && doing[1]<=' ' || doing[0] == '?') {
					strcat(doing, " (");
					strcat(doing, pr[i].w_comm);
					strcat(doing, ")");
				}
#endif
			}
		}
		putline();
	}
	/* NOTREACHED */
}

/* figure out the major/minor device # pair for this tty */
gettty()
{
	char ttybuf[20];
	struct stat statbuf;

	ttybuf[0] = 0;
	strcpy(ttybuf, "/dev/");
	strcat(ttybuf, utmp.ut_line);
	stat(ttybuf, &statbuf);
	tty = statbuf.st_rdev;
	uid = statbuf.st_uid;
}

/*
 * putline: print out the accumulated line of info about one user.
 */
putline()
{
	register int tm;

	/* print login name of the user */
	printf("%-*.*s ", NMAX, NMAX, utmp.ut_name);

	/* print tty user is on */
	if (lflag)
		/* long form: all (up to) LMAX chars */
		printf("%-*.*s", LMAX, LMAX, utmp.ut_line);
	else {
		/* short form: 2 chars, skipping 'tty' if there */
		if (utmp.ut_line[0]=='t' && utmp.ut_line[1]=='t' && utmp.ut_line[2]=='y')
			printf("%-2.2s", &utmp.ut_line[3]);
		else
			printf("%-2.2s", utmp.ut_line);
	}

	if (lflag)
		/* print when the user logged in */
		prtat(&utmp.ut_time);

	/* print idle time */
	if (idle >= 36 * 60)
		printf("%2ddays ", (idle + 12 * 60) / (24 * 60));
	else
		prttime(idle," ");

	if (lflag) {
		/* print CPU time for all processes & children */
		prttime(jobtime," ");
		/* print cpu time for interesting process */
		prttime(proctime," ");
	}

	/* what user is doing, either command tail or args */
	printf(" %-.32s\n",doing);
	fflush(stdout);
}

/* find & return number of minutes current tty has been idle */
time_t
findidle()
{
	struct stat stbuf;
	time_t lastaction, diff;
	char ttyname[20];

	strcpy(ttyname, "/dev/");
	strncat(ttyname, utmp.ut_line, LMAX);
	stat(ttyname, &stbuf);
	time(&now);
	lastaction = stbuf.st_atime;
	diff = now - lastaction;
	diff = DIV60(diff);
	if (diff < 0) diff = 0;
	return(diff);
}

#define	HR	(60 * 60)
#define	DAY	(24 * HR)
#define	MON	(30 * DAY)

/*
 * prttime prints a time in hours and minutes or minutes and seconds.
 * The character string tail is printed at the end, obvious
 * strings to pass are "", " ", or "am".
 */
prttime(tim, tail)
	time_t tim;
	char *tail;
{

	if (tim >= 60) {
		printf("%3d:", tim/60);
		tim %= 60;
		printf("%02d", tim);
	} else if (tim > 0)
		printf("    %2d", tim);
	else
		printf("      ");
	printf("%s", tail);
}

char *weekday[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char *month[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* prtat prints a 12 hour time given a pointer to a time of day */
prtat(time)
	long *time;
{
	struct tm *p;
	register int hr, pm;

	p = localtime(time);
	hr = p->tm_hour;
	pm = (hr > 11);
	if (hr > 11)
		hr -= 12;
	if (hr == 0)
		hr = 12;
	if (now - *time <= 18 * HR)
		prttime(hr * 60 + p->tm_min, pm ? "pm" : "am");
	else if (now - *time <= 7 * DAY)
		printf(" %s%2d%s", weekday[p->tm_wday], hr, pm ? "pm" : "am");
	else
		printf(" %2d%s%2d", p->tm_mday, month[p->tm_mon], p->tm_year);
}

/*
 * readpr finds and reads in the array pr, containing the interesting
 * parts of the proc and user tables for each live process.
 */
readpr()
{
	register struct proc *mproc;
	register struct user *up;
	struct sess sess;

	/*
	 * allocate proc table
	 */
	readsym(X_NPROC, &nproc, sizeof(nproc));
#ifdef sun
	readsym(X_NRCONSDEV, &rconsdev, sizeof(rconsdev));
#endif
	pr = (struct pr *)calloc(nproc, sizeof (struct pr));
	np = 0;
	while ((mproc = kvm_nextproc(kd)) != NULL) {
		/* decide if it's an interesting process */
		if (mproc->p_stat==0 || mproc->p_stat==SZOMB ||
		    mproc->p_pgrp==0)
			continue;

		/* read in session info */
		if (kvm_read(kd, mproc->p_sessp, &sess, sizeof(sess)) != 
		    sizeof(sess))
			continue;
		/* if no tty or a system process */
		if (!sess.s_ttyp  ||  sess.s_sid == SESS_SYS)
			continue;
		/* read in the user structure */
		if ((up = kvm_getu(kd, mproc)) == NULL)
			continue;
		/* save the interesting parts */
		pr[np].w_pid = mproc->p_pid;
		pr[np].w_flag = mproc->p_flag;
		pr[np].w_size = mproc->p_dsize + mproc->p_ssize;
		pr[np].w_igintr = (((int)up->u_signal[2]==1) +
		    2*((int)up->u_signal[2]>1) + 3*((int)up->u_signal[3]==1)) +
		    6*((int)up->u_signal[3]>1);
		pr[np].w_time =
		    up->u_ru.ru_utime.tv_sec + up->u_ru.ru_stime.tv_sec;
		pr[np].w_ctime =
		    up->u_cru.ru_utime.tv_sec + up->u_cru.ru_stime.tv_sec;
#ifdef sun
		if (sess.s_ttyd == rconsdev)
			sess.s_ttyd = makedev(0, 0);	/* "/dev/console" */
#endif
		pr[np].w_tty = sess.s_ttyd;
		pr[np].w_uid = mproc->p_uid;
		strcpyn(pr[np].w_comm, up->u_comm, sizeof (pr->w_comm));
		/*
		 * Get args if there's a chance we'll print it.
		 */
		pr[np].w_args[0] = 0;
		strcat(pr[np].w_args, getargs(mproc, up));
		if (pr[np].w_args[0] == 0 ||
		    pr[np].w_args[0] == '-' && pr[np].w_args[1] <= ' ' ||
		    pr[np].w_args[0] == '?') {
			strcat(pr[np].w_args, " (");
			strcat(pr[np].w_args, pr[np].w_comm);
			strcat(pr[np].w_args, ")");
		}
		np++;
	}
}

/*
 * strcpyn: copy a string of, at most, n characters without blank-padding.
 * asciz termination is guaranteed.  'n' had better not be zero.
 */
strcpyn(s1, s2, n)
char *s1, *s2;
register int n;
{
	while (n-- > 0)
		if ((*s1++ = *s2++) == '\0')
			return;
	*--s1 = '\0';
}

/*
 * readsym: get the value of a symbol from the namelist.
 * exit if error.
 */
readsym(n, b, s)
int n;
char *b;
int s;
{
	if (nl[n].n_type == 0) {
		fprintf(stderr, "%s: '%s' not in namelist\n",
		    prog, nl[n].n_name);
		exit(1);
	}
	if (kvm_read(kd, nl[n].n_value, b, s) != s) {
		fprintf(stderr, "%s: kernel read error\n", prog);
		exit(1);
	}
}

/*
 * getargs: given pointers to a proc & user structure, construct an
 * argument string that is no longer than ARGWIDTH.  Return an empty string
 * if the arguments are unreadable.
 */
char *
getargs(p, u)
struct proc *p;
struct user *u;
{
	char **argv, **av;
	register int sz = 0;
	register int err = 0;
	unsigned char c;
	static char argbuf[ARGWIDTH+1];
#define asiz (sizeof (argbuf))

	if (kvm_getcmd(kd, p, u, &argv, NULL) == -1)
		return("");
	av = argv;

	/* gather a string of arguments */
	argbuf[0] = '\0';
	while ((*argv != NULL) && (sz < (asiz - 1))) {
		strcpyn(&argbuf[sz], *argv++, (asiz - 1 - sz));
		sz = strlen(argbuf) + 1;
		if (sz < asiz) {
			argbuf[sz-1] = ' ';
			argbuf[sz] = '\0';
		}
	}
	/* get rid of unsavory characters */
	for (sz = 0; ; sz++) {
		if ((c = argbuf[sz]) == '\0')
			break;
		if (!isprint (c)) {
			if (err++ > 5) {
				argbuf[0] = '\0';
				break;
			}
			argbuf[sz] = '?';
		}
	}

	free(av);		/* release the malloc'ed arguments */
	return (argbuf);
}
