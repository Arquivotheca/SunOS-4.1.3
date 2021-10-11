/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctcom.c 1.1 92/07/30 SMI"; /* from S5R3.1 acct:acctcom.c 1.27 */

#include	<time.h>
#include	<string.h>
#include	<sys/types.h>
#include	"acctdef.h"
#include	<grp.h>
#include	<stdio.h>
#include	<sys/acct.h>
#include	<pwd.h>
#include	<sys/stat.h>

#define MYKIND(flag)	((flag & ACCTF) == 0)
#define SU(flag)	((flag & ASU) == ASU)
#define PACCT		"/usr/adm/pacct"
#define MEANSIZE	01
#define KCOREMIN	02
#define HOGFACTOR	04
#define	SEPTIME		010
#define	CPUFACTOR	020
#define IORW		040
#define	pf(dble)	fprintf(stdout, "%8.2lf", dble)
#define	ps(s)		fprintf(stdout, "%8.8s", s)
#define	diag(string)		fprintf(stderr, "\t%s\n", string)

struct	acct ab;
char	command_name[16];
char	timestring[16];
char	obuf[BUFSIZ];

double	cpucut,
	syscut,
	hogcut,
	iocut,
	realtot,
	cputot,
	usertot,
	systot,
	kcoretot,
	iotot,
	rwtot;
long	timezone;
int	daylight;	/* daylight savings time if set */
long	daydiff,
	offset = -2,
	elapsed,
	sys,
	user,
	cpu,
	mem,
	io,
	rw,
	cmdcount;
time_t	tstrt_b,
	tstrt_a,
	tend_b,
	tend_a,
	etime;
int	backward,
	flag_field,
	average,
	quiet,
	option,
	verbose = 1,
	uidflag,
	gidflag,
	unkid,	/*user doesn't have login on this machine*/
	errflg,
	su_user,
	fileout = 0,
	stdinflg,
	nfiles;
dev_t	linedev	= -1;
uid_t	uidval,
	gidval;
char	*cname = NULL; /* command name pattern to match*/

struct passwd *getpwnam(), *pw;
struct group *getgrnam(),*grp;
long	ftell(),
	convtime(),
	tmsecs(),
	expand();
char	*ctime(),
	*ofile,
	*devtolin(),
	*uidtonam();
dev_t	lintodev();
extern	time_t time();
FILE	*ostrm;

static int pagesize;
static int kperpage;
static int pageperk;
#define	KCORE(pages)	(pagesize > 1024 ? \
			    (pages)*kperpage : \
			    (pages)/pageperk)

main(argc, argv)
char **argv;
{
	register int	c;
	extern int	optind;
	extern char	*optarg;

	pagesize = getpagesize();
	if (pagesize > 1024)
		kperpage = pagesize/1024;
	else
		pageperk = 1024/pagesize;

	setbuf(stdout,obuf);
	while((c = getopt(argc, argv,
		"C:E:H:I:O:S:abe:fg:hikl:mn:o:qrs:tu:v")) != EOF) {
		switch(c) {
		case 'C':
			sscanf(optarg,"%lf",&cpucut);
			continue;
		case 'O':
			sscanf(optarg,"%lf",&syscut);
			continue;
		case 'H':
			sscanf(optarg,"%lf",&hogcut);
			continue;
		case 'I':
			sscanf(optarg,"%lf",&iocut);
			continue;
		case 'a':
			average++;
			continue;
		case 'b':
			backward++;
			continue;
		case 'g':
			if(sscanf(optarg,"%hu",&gidval) == 1)
				gidflag++;
			else if((grp=getgrnam(optarg)) == NULL)
				fatal("Unknown group", optarg);
			else {
				gidval=grp->gr_gid;
				gidflag++;
			}
			continue;
		case 'h':
			option |= HOGFACTOR;
			continue;
		case 'i':
			option |= IORW;
			continue;
		case 'k':
			option |= KCOREMIN;
			continue;
		case 'm':
			option |= MEANSIZE;
			continue;
		case 'n':
			cname=(char *)cmset(optarg);
			continue;
		case 't':
			option |= SEPTIME;
			continue;
		case 'r':
			option |= CPUFACTOR;
			continue;
		case 'v':
			verbose=0;
			continue;
		case 'l':
			linedev = lintodev(optarg);
			continue;
		case 'u':
			if(*optarg == '?')
				unkid++;
			else if(*optarg == '#')
				su_user++;
			else if(sscanf(optarg, "%hu", &uidval) == 1)
				uidflag++;
			else if((pw = getpwnam(optarg)) == NULL)
				fprintf(stderr, "%s: Unknown user %s\n",
					argv[0], optarg);
			else {
				uidval = pw->pw_uid;
				uidflag++;
			}
			continue;
		case 'q':
			quiet++;
			verbose=0;
			average++;
			continue;
		case 's':
			tend_a = convtime(optarg);
			continue;
		case 'S':
			tstrt_a = convtime(optarg);
			continue;
		case 'f':
			flag_field++;
			continue;
		case 'e':
			tstrt_b = convtime(optarg);
			continue;
		case 'E':
			tend_b = convtime(optarg);
			continue;
		case 'o':
			ofile = optarg;
			fileout++;
			if((ostrm = fopen(ofile, "w")) == NULL) {
				perror("open error on output file");
				errflg++;
			}
			continue;
		case '?':
			errflg++;
			continue;
		}
	}

	if(errflg) {
		usage();
		exit(1);
	}

	for ( ; optind < argc; optind++){	/* Bug fix to allow acctcom */
		dofile(argv[optind]);		/* to properly process mult.*/
		nfiles++;			/* pacct file arguments.    */
	}

	if(nfiles==0) {
		if(isatty(0) || isdevnull())
			dofile(PACCT);
		else {
			stdinflg = 1;
			backward = offset = 0;
			dofile(NULL);
		}
	}
	doexit(0);
}

