#ifndef lint
static  char sccsid[] = "@(#)mktp.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*
 *	mktp - driver for make tape utility programs
 */

#include "mktp.h"
#include <curses.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

extern char *malloc(), *sprintf(), *strcpy(), *strcat();

static int toc_only = 0;       /* !0 = no human intervention, just make toc */
static char expert = 0;
static char *ifile;
static char *xfile;

static int curstate;

#define	NIL	0	/* Quit state	*/
#define	PSTATE	1	/* parsed state	*/
#define	SSTATE	2	/* sized state	*/
#define	FSTATE	3	/* fitted state	*/
#define	ESTATE	4	/* edit state	*/
#define	DSTATE	5	/* distributed state	*/

#define	EOS	((char) 0x7f)

/*
 * First char is the istate, all following chars are legal
 * transitions. Nil means self-transition.
 */

static char t1[] = { NIL, NIL, EOS };
static char t2[] = { PSTATE, SSTATE, NIL, EOS };
static char t3[] = { SSTATE, PSTATE, FSTATE, NIL, EOS };
static char t4[] = { FSTATE, PSTATE, SSTATE, ESTATE, DSTATE, NIL, EOS };
static char t5[] = { ESTATE, PSTATE, SSTATE, FSTATE, DSTATE, NIL, EOS };
static char t6[] = { DSTATE, PSTATE, SSTATE, ESTATE, FSTATE, NIL, EOS };
static char *trans_tab[6] = { t1, t2, t3, t4, t5, t6 };

static char nchoices[6] = { 1, 3, 4, 6, 6, 6 };


static char *
cstrings[6] =
{
	"Quit",
	"Parse Prototype File (destroys Size, Fit and Edit)",
	"Size Entries (may take a long time; destroys Fit and Edit)",
	"Perform initial Fit of Entries to Device (destroys Edit)",
	"Edit Table of Contents",
	"Make Distribution (may take a long time)",
};


main(argc,argv)
int argc;
char **argv;
{
	time_t mdate();
	register getstate, newstate;

	if(argc > 3 || argc < 2)
	{
usage:
                (void) fprintf(stderr,"Usage: %s [-e|-t] infile\n",*argv);
		return(1);
	}
	else if(argc == 3)
	{
                if(argv[1][0] == '-') {
                        if (argv[1][2] != '\0')
                                goto usage;
                        if (argv[1][1] == 'e')
                                expert = 1;
                        else if (argv[1][1] == 't')
                                toc_only = 1;
                        else
                                goto usage;
                } else
			goto usage;
		argv++;
	}

	ifile = argv[1];
	xfile = malloc((unsigned)strlen(ifile)+4+1);
	(void) strcat(strcpy(xfile,ifile),".xdr");

	if(access(ifile,R_OK) < 0)
	{
		(void) fprintf(stderr,"Cannot access prototype file '%s'\n",
			ifile);
		return(1);
	}
	else if(access(xfile,F_OK) < 0)
	{
		if(do_parse_input() < 0)
			return(-1);
	}

        if (toc_only) {
                if (do_parse_input() < 0) {
                        (void) fprintf(stderr,"mktp: error while parsing %s\n",
                            ifile);
                        exit(1);
                }
                if (do_size() < 0) {
                        (void) fprintf(stderr,"mktp: error while sizeing %s\n",
                            ifile);
                        exit(1);
                }
                if(do_fit_file() != 0) {
                        (void) fprintf(stderr,"mktp: error during fit of %s\n",
                            ifile);
                        exit(1);
                }
                exit(0);
        }

	if(mdate(xfile) <= mdate(ifile))
	{
		(void) fprintf(stdout,
			"Warning: Prototype file '%s' newer than workfile\n",
			ifile);
		(void) fprintf(stdout,"Re-parse of input file recommended\n");
		(void) sleep(2);
	}

	for(getstate = 1;;)
	{
		if(setup(getstate) < 0)
			return(-1);
		getstate = 1;

		newstate = gchoices();

		switch(newstate)
		{
		case NIL:
			return(0);
		case PSTATE:
			if(do_parse_input() < 0)
				return(-1);
			curstate = newstate;
			break;
		case SSTATE:
			if(do_size() < 0)
				getstate = 0;
			else
				curstate = newstate;
			break;
		case ESTATE:
			getstate = 0;
			if(do_edittoc() == 0)
				curstate = newstate;
			break;
		case FSTATE:
			if(do_fit_file() == 0)
				curstate = newstate;
			break;
		case DSTATE:
			getstate = 0;
			if(do_make_distribution() == 0)
				curstate = newstate;
		}
	}

}



