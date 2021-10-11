#ifndef lint
static	char sccsid[] = "@(#)cc.c 1.1 92/07/30 SMI"; /* from UCB 4.7 83/07/01 */
#endif
/*
 * cc - front end for C compiler
 */
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/dir.h>
#include <strings.h>

extern 	char *	sys_siglist[];

char	*cpp = "/lib/cpp";
char	*count = "/usr/lib/bb_count";
char	*ccom = "/lib/ccom";
char	*inline = "/usr/lib/inline";
char	*c2 = "/lib/c2";
char	*as = "/bin/as";
char	*ld = "/bin/ld";
char	*libc = "-lc";
char	*crt0 = "crt0.o";
char	*gcrt0 = "gcrt0.o";
char	*mcrt0 = "mcrt0.o";

char	tmp0[30];		/* big enough for /tmp/ctm%05.5d */
char	*tmp1, *tmp2, *tmp3, *tmp4, *tmp5;
char	*outfile;
char	*asoutfile;
char	*savestr(), *strspl(), *setsuf();
char	*getsuf();
int	idexit();
char	**av, **clist, **llist, **plist;
int	cflag, eflag, oflag, pflag, sflag, wflag, Rflag, exflag, proflag;
int	aflag;
int	gproflag, gflag, Gflag;
int	vflag;
int	iflag;
int	Jflag;
int	chlibflag;
extern	char *malloc(), *calloc();

#define strequal(s1,s2) (strcmp((s1),(s2))==0)

#ifdef mc68000

/*
 * starting in release 3.0, we must figure out what kind of processor
 * we are running on, and generate code accordingly.  This requires
 * some magic routines from libc.a
 */
int	is68020();	/* returns 1 if the host is a 68020 */

struct mach_info { 
	char *optname; 
	int  found; 
	int  isatype;
	char *crt1;
};

struct mach_info machopts[] = {
	"-m68010",   0,	1,  (char*)0,	/* use 68010 subset */
	"-m68020",   0, 1,  (char*)0,	/* use 68020 extensions */
	(char*)0
};

struct mach_info floatopts[] = {
	"-ffpa",     0, 1,  "Wcrt1.o",	/* sun fpa */
	"-f68881",   0, 1,  "Mcrt1.o",	/* 68881 */
	"-fsky",     0, 1,  "Scrt1.o",	/* sky board */
	"-fsoft",    0, 1,  "Fcrt1.o",	/* software floating point */
	"-fswitch",  0, 1,  (char*)0,	/* switched floating point */
	"-fsingle",  0, 0,  (char*)0,	/* single precision float */
	"-fsingle2", 0, 0,  (char*)0,	/* pass float args as floats */
	"-fstore",   0, 0,  (char*)0,	/* coerce fp regs to storage format */
	(char *)0 ,
};

extern char *getenv();
char *FLOAT_OPTION = "FLOAT_OPTION";
struct mach_info *machtype=NULL;	/* selected target machine type */
struct mach_info *fptype=NULL;		/* selected floating pt machine type */
struct mach_info *default_machtype();	/* default target machine type */
struct mach_info *default_fptype();	/* default floating point machine */

#define M_68010  &machopts[0]
#define M_68020  &machopts[1]
#define F_fpa    &floatopts[0]
#define F_68881  &floatopts[1]
#define F_sky    &floatopts[2]
#define F_soft   &floatopts[3]
#define F_switch &floatopts[4]
#define F_store  &floatopts[5]

#define use68010  (machtype == M_68010)
#define use68020  (machtype == M_68020)

#define unsupported(machtype, fptype) \
	( machtype == M_68010 && fptype == F_fpa \
	|| machtype == M_68020 && fptype == F_sky )

#endif
char	*dflag;
int	exfail;
char	*chpass;
char	*npassname;
char	*ccname;

int	nc, nl, np, nxo, na;