dofile(fname)
char *fname;
{
	register struct acct *a = &ab;
	long curtime;
	time_t	ts_a = 0,
		ts_b = 0,
		te_a = 0,
		te_b = 0;
	long	daystart;
	long	nsize;

	if(fname != NULL)
		if(freopen(fname, "r", stdin) == NULL) {
			fprintf(stderr, "acctcom: cannot open %s\n", fname);
			return;
		}

	if(backward) {
		backward = 0;
		if(aread() == 0)
			return;
		backward = 1;
		nsize = sizeof(struct acct);	/* make sure offset is signed */
		fseek(stdin, (long)(-nsize), 2);
	} else {
		if(aread() == 0)
			return;
		rewind(stdin);
	}
	tzset();
	daydiff = a->ac_btime - (a->ac_btime % SECSINDAY);
	daystart = (a->ac_btime-timezone) - ((a->ac_btime-timezone) % SECSINDAY);
	time(&curtime);
	if(daydiff < (curtime - (curtime % SECSINDAY))) {
		/*
		 * it is older than today
		 */
		fprintf(stdout,
			"\nACCOUNTING RECORDS FROM:  %s", ctime(&a->ac_btime));
	}

	if(tstrt_a) {
		ts_a = tstrt_a + daystart;
		if(daylight)
			ts_a -= 3600;
		fprintf(stdout, "START AFT: %s", ctime(&ts_a));
	}
	if(tstrt_b) {
		ts_b = tstrt_b + daystart;
		if(daylight)
			ts_b -= 3600;
		fprintf(stdout, "START BEF: %s", ctime(&ts_b));
	}
	if(tend_a) {
		te_a = tend_a + daystart;
		if(daylight)
			te_a -= 3600;
		fprintf(stdout, "END AFTER: %s", ctime(&te_a));
	}
	if(tend_b) {
		te_b = tend_b + daystart;
		if(daylight)
			te_b -= 3600;
		fprintf(stdout, "END BEFOR: %s", ctime(&te_b));
	}
	if(ts_a) {
		if (te_b && ts_a > te_b) te_b += SECSINDAY;
	}

	while(aread() != 0) {
		elapsed = expand(a->ac_etime);
		etime = a->ac_btime + (long)SECS(elapsed);
		if(ts_a || ts_b || te_a || te_b) {

			if(te_a && (etime < te_a)) {
				if(backward) return;
				else continue;
			}
			if(te_b && (etime > te_b)) {
				if(backward) continue;
				else return;
			}
			if(ts_a && (a->ac_btime < ts_a))
				continue;
			if(ts_b && (a->ac_btime > ts_b))
				continue;
		}
		if(!MYKIND(a->ac_flag))
			continue;
		if(su_user && !SU(a->ac_flag))
			continue;
		sys = expand(a->ac_stime);
		user = expand(a->ac_utime);
		cpu = sys + user;
		if(cpu == 0)
			cpu = 1;
		mem = expand(a->ac_mem);
		substr(a->ac_comm,command_name,0,8);
		io=expand(a->ac_io);
		rw=expand(a->ac_rw);
		if(cpucut && cpucut >= SECS(cpu))
			continue;
		if(syscut && syscut >= SECS(sys))
			continue;
		if(linedev != -1 && a->ac_tty != linedev)
			continue;
		if(uidflag && a->ac_uid != uidval)
			continue;
		if(gidflag && a->ac_gid != gidval)
			continue;
		if(cname && !cmatch(a->ac_comm,cname))
			continue;
		if(iocut && iocut > io)
			continue;
		if(unkid && uidtonam(a->ac_uid)[0] != '?')
			continue;
		if(verbose && (fileout == 0)) {
			printhd();
			verbose = 0;
		}
		if(elapsed == 0)
			elapsed++;
		if(hogcut && hogcut >= (double)cpu/(double)elapsed)
			continue;
		if(fileout)
			fwrite(&ab, sizeof(ab), 1, ostrm);
		else
			println();
		if(average) {
			cmdcount++;
			realtot += (double)elapsed;
			usertot += (double)user;
			systot += (double)sys;
			kcoretot += (double)mem;
			iotot += (double)io;
			rwtot += (double)rw;
		};
	}
}

