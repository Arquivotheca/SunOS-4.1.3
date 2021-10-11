/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)uusched.c 1.1 92/07/30"	/* from SVR3.2 uucp:uusched.c 2.5 */

#include	"uucp.h"

#define USAGE	"[-xNUM] [-uNUM]"

struct m {
	char	mach[15];
	int	ccount;
} M[UUSTAT_TBL+2];

short Uopt;
void cleanup();

void logent(){}		/* to load ulockf.c */

main(argc, argv, envp)
char *argv[];
char **envp;
{
	struct m *m, *machine();
	DIR *spooldir, *subdir;
	char *str, *rindex();
	char f[256], subf[256];
	short num, snumber;
	char lckname[MAXFULLNAME];
	int i, maxnumb;
	FILE *fp;

	Uopt = 0;
	Env = envp;

	(void) strcpy(Progname, "uusched");
	while ((i = getopt(argc, argv, "u:x:")) != EOF) {
		switch(i){
		case 'x':
			Debug = atoi(optarg);
			if (Debug <= 0) {
				fprintf(stderr,
				"WARNING: %s: invalid debug level %s ignored, using level 1\n",
				Progname, optarg);
				Debug = 1;
			}
#ifdef SMALL
			fprintf(stderr,
			"WARNING: uusched built with SMALL flag defined -- no debug info available\n");
#endif /* SMALL */
			break;
		case 'u':
			Uopt = atoi(optarg);
			if (Uopt <= 0) {
				fprintf(stderr,
				"WARNING: %s: invalid debug level %s ignored, using level 1\n",
				Progname, optarg);
				Uopt = 1;
			}
			break;
		default:
			(void) fprintf(stderr, "\tusage: %s %s\n",
			    Progname, USAGE);
			cleanup(1);
		}
	}
	if (argc != optind) {
		(void) fprintf(stderr, "\tusage: %s %s\n", Progname, USAGE);
		cleanup(1);
	}

	DEBUG(9, "Progname (%s): STARTED\n", Progname);
	fp = fopen(LMTUUSCHED, "r");
	if (fp == NULL) {
		DEBUG(1, "No limitfile - %s\n", LMTUUSCHED);
	} else {
		(void) fscanf(fp, "%d", &maxnumb);
		(void) fclose(fp);
		DEBUG(4, "Uusched limit %d -- ", maxnumb);

		for (i=0; i<maxnumb; i++) {
		    (void) sprintf(lckname, "%s.%d", S_LOCK, i);
		    if ( ulockf(lckname, (time_t)  X_LOCKTIME) == 0)
			break;
		}
		if (i == maxnumb) {
		    DEBUG(4, "found %d -- cleanuping\n ", maxnumb);
		    cleanup(0);
		}
		DEBUG(4, "continuing\n", maxnumb);
	}

	if (chdir(SPOOL) != 0 || (spooldir = opendir(SPOOL)) == NULL)
		cleanup(101);		/* good old code 101 */
	while (gnamef(spooldir, f) == TRUE) {
	    if (EQUALSN("LCK..", f, 5))
		continue;

	    if (DIRECTORY(f) && (subdir = opendir(f))) {
	        while (gnamef(subdir, subf) == TRUE)
		    if (subf[1] == '.') {
		        if (subf[0] == CMDPRE) {
				/* Note - we can break now, but the
				 * count may be useful for enhanced
				 * scheduling
				 */
				machine(f)->ccount++;
			}
		    }
		closedir(subdir);
	    }
	}

	/* Make sure the overflow entry is null since it may be incorrect */
	M[UUSTAT_TBL].mach[0] = NULLCHAR;

	/* count the number of systems */
	for (num=0, m=M; m->mach[0] != '\0'; m++, num++) {
	    DEBUG(5, "machine: %s, ", M[num].mach);
	    DEBUG(5, "ccount: %d\n", M[num].ccount);
	}

	DEBUG(5, "Execute num=%d \n", num);
	while (num > 0) {
	    snumber = (time((long *) 0) % (long) (num));  /* random num */
	    (void) strcpy(Rmtname, M[snumber].mach);
	    DEBUG(5, "num=%d, ", num);
	    DEBUG(5, "snumber=%d, ", snumber);
	    DEBUG(5, "Rmtname=%s\n", Rmtname);
	    (void) sprintf(lckname, "%s.%s", LOCKPRE, Rmtname);
	    if (checkLock(lckname) != FAIL && callok(Rmtname) == 0) {
		/* no lock file and status time ok */
		DEBUG(5, "call exuucico(%s)\n", Rmtname);
		exuucico(Rmtname);
	    }
	    else {
		/* system locked - skip it */
		DEBUG(5, "system %s locked or inappropriate status--skip it\n",
		    Rmtname);
	    }
	    
	    M[snumber] = M[num-1];
	    num--;
	}
	cleanup(0);

	/* NOTREACHED */
}

struct m	*
machine(name)
char	*name;
{
	struct m *m;
	int	namelen;

	namelen = strlen(name);
	DEBUG(9, "machine(%s) called\n", name);
	for (m = M; m->mach[0] != '\0'; m++)
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
	if (m-M >= UUSTAT_TBL) {
	    if (m-M == UUSTAT_TBL) {
		errent("MACHINE TABLE FULL", "", UUSTAT_TBL,
		__FILE__, __LINE__);
	    }
	    else
		/* use the last entry - overwrite it */
		m = &M[UUSTAT_TBL];
	}

	(void) strcpy(m->mach, name);
	m->ccount = 0;
	return(m);
}

exuucico(name)
char *name;
{
	char cmd[BUFSIZ];
	int ret;
	char uopt[5];
	char sopt[BUFSIZ];

	(void) sprintf(sopt, "-s%s", name);
	if (Uopt)
	    (void) sprintf(uopt, "-x%.1d", Uopt);

	if (vfork() == 0) {
	    if (Uopt)
	        (void) execle(UUCICO, "UUCICO", "-r1", uopt, sopt, (char *) 0, Env);
	    else
	        (void) execle(UUCICO, "UUCICO", "-r1", sopt, (char *) 0, Env);

	    cleanup(100);
	}
	(void) wait(&ret);

	DEBUG(3, "ret=%d, ", ret);
}


void
cleanup(code)
int	code;
{
	rmlock(CNULL);
	exit(code);
}