main(argc, argv)
	register int argc;
	register char **argv;
{
	char *t;
	char *assource;
	char *cppout;
	char *cp;
	register int i, j, tmpi;
	register struct mach_info *mp;
	int optfound;

	/* ld currently adds upto 5 args; 10 is room to spare */
	av = (char **)calloc((unsigned)argc+10, sizeof (char **));
	clist = (char **)calloc((unsigned)argc, sizeof (char **));
	llist = (char **)calloc((unsigned)argc, sizeof (char **));
	plist = (char **)calloc((unsigned)argc, sizeof (char **));
	ccname = argv[0];
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') switch (argv[i][1]) {

		case 'S':
			sflag++;
			cflag++;
			continue;
		case 'o':
			if (++i < argc) {
				outfile = argv[i];
				cp = getsuf(outfile);
				if (strequal(cp, "c") || strequal(cp, "o")) {
					error("-o would overwrite %s",
					    outfile);
					exit(8);
				}
			}
			continue;
		case 'R':
			Rflag++;
			continue;
		case 'O':
			/*
			 * There might be further chars after -O; we just
			 * pass them on to c2 as an extra argument -- later.
			 */
			oflag++;
			continue;
		case 'J':
			Jflag++;
			continue;
		case 'p':
			proflag++;
			if (argv[i][2] == 'g'){
				crt0 = gcrt0;
				gproflag++;
			} else {
				crt0 = mcrt0;
			}
			continue;
		case 'g':
			if (argv[i][2] == 'o') {
			    Gflag++;	/* old format for -go */
			} else {
			    gflag++;	/* new format for -g */
			}
			continue;
		case 'w':
			wflag++;
			continue;
		case 'E':
			exflag++;
		case 'P':
			pflag++;
			plist[np++] = argv[i];
		case 'c':
			cflag++;
			continue;
		case 'D':
		case 'I':
		case 'U':
		case 'C':
			plist[np++] = argv[i];
			continue;
		case 't':
			if (chpass)
				error("-t overwrites earlier option", 0);
			chpass = argv[i]+2;
			if (chpass[0]==0)
				chpass = "012pialc";
			continue;
		case 'f':
#ifdef mc68000
			/*
			 * floating point option switches
			 */
			optfound = 0;
			for (mp = floatopts; mp->optname; mp++) {
			    if (!strcmp(argv[i], mp->optname)){
				if (mp->isatype) {
				    if (fptype != NULL && fptype != mp) {
					error("%s overwrites earlier option",
					    mp->optname);
				    }
				    fptype = mp;
				} else {
				    mp->found = 1;
				}
				optfound = 1;
				break;
			    }
			}
			if (!optfound) {
			    if (argv[i][2] == '\0') {
				fprintf(stderr,
				    "%s: warning: -f option is obsolete\n",
				    ccname);
			    } else {
				fprintf(stderr,
				    "%s: warning: %s option not recognized\n",
				    ccname, argv[i]);
			    }
			}
			continue;
#else !mc68000
			fprintf(stderr,
			    "%s: warning: -f option is obsolete\n",
			    ccname);
			continue;
#endif !mc68000

#ifdef	mc68000
		case 'm':
			optfound = 0;
			for (mp = machopts; mp->optname; mp++) {
			    if (!strcmp(argv[i], mp->optname)) {
				if (mp->isatype) {
				    if (machtype != NULL && machtype != mp) {
					error("%s overwrites earlier option",
					    mp->optname);
				    }
				    machtype = mp;
				} else {
				    mp->found = 1;
				}
				optfound = 1;
				break;
			    }
			}
			if (!optfound) {
			    fprintf(stderr,
				"%s: warning: %s option not recognized\n",
				ccname, argv[i]);
			}
			continue;
#endif	mc68000

		case 'B':
			if (npassname)
				error("-B overwrites earlier option", 0);
			npassname = argv[i]+2;
			if (npassname[0]==0)
				npassname = "/usr/c/o";
			continue;
		case 'd':
			dflag = argv[i];
			continue;
		case 'a':
			if (strcmp(argv[i]+1,"align") == 0) {
				if (i+1 >= argc || argv[i+1][0] == '-') {
					fprintf(stderr,
			    "%s: missing argument to -align option\n", ccname);
					continue;
				}
				llist[nl++] = "-align";
				llist[nl++] = argv[++i];
				continue;
			}
			aflag++;
			continue;
		case 'A':
			/*
			 * There might be further chars after -A; we just
			 * pass them on to c2 as an extra argument -- later.
			 */
			continue;
		case 'v':
			vflag++;
			continue;
		}
		t = argv[i];
		cp = getsuf(t);
		if (strequal(cp,"c") || strequal(cp,"s") || exflag) {
			clist[nc++] = t;
			t = setsuf(t, 'o');
		} else if (strequal(cp, "il")) {
			iflag++;
		}
		if (!strequal(cp, "il") && nodup(llist, t)) {
			llist[nl++] = t;
			if (strequal(getsuf(t),"o"))
				nxo++;
		}
	}
	if (outfile && cflag && nc == 1) {
		asoutfile = outfile;
	}
	if (gflag || Gflag) {
		if (oflag)
			fprintf(stderr, "%s: warning: -g disables -O\n", ccname);
		oflag = 0;
	}