aread()
{
	register flag;
	static	 ok = 1;

	if(fread((char *)&ab, sizeof(struct acct), 1, stdin) != 1)
		flag = 0;
	else
		flag = 1;

	if(backward) {
		if(ok) {
			if(fseek(stdin,
				(long)(offset*sizeof(struct acct)), 1) != 0) {
					rewind(stdin);	/* get 1st record */
					ok = 0;
			}
		} else
			flag = 0;
	}
	return(flag);
}

printhd()
{
	fprintf(stdout, "COMMAND                      START    END          REAL");
	ps("CPU");
	if(option & SEPTIME)
		ps("(SECS)");
	if(option & IORW){
		ps("CHARS");
		ps("BLOCKS");
	}
	if(option & CPUFACTOR)
		ps("CPU");
	if(option & HOGFACTOR)
		ps("HOG");
	if(!option || (option & MEANSIZE))
		ps("MEAN");
	if(option & KCOREMIN)
		ps("KCORE");
	fprintf(stdout, "\n");
	fprintf(stdout, "NAME       USER     TTYNAME  TIME     TIME       (SECS)");
	if(option & SEPTIME) {
		ps("SYS");
		ps("USER");
	} else
		ps("(SECS)");
	if(option & IORW) {
		ps("TRNSFD");
		ps("R/W");
	}
	if(option & CPUFACTOR)
		ps("FACTOR");
	if(option & HOGFACTOR)
		ps("FACTOR");
	if(!option || (option & MEANSIZE))
		ps("SIZE(K)");
	if(option & KCOREMIN)
		ps("MIN");
	if(flag_field)
		fprintf(stdout, "  F STAT");
	fprintf(stdout, "\n");
	fflush(stdout);
}

println()
{

	char name[32];
	register struct acct *a = &ab;

	if(quiet)
		return;
	if(!SU(a->ac_flag))
		strcpy(name,command_name);
	else {
		strcpy(name,"#");
		strcat(name,command_name);
	}
	fprintf(stdout, "%-9.9s", name);
	strcpy(name,uidtonam(a->ac_uid));
	if(*name != '?')
		fprintf(stdout, "  %-8.8s", name);
	else
		fprintf(stdout, "  %-8d",a->ac_uid);
	fprintf(stdout, " %-8.8s",a->ac_tty != -1? devtolin(a->ac_tty):"?");
	fprintf(stdout, "%.9s", &ctime(&a->ac_btime)[10]);
	fprintf(stdout, "%.9s ", &ctime(&etime)[10]);
	pf((double)SECS(elapsed));
	if(option & SEPTIME) {
		pf((double)sys / HZ);
		pf((double)user / HZ);
	} else
		pf((double)cpu / HZ);
	if(option & IORW)
		fprintf(stdout, "%8ld%8ld",io,rw);
	if(option & CPUFACTOR)
		pf((double)user / cpu);
	if(option & HOGFACTOR)
		pf((double)cpu / elapsed);
	if(!option || (option & MEANSIZE))
		pf(KCORE(mem / cpu));
	if(option & KCOREMIN)
		pf(MINT(KCORE(mem)));
	if(flag_field)
		fprintf(stdout, "  %1o %3o", a->ac_flag, a->ac_stat);
	fprintf(stdout, "\n");
}

