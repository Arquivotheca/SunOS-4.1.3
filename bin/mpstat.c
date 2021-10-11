/*
 * mpstat - multiprocessor status
 */

#ifndef lint
static	char sccsid[] = "@(#)mpstat.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <signal.h>
#include <kvm.h>

#include <sys/param.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/dk.h>
#include <sys/termios.h>
#include <sundev/mbvar.h>
#include <machine/param.h>

struct nlist    unl[] = {
	{"_panic_cpu" },
#define X_CPUID		0
	{ "" },
};

struct nlist    nl[] = {
	{"_ncpu"},
#define X_NCPU		0
	{"_cp_time"},
#define	X_CP_TIME	1
	{"_mp_time"},
#define	X_MP_TIME	2
	{"_hz"},
#define	X_HZ		3
	{"_procset"},
#define X_PROCSET	4

	{0},
};

#define	PERCENT		100.	/* scale factor fraction to percent */
#define LINES4HDR	19	/* default number of lines to emit after
				 * header */
#define TOGGLE(x)	(!(x) & 01)

kvm_t          *kd;		/* kvm descriptor  */
int             hz;
int             tohdr = 1;
int             window_rows;
char           *cmdname;	/* name by which command was invoked */
int             iter;
int             interval = -1;
int             toggle;

int             ncpu;
int             procset;

long            cp_time[3][CPUSTATES];
long            mp_time[3][NCPU][CPUSTATES];

void            printhdr();
char           *calloc();
off_t           lseek();
char           *sprintf();
char           *rindex();
void            getwinsize();

#define setbasename(target, source) \
do {char *s = source, *t = rindex(s,'/'); target = t ? t + 1 : s;} while (0)

#define KVM_GET(ndx, variable) \
	if (kvm_read(kd, (u_long)nl[ndx].n_value, \
		(char *)&variable, sizeof (variable)) != sizeof(variable)) \
			fatal("symbol lookup failed on ", nl[ndx].n_name);


main(argc, argv)
	char           *argv[];
{
	extern char    *ctime();
	int		fnd;

	setbasename(cmdname, *argv);
	argc--, argv++;

	while (argc && **argv == '-') {
		register char  *cp;

		for (cp = *argv + 1; *cp; cp++)
			switch (*cp) {
			default:
				usage();
			}
		argc--, argv++;
	}

	if (argc > 0) {
		interval = atoi(argv[0]);
		if (interval < 0)
			fatal("negative interval", 0);
		argc--, argv++;
	}
	if (argc > 0) {
		iter = atoi(argv[0]);
		if (iter == 0)
			exit(0);
		if (iter < 0)
			fatal("negative count", 0);
		argc--, argv++;
	}
	if (argc != 0)
		usage();


	kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "mpstat");
	if (kd == NULL) {
		perror("open");
		exit(1);
	}
	fnd = kvm_nlist(kd, unl);
	if (fnd != 0) {
		fprintf(stderr,"running on kernel with options UNIPROCESSOR\n");
		exit(1);
	}
	fnd = kvm_nlist(kd, nl);
	if (fnd == -1) {
		fprintf(stderr,"couldn't read name list\n");
		exit(1);
	} else if (fnd > 0) {
		fprintf(stderr,"couldn't read %d symbols in name list\n",
			fnd);
		exit(1);
	}

	KVM_GET(X_HZ, hz);
	KVM_GET(X_NCPU, ncpu);
	KVM_GET(X_PROCSET, procset);

	getwinsize();
	signal(SIGCONT, printhdr);
	signal(SIGWINCH, getwinsize);

	scan();			/* this is the main loop routine */
	exit(0);
}

scan()
{
	register        cpunum;

	/*
	 * MAIN LOOP
	 */
	for (;;) {
		if (--tohdr == 0)
			printhdr();

		toggle = TOGGLE(toggle);
		kvm_read(kd, (u_long) nl[X_CP_TIME].n_value,
			 cp_time[toggle], sizeof(cp_time[0]));
		kvm_read(kd, (u_long) nl[X_MP_TIME].n_value,
			 mp_time[toggle], sizeof(mp_time[0]));

		/*
		 * We start with -1 because it is the magic cookie that is
		 * associated with the *average* cpu data, as opposed to
		 * the per processor data.
		 */
		for (cpunum = -1; cpunum < NCPU; cpunum++)
			dodata(cpunum);

		printf("\n");
		fflush(stdout);

		if (--iter == 0 || interval == -1)
			exit(0);
		if (iter < 0)
			iter = -1;	/* so iter doesn't wrap around */
		if (interval > 0)
			sleep(interval);
	}
}

/*
 * Process and print the cpu data for cpu cpunum.  If cpunum is -1, print the
 * data held in cp_time, the average cpu time data.
 */

dodata(cpunum)
	int             cpunum;
{
	register        i;
	double          etime;	/* elapsed time */

	etime = 0.0;

	/*
	 * Handle average cpu data (held in cp_time) here.
	 */
	if (cpunum == -1) {	/* cp_time? */
		for (i = 0; i < CPUSTATES; i++) {
			/*
			 * Store the differences in the 3rd element of the
			 * array.
			 */
			cp_time[2][i] =
				cp_time[toggle][i] - cp_time[TOGGLE(toggle)][i];
			etime += cp_time[2][i];
		}
		if (etime == 0.0)
			etime = 1.0;
		for (i = 0; i < CPUSTATES; i++) {
			printf("%3.0f", PERCENT * cp_time[2][i] / etime);
		}
		return;
	}
	if (!(procset & (1 << cpunum)))
		return;
	/*
	 * Handle per cpu data (held in mp_time) here.
	 */
	for (i = 0; i < CPUSTATES; i++) {
		/*
		 * Store the differences in the 3rd element of the array.
		 */
		mp_time[2][cpunum][i] =
			mp_time[toggle][cpunum][i] -
			mp_time[TOGGLE(toggle)][cpunum][i];
		etime += mp_time[2][cpunum][i];
	}
	if (etime == 0.0)
		etime = 1.0;
	putchar(' ');
	for (i = 0; i < CPUSTATES; i++) {
		printf("%3.0f", PERCENT * mp_time[2][cpunum][i] / etime);
	}
}

void
printhdr()
{
	register        i;

	for (i = -1; i < NCPU; i++) {
		if (i == -1) {
			printf(" average      ");
			continue;
		}
		if (procset & (1 << i))
			printf("cpu%d         ", i);
	}
	printf("\n");
	for (i = -1; i < ncpu; i++)
		printf("%s us ni sy id", (i > -1) ? " " : "");
	printf("\n");

	tohdr = window_rows;
}



/* issue fatal message(s) and exit */
fatal(s1, s2)
	char           *s1, *s2;
{
	if (s1) {
		fprintf(stderr, "%s: %s", cmdname, s1);
		if (s2)
			fprintf(stderr, "%s", s2);
	} else
		fprintf(stderr, "%s: fatal error", cmdname);

	fprintf(stderr, "\n");
	exit(2);
}

/* print out the usage line and quit */
usage()
{
	fprintf(stderr,"usage: %s [interval [count]]\n",cmdname);
	exit(1);
}

void
getwinsize()
{
	struct winsize  win;

	if (ioctl(0, TIOCGWINSZ, &win) == -1 || win.ws_row < 5)
		window_rows = LINES4HDR;
	else
		window_rows = win.ws_row - 3;
	tohdr = 1;
}