#ifdef mc68000
	/*
	 * if no machine type specified, use the default
	 */
	if (machtype == NULL) {
		machtype = default_machtype();
	}
	/*
	 * if no floating point machine type specified, use the default
	 */
	if (fptype == NULL) {
		fptype = default_fptype(machtype);
	} else if (unsupported(machtype, fptype)) {
		t = fptype->optname;
		fptype = default_fptype(machtype);
		fprintf(stderr,
		    "%s: warning: %s option not supported with %s; %s used\n",
		    ccname, t, machtype->optname, fptype->optname);
	}
	machtype->found = 1;
	fptype->found = 1;
#endif mc68000

	if (npassname && chpass ==0)
		chpass = "012pialc";
	if (chpass && npassname==0)
		npassname = "/usr/new/";
	if (chpass)
	for (t=chpass; *t; t++) {
		switch (*t) {

		case '0':
		case '1':
			ccom = strspl(npassname, "ccom");
			continue;
		case '2':
			c2 = strspl(npassname, "c2");
			continue;
		case 'p':
			cpp = strspl(npassname, "cpp");
			continue;
		case 'i':
			inline = strspl(npassname, "inline");
			continue;
		case 'a':
			as = strspl(npassname, "as");
			continue;
		case 'l':
			ld = strspl(npassname, "ld");
			continue;
		case 'c':
			libc = strspl(npassname, "libc.a");
			chlibflag++;
			continue;
		}
	}
	if (nc==0)
		goto nocom;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, idexit);
	if (pflag==0)
		sprintf(tmp0, "/tmp/ctm%05.5d", getpid());
	/*
	 * rules of order:
	 *	1. The variables tmp[0-7] are set to the names of tmp files.
	 *	2. Do not set tmp[0-7] to names of other (non-temp) files
	 *	3. Unlink files when you're done with them.
	 *	4. Do not unlink anything except tmp[0-7] and the final
	 *	   .o in the case where a linked executable was produced
	 *	   and only one .[cs] file compiled.
	 */
	tmp1 = strspl(tmp0, "1");	/* default cpp output */
	tmp2 = strspl(tmp0, "2");	/* default bb_count output */
	tmp3 = strspl(tmp0, "3");	/* default inline input */
	tmp4 = strspl(tmp0, "4");	/* default c2 input */
	tmp5 = strspl(tmp0, "5");	/* default assembler input */
	for (i=0; i<nc; i++) {
		if (nc > 1) {
			printf("%s:\n", clist[i]);
			fflush(stdout);
		}
		if (strequal(getsuf(clist[i]), "s")) {
			assource = clist[i];
			goto assemble;
		} else {
			assource = tmp5;
		}
		if (pflag) {
			cppout = setsuf(clist[i], 'i');
		} else {
			cppout = tmp1;
		}
		av[0] = "cpp"; av[1] = clist[i]; av[2] = exflag ? "-" : cppout;
		na = 3;
		for (j = 0; j < np; j++)
			av[na++] = plist[j];
		av[na++] = 0;
		if (callsys(cpp, av)) {
			exfail++;
			eflag++;
		}
		if (pflag || exfail) {
			cflag++;
			continue;
		}
		/*
		 * Call the bb_count preprocessor
		 */
		if (aflag) {
			av[0] = "bb_count";
			av[1] = tmp1;
			av[2] = clist[i];
			av[3] = tmp2;
			av[4] = 0;
			if (callsys(count, av)) {
				exfail++;
				eflag++;
			}
			unlink(tmp1);
			if (pflag || exfail) {
				cflag++;
				continue;
			}
		}
		if (sflag)
			assource = setsuf(clist[i], 's');
		av[0] = "ccom"; 
		av[1] = aflag? tmp2: tmp1; 
		av[2] = iflag? tmp3: oflag? tmp4: assource; 
		na = 3;
		if (proflag)
			av[na++] = "-XP";
		if (Jflag)
			av[na++] = "-XJ";
		if (gflag) {
			av[na++] = "-Xg";
		} else if (Gflag) { 
			av[na++] = "-XG"; 
		}
		if (wflag)
			av[na++] = "-w";
#ifdef mc68000
		/* pass code gen options to ccom */
		for (mp = machopts; mp->optname; mp++) {
		    if (mp->found) {
			av[na++] = mp->optname;
		    }
		}
		for (mp = floatopts; mp->optname; mp++) {
		    if (mp->found) {
			av[na++] = mp->optname;
		    }
		}
#endif
		av[na] = 0;
		if (callsys(ccom, av)) {
			cflag++;
			eflag++;
			continue;
		}
		unlink(tmp1);
		unlink(tmp2);
		if (iflag) {
			/* run inline expansion */
			av[0] = "inline";
			av[1] = tmp3;
			av[2] = "-o";
			av[3] = oflag ? tmp4 : assource;
			na = 4;
			for (tmpi = 1; tmpi < argc; tmpi++) {
				/* pass .i files to inline expander */
				t = argv[tmpi];
				if (strequal(getsuf(t), "il")) {
					av[na++] = "-i";
					av[na++] = t;
				}
			}
			av[na] = 0;
			if (callsys(inline, av)) {
				cflag++;
				eflag++;
				continue;
			}
			unlink(tmp3);
		}
		if (oflag) {
			av[0] = "c2";
			av[1] = tmp4;
			av[2] = assource;
			na = 3;
			av[na++] = (use68020 ? "-20" : "-10");
			/* Pass -Oxxx arguments to optimizer */
			for (tmpi = 1; tmpi < argc; tmpi++) {
				if (argv[tmpi][0] == '-'
				 && argv[tmpi][1] == 'O'
				 && argv[tmpi][2] != '\0') {
					av[na++] = argv[tmpi]+2;
				}
			}
			av[na] = 0;
			if (callsys(c2, av)) {
				cflag++;
				eflag++;
				continue;
			}
			unlink(tmp4);
		}
		if (sflag)
			continue;
	assemble:
		av[0] = "as";
		av[1] = "-o";
		if (asoutfile) {
		    av[2] = asoutfile;
		} else {
		    av[2] = setsuf(clist[i], 'o');
		}
		na = 3;
		av[na++] = (use68020 ? "-20" : "-10");
		if (Rflag)
			av[na++] = "-R";
		if (dflag)
			av[na++] = dflag;
		/* Pass -Axxx arguments to assembler */
		for (tmpi = 1; tmpi < argc; tmpi++) {
			if (argv[tmpi][0] == '-'
			 && argv[tmpi][1] == 'A'
			 && argv[tmpi][2] != '\0') {
				av[na++] = argv[tmpi]+2;
			}
		}
		av[na++] = assource;
		av[na] = 0;
		if (callsys(as, av) > 1) {
			cflag++;
			eflag++;
			continue;
		}
	}