static time_t
mdate(f)
char *f;
{
	auto struct stat sb;

	if(stat(f,&sb) < 0)
	{
		(void) fprintf(stderr,"Could not stat file '%s'\n",f);
		exit(1);
	}
	return(sb.st_mtime);
}

static int
setup(findstate)
{
	static first = 1;
	register nentries;

	if(first)
		first = 0;
	else
		destroy_all_entries();

	if(!get_tables(xfile))
		return(-1);
	else if(findstate)
	{
		/*
		 * Now determine current state to enter, based upon
		 * dstate word.
		 */

		if(dinfo.dstate == PARSED)
			curstate = PSTATE;
		else if(dinfo.dstate == (SIZED|PARSED))
			curstate = SSTATE;
		else
			curstate = DSTATE;
	}
	return(0);
}



static int
gchoices()
{
	void (*isav)(), (*qsav)();
	int ok, isquit;
	register choice, newstate;
	register char *strans;

	(void) initscr();
        isav = signal(SIGINT,SIG_IGN);
        qsav = signal(SIGQUIT,SIG_IGN);
        nonl();         /* turn off CR-LF remap */
        crmode();       /* set cbreak...        */
        noecho();       /* turn off echoing     */

	clear();

	/*
	 * Put up basic info
	 */

	(void) mvprintw(0,20,"mktp version %s of %s","1.1","92/07/30");
	(void) mvprintw(2,14,"Prototype File: %s",ifile);
	(void) mvprintw(3,16,"Working File: %s",xfile);


	for(;;)
	{
		(void) mvprintw(5,30,"Choices");

		strans = trans_tab[curstate];
		for(choice = 1; *strans != EOS; choice++, strans++)
			(void) mvprintw(6+choice,10,"[%1d] %s",choice,
				cstrings[*strans]);

		(void) move(22,7);
		(void) clrtoeol();
		(void) printw("What would you like? ");

		refresh();

		if((choice = getch()) == ERR)
		{
			choice = NIL;
			break;
		}

		addch(choice);
		addch('\n');

		choice -= '0';

		if(choice < 1 || choice > nchoices[curstate])
		{
			if(choice+'0' > ' ' && choice+'0' < 0177)
				errprint("Illegal choice '%c'",choice+'0');
			else
				errprint("Illegal choice 0%o",choice+'0');

			infoprint("Try again........");
		}
		else
		{
			strans = trans_tab[curstate];
			newstate = (int) strans[choice-1];
			if(newstate == NIL)
				break;
			if(!expert)
			{
				(void) move(22,7);
				(void) clrtoeol();
				(void) printw(
					"Are you sure you want choice %d? ",
					choice);
				refresh();
				if((ok = getch()) == ERR)
				{
					choice = NIL;
					break;
				}
				addch(ok);
				addch('\n');
				if(ok == 'y' || ok == 'Y')
					break;
			}
			else
				break;
		}
	}

        (void) signal(SIGINT,isav);
        (void) signal(SIGQUIT,qsav);
        move(LINES,0);
        refresh();
        endwin();
	strans = trans_tab[curstate];
	newstate = (int) strans[choice-1];
	return(newstate);
}

#define	TMP	"mktp.tmp.xdr"

static int
do_parse_input()
{
	char buf[256];
	if(system(sprintf(buf,"parse_input %s %s",ifile,TMP)) != 0)
	{
		(void) unlink(TMP);
		(void) sleep(2);
		return(-1);
	}
	else
		return(copytmp());
}

static int
do_size()
{
	char buf[256];
	if(system(sprintf(buf,"calc_sizes -v %s %s",xfile,TMP)) != 0)
	{
		(void) unlink(TMP);
		(void) sleep(2);
		return(-1);
	}
	else
		return(copytmp());
}

static int
do_edittoc()
{
	char buf[256];
	if(system(sprintf(buf,"edittoc %s %s",xfile,TMP)) != 0)
	{
		(void) unlink(TMP);
		(void) sleep(2);
		return(-1);
	}
	else
		return(copytmp());
}

static int
do_fit_file()
{
	char buf[256];
	if(system(sprintf(buf,"fit_file -v %s %s",xfile,TMP)) != 0)
	{
		(void) unlink(TMP);
		(void) sleep(2);
		return(-1);
	}
	else
		return(copytmp());
}

static int
do_make_distribution()
{
	char buf[256];
	if(system(sprintf(buf,"make_distribution %s",xfile)) != 0)
	{
		(void) sleep(2);
		return(-1);
	}
	else
		return(0);
}

static
copytmp()
{
	char buf[256];
	if(system(sprintf(buf,"mv %s %s",TMP,xfile)) != 0)
	{
		(void) fprintf(stderr,"Oh no! Tragedy has struck\n");
		(void) sleep(2);
		return(-1);
	}
	else
		(void) unlink(TMP);
	return(0);
}