/*
 * convtime converts time arg to internal value
 * arg has form hr:min:sec, min or sec are assumed to be 0 if omitted
 */
long
convtime(str)
char *str;
{
	long	hr, min, sec;
	extern	time_t	time();

	min = sec = 0;

	if(sscanf(str, "%ld:%ld:%ld", &hr, &min, &sec) < 1) {
		fatal("acctcom: bad time:", str);
	}
	tzset();
	sec += (min*60);
	sec += (hr*3600);
	return(sec + timezone);
}

cmatch(comm, cstr)
register char	*comm, *cstr;
{

	char	xcomm[9];
	register i;

	for(i=0;i<8;i++){
		if(comm[i]==' '||comm[i]=='\0')
			break;
		xcomm[i] = comm[i];
	}
	xcomm[i] = '\0';

	return(regex(cstr,xcomm));
}

cmset(pattern)
register char	*pattern;
{

	if((pattern=(char *)regcmp(pattern,0))==NULL){
		fatal("pattern syntax", NULL);
	}

	return((unsigned)pattern);
}

doexit(status)
{
	if(!average)
		exit(status);
	if(cmdcount) {
		fprintf(stdout, "\cmds=%ld ",cmdcount);
		fprintf(stdout, "Real=%-6.2f ",SECS(realtot)/cmdcount);
		cputot = systot + usertot;
		fprintf(stdout, "CPU=%-6.2f ",SECS(cputot)/cmdcount);
		fprintf(stdout, "USER=%-6.2f ",SECS(usertot)/cmdcount);
		fprintf(stdout, "SYS=%-6.2f ",SECS(systot)/cmdcount);
		fprintf(stdout, "CHAR=%-8.2f ",iotot/cmdcount);
		fprintf(stdout, "BLK=%-8.2f ",rwtot/cmdcount);
		fprintf(stdout, "USR/TOT=%-4.2f ",usertot/cputot);
		fprintf(stdout, "HOG=%-4.2f ",cputot/realtot);
		fprintf(stdout, "\n");
	}
	else
		fprintf(stdout, "\nNo commands matched\n");
	exit(status);
}
isdevnull()
{
	struct stat	filearg;
	struct stat	devnull;

	if(fstat(0,&filearg) == -1) {
		fprintf(stderr,"acctcom: cannot stat stdin\n");
		return(NULL);
	}
	if(stat("/dev/null",&devnull) == -1) {
		fprintf(stderr,"acctcom: cannot stat /dev/null\n");
		return(NULL);
	}

	if(filearg.st_rdev == devnull.st_rdev) return(1);
	else return(NULL);
}

fatal(s1, s2)
char *s1, *s2;
{
	fprintf(stderr,"acctcom: %s %s\n", s1, s2);
	exit(1);
}

usage()
{
	fprintf(stderr, "Usage: acctcom [options] [files]\n");
	fprintf(stderr, "\nWhere options can be:\n");
	diag("-b	read backwards through file");
	diag("-f	print the fork/exec flag and exit status");
	diag("-h	print hog factor (total-CPU-time/elapsed-time)");
	diag("-i	print I/O counts");
	diag("-k	show total Kcore minutes instead of memory size");
	diag("-m	show mean memory size");
	diag("-r	show CPU factor (user-time/(sys-time + user-time))");
	diag("-t	show separate system and user CPU times");
	diag("-v	don't print column headings");
	diag("-a	print average statistics of selected commands");
	diag("-q	print average statistics only");
	diag("-l line	\tshow processes belonging to terminal /dev/line");
	diag("-u user	\tshow processes belonging to user name or user ID");
	diag("-u #	\tshow processes executed by super-user");
	diag("-u ?	\tshow processes executed by unknown UID's");
	diag("-g group	show processes belonging to group name of group ID");
	diag("-s time	\tshow processes ending after time (hh[:mm[:ss]])");
	diag("-e time	\tshow processes starting before time");
	diag("-S time	\tshow processes starting after time");
	diag("-E time	\tshow processes ending before time");
	diag("-n regex	select commands matching the ed(1) regular expression");
	diag("-o file	\tdo not print, put selected pacct records into file");
	diag("-H factor	show processes that exceed hog factor");
	diag("-O sec	\tshow processes that exceed CPU system time sec");
	diag("-C sec	\tshow processes that exceed total CPU time sec");
	diag("-I chars	show processes that transfer more than char chars");
}