nocom:
	if (cflag==0 && nl!=0) {
		i = 0;
		na = 0;
		av[na++] = "ld";
		av[na++] = "-X";
		av[na++] = strspl(chlibflag? npassname : "/lib/", crt0);
		if (fptype->crt1 != NULL) {
			av[na++] = strspl(chlibflag? npassname : "/lib/",
				fptype->crt1);
		}
		if (outfile) {
			av[na++] = "-o";
			av[na++] = outfile;
		}
		while (i < nl)
			av[na++] = llist[i++];
		if (aflag) 
			av[na++] = "/usr/lib/bb_link.o";
		if (gflag || Gflag)
			av[na++] = "-lg";
		if (proflag)
			av[na++] = "-lc_p";
		else
			av[na++] = libc;
		av[na++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			unlink(setsuf(clist[0], 'o'));
	}
	dexit();
}

#ifdef mc68000

/*
 * default target machine type is the same as host
 */
struct mach_info *
default_machtype()
{
    return (is68020()? M_68020 : M_68010);
}

/*
 *  Floating point is such a zoo on this machine that
 *  nobody agrees what the default should be.  So let
 *  the user decide, and to hell with it.
 */
struct mach_info *
default_fptype(mtp)
    struct mach_info *mtp;
{
    register char *env_string;
    register struct mach_info *ftp;

    env_string = getenv(FLOAT_OPTION);
    if (env_string == NULL) {
	return (F_soft);
    }
    for(ftp = floatopts; ftp->isatype; ftp++) {
	if (!strcmp(ftp->optname+1, env_string)) {
	    if (unsupported(mtp, ftp)) {
		ftp = F_soft;
		fprintf(stderr,
		"%s: warning: FLOAT_OPTION=%s not supported with %s; %s used\n",
		    ccname, env_string, mtp->optname+1, ftp->optname+1);
	    }
	    return(ftp);
	}
    }
    ftp = F_soft;
    fprintf(stderr,
	"%s: warning: FLOAT_OPTION=%s not recognized; %s used\n",
	ccname, env_string, ftp->optname+1);
    return(ftp);
}

#endif mc68000

idexit()
{

	eflag = 100;
	dexit();
}

dexit()
{

	if (!pflag) {
		unlink(tmp1);
		unlink(tmp2);
		unlink(tmp3);
		unlink(tmp4);
		unlink(tmp5);
	}
	exit(eflag);
}

/*VARARGS1*/
error(s, x1, x2, x3, x4)
	char *s;
{
	FILE *diag = exflag ? stderr : stdout;

	fprintf(diag, "%s: ", ccname);
	fprintf(diag, s, x1, x2, x3, x4);
	putc('\n', diag);
	exfail++;
	cflag++;
	eflag++;
}

char *
getsuf(name)
char *name;
{
	char *suff;
	suff = rindex(name, '.');
	if (suff == NULL)
		return("");
	return suff+1;
}

char *
setsuf(as, ch)
	char *as;
{
	register char *s, *s1;

	s = s1 = savestr(as);
	while (*s)
		if (*s++ == '/')
			s1 = s;
	s[-1] = ch;
	return (s1);
}

callsys(f, v)
	char *f, **v;
{
	int t, status;

#ifdef DEBUG
	printf("fork %s:", f);
	for (t = 0; v[t]; t++) printf(" %s", v[t][0]? v[t]: "(empty)");
	printf("\n");
	return 0;
#else  DEBUG
	if (vflag){
		fprintf(stderr,"%s: ",f);
		for (t = 0; v[t]; t++) fprintf(stderr, " %s", v[t][0]? v[t]: "(empty)");
		fprintf(stderr, "\n");
	}
	fflush(stderr);	/* purge any junk before the vfork */
	t = vfork();
	if (t == -1) {
		fprintf( stderr, "%s: No more processes\n", ccname);
		return (100);
	}
	if (t == 0) {
		execv(f, v);
		/*
		 * We are now in The Vfork Zone, and can't use "fprintf".
		 * We use "write" and "_perror" instead.
		 */
		write(2, ccname, strlen(ccname));
		write(2, ": Can't execute ", 16);
		_perror(f);
		_exit(100);
	}
	while (t != wait(&status))
		;
	if ((t=(status&0377)) != 0 && t!=14) {
		if (t!=2) {
			fprintf( stderr, "%s: Fatal error in %s: %s%s\n", ccname, f, sys_siglist[t&0177], (t&0200)?" (core dumped)":"" );
			eflag = 8;
		}
		dexit();
	}
	return ((status>>8) & 0377);
#endif DEBUG
}

nodup(l, os)
	char **l, *os;
{
	register char *t, *s;
	register int c;

	s = os;
	if (!strequal(getsuf(s),"o"))
		return (1);
	while (t = *l++) {
		while (c = *s++)
			if (c != *t++)
				break;
		if (*t==0 && c==0)
			return (0);
		s = os;
	}
	return (1);
}

#define	NSAVETAB	1024
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = malloc((unsigned)saveleft);
		if (savetab == 0) {
			fprintf(stderr, "%s: ran out of memory (savestr)\n", ccname);
			exit(1);
		}
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

char *
strspl(left, right)
	char *left, *right;
{
	char buf[BUFSIZ];

	strcpy(buf, left);
	strcat(buf, right);
	return (savestr(buf));
}
