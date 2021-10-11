#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/08/05 SMI"; /* from S5R2 1.2 */
#endif

# include "defs"
/*
command make to update programs.
Flags:	'd'  print out debugging comments
	'p'  print out a version of the input graph
	's'  silent mode--don't print out commands
	'f'  the next argument is the name of the description file;
	     makefile is the default
	'i'  ignore error codes from the shell
	'S'  stop after any command fails (normally do parallel work)
	'n'   don't issue, just print, commands
	't'   touch (update time of) files but don't issue command
	'q'   don't do anything, but check if object is up to date;
	      returns exit code 0 if up to date, -1 if not
*/

char makefile[] = "makefile";
char Nullstr[] = "";
char Makefile[] =	"Makefile";
char RELEASE[] = "RELEASE";

NAMEBLOCK mainname ;
NAMEBLOCK firstname;
LINEBLOCK sufflist;
VARBLOCK firstvar;
PATTERN firstpat ;
OPENDIR firstod;


#ifdef unix
#include <signal.h>
void (*sigivalue)() = SIG_DFL;
void (*sigqvalue)() = SIG_DFL;
int waitpid=0;
#endif

int Mflags=MH_DEP;
int ndocoms=0;
int okdel=YES;

#ifdef BSD
CHARSTAR prompt="";	/* 4.2BSD, no prompt */
#else
CHARSTAR prompt="\t";	/* other systems -- pick what you want */
#endif
char junkname[20];
char funny[128];




char Makeflags[]="MAKEFLAGS";

main(argc,argv)
int argc;
CHARSTAR argv[];
{
	register NAMEBLOCK p;
	int i, j;
	int descset, nfargs;
	TIMETYPE tjunk;
	CHARSTAR s;
#ifdef unix
	void intrupt();
#endif
#ifdef sun
	int myexit();

	on_exit(myexit, 0);
#endif
#ifdef METERFILE
	meter(METERFILE);
#endif

	descset = 0;

	for(s = "#|=^();&<>*?[]:$`'\"\\\n" ; *s ; ++s)
		funny[*s] |= META;
	for(s = "\n\t :=;{}&>|" ; *s ; ++s)
		funny[*s] |= TERMINAL;
	funny['\0'] |= TERMINAL;

	TURNON(INTRULE);		/* Default internal rules, turned on */

/*
 *	Set command line flags
 */

	getmflgs();				/* Init $(MAKEFLAGS) variable */
	setflags(argc, argv);
#ifdef BSD
	{
		/*
		 * For compatibility with 4.2BSD make, set MFLAGS
		 * variable (like MAKEFLAGS but with leading '-'
		 * and not exported).
		 */
		char buf[20];

		strcpy(buf, "-");
		strcat(buf, varptr(Makeflags)->varval);
		setvar("MFLAGS", buf);
	}
#endif

	setvar("$","$");


/*
 *	Read command line "=" type args and make them readonly.
 */
	TURNON(INARGS|EXPORT);
	if(IS_ON(DBUG))(void)printf("Reading \"=\" type args on command line.\n");
	for(i=1; i<argc; ++i)
		if(argv[i]!=0 && argv[i][0]!=MINUS && (eqsign(argv[i]) == YES) )
			argv[i] = 0;
	TURNOFF(INARGS|EXPORT);

/*
 *	Read internal definitions and rules.
 */

	if( IS_ON(INTRULE) )
	{
		if(IS_ON(DBUG))(void)printf("Reading internal rules.\n");
		(void)rdd1((FILE *)NULL);
	}

/*
 *	Done with internal rules, now.
 */
	TURNOFF(INTRULE);

/*
 *	Read environment args.  Let file args which follow override.
 *	unless 'e' in MAKEFLAGS variable is set.
 */
	if( any( (varptr(Makeflags))->varval, 'e') )
		TURNON(ENVOVER);
	if(IS_ON(DBUG))(void)printf("Reading environment.\n");
	TURNON(EXPORT);
	readenv();
	TURNOFF(EXPORT|ENVOVER);

/*
 *	Read command line "-f" arguments.
 */

	rdmakecomm();

	for(i = 1; i < argc; i++)
		if( argv[i] && argv[i][0] == MINUS && argv[i][1] == 'f' && argv[i][2] == CNULL)
		{
			argv[i] = 0;
			if(i >= argc-1)
				fatal("No description argument after -f flag");
			if( rddescf(argv[++i], YES) )
				fatal1("Cannot open %s", argv[i]);
			argv[i] = 0;
			++descset;
		}


/*
 *	If no command line "-f" args then look for some form of "makefile"
 */
	if( !descset )
#ifdef unix
		if( rddescf(makefile, YES))
			(void)rddescf(Makefile, YES);

#endif
#ifdef gcos
		(void)rddescf(makefile, NO);
#endif


	if(IS_ON(PRTR)) printdesc(NO);

	if( srchname(".IGNORE") ) TURNON(IGNERR);
	if( srchname(".SILENT") ) TURNON(SIL);
	if(p=srchname(".SUFFIXES")) sufflist = p->linep;

#ifdef unix
	sigivalue = signal(SIGINT,SIG_IGN);
	sigqvalue = signal(SIGQUIT,SIG_IGN);
	enbint(intrupt);
#endif

	nfargs = 0;

	for(i=1; i<argc; ++i)
		if((s=argv[i]) != 0)
		{
			if((p=srchname(s)) == 0)
			{
				p = makename(s);
			}
			++nfargs;
			(void)doname(p, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

/*
	If no file arguments have been encountered, make the first
	name encountered that doesn't start with a dot
	*/

	if(nfargs == 0)
		if(mainname == 0)
			fatal("No arguments or description file");
		else
		{
			(void)doname(mainname, 0, &tjunk);
			if(IS_ON(DBUG)) printdesc(YES);
		}

	exit(0);
	/* NOTREACHED */
}



#ifdef unix
void intrupt()
{
	CHARSTAR p;
	NAMEBLOCK q;

	if(okdel && IS_OFF(NOEX) && IS_OFF(TOUCH) &&
	   (p=varptr("@")->varval) && 
	   (q=srchname(p)) && 
	   (exists(q)>0) &&
	   !isprecious(p) )
	{
		if(isdir(p))
			(void)fprintf(stderr, "\n*** %s NOT REMOVED.",p);
		else if(unlink(p) == 0)
			(void)fprintf(stderr, "\n***  %s removed.", p);
	}
	if(junkname[0])
		(void)unlink(junkname);
	(void)fprintf(stderr, "\n");
	exit(2);
}




isprecious(p)
CHARSTAR p;
{
	register NAMEBLOCK np;
	register LINEBLOCK lp;
	register DEPBLOCK dp;

	if(np = srchname(".PRECIOUS"))
	    for(lp = np->linep ; lp ; lp = lp->nextline)
		for(dp = lp->depp ; dp; dp = dp->nextdep)
			if(equal(p, dp->depname->namep))
				return(YES);

	return(NO);
}


enbint(k)
void (*k)();
{
	if(sigivalue != SIG_IGN)
		(void)signal(SIGINT,k);
	if(sigqvalue != SIG_IGN)
		(void)signal(SIGQUIT,k);
}
#endif

extern CHARSTAR builtin[];

CHARSTAR *linesptr=builtin;

FILE * fin;

rdmakecomm()
{
#ifdef PWB
	register char *nlog;
	char s[BUFSIZ];
	CHARSTAR concat(), getenv();

	if((nlog=getenv("HOME")) != NULL)
	{
		if(rddescf( concat(nlog,"/makecomm",s), NO))
			(void)rddescf( concat(nlog,"/Makecomm",s), NO);
	}

	if(rddescf("makecomm", NO))
		(void)rddescf("Makecomm", NO);
#endif
}

extern int yylineno;
extern CHARSTAR zznextc;
rddescf(descfile, flg)
CHARSTAR descfile;
int flg;			/* if YES try s.descfile */
{
	FILE * k;
	CHARSTAR descfile_sccs;
	NAMEBLOCK descfile_name, descfile_sccs_name;
	TIMETYPE descfile_time, descfile_sccs_time;

/* read and parse description */

	if(equal(descfile, "-"))
		return( rdd1(stdin) );

	if(flg)
	{
		/*
		 * If we may try the SCCS file, we check to see if the
		 * file does not exist or if it does exist but is out
		 * of date with respect to the SCCS file.  If either
		 * is the case, we must "get" the file; if that fails,
		 * we give up.
		 */
		descfile_sccs = (char *)malloc(strlen(descfile) + 32);
		if (descfile_sccs == NULL)
			fatal("Cannot allocate memory");
		strcpy(descfile_sccs, descfile);
		trysccs(descfile_sccs);

		descfile_name = makename(descfile);
		descfile_sccs_name = makename(descfile_sccs);
		descfile_time = exists(descfile_name);
		descfile_sccs_time = exists(descfile_sccs_name);

		if (descfile_sccs_time != 0)
		{
			if (descfile_time == 0 ||
			    descfile_time < descfile_sccs_time)
			{
				if(get(descfile, CD, varptr(RELEASE)->varval) == NO)
					return(1);
			}
		}
	}

	if( (k = fopen(descfile,"r")) != NULL)
	{
		if(IS_ON(DBUG))(void)printf("Reading %s\n", descfile);
		return( rdd1(k) );
	}
	return(1);
}




rdd1(k)
FILE * k;
{
	fin = k;
	yylineno = 0;
	zznextc = 0;

	if( yyparse() )
		fatal("Description file error");

	if(fin != NULL && fin != stdin)
		(void)fclose(fin);

	return(0);
}

printdesc(prntflag)
int prntflag;
{
	NAMEBLOCK p;
	VARBLOCK vp;
	OPENDIR od;

#ifdef unix
	if(prntflag)
	{
		(void)fprintf(stderr,"Open directories:\n");
		for(od=firstod; od!=0; od = od->nextopendir)
			(void)fprintf(stderr,"\t%d: %s\n",
			    od->dirfc ? od->dirfc->dd_fd : 0, od->dirn);
	}
#endif

	if(firstvar != 0) (void)fprintf(stderr,"Macros:\n");
	for(vp=firstvar; vp!=0; vp=vp->nextvar)
		if(vp->v_aflg == NO)
			(void)printf("%s = %s\n" , vp->varname , vp->varval);
		else
		{
			CHAIN pch;

			(void)fprintf(stderr,"Lookup chain: %s\n\t", vp->varname);
			for(pch = (CHAIN)vp->varval; pch; pch = pch->nextchain)
				(void)fprintf(stderr," %s",
					((NAMEBLOCK)pch->datap)->namep);
			(void)fprintf(stderr,"\n");
		}

	for(p=firstname; p!=0; p = p->nextname)
		prname(p, prntflag);
	(void)printf("\n");
	(void)fflush(stdout);
}

prname(p, prntflag)
register NAMEBLOCK p;
{
	register LINEBLOCK lp;
	register DEPBLOCK dp;
	register SHBLOCK sp;

	(void)fflush(stdout);
	if(p->linep != 0)
		(void)printf("\n\n%s:",p->namep);
	else
		(void)fprintf(stderr, "\n\n%s", p->namep);
	if(prntflag)
	{
		(void)fprintf(stderr,"  done=%d",p->done);
	}
	if(p==mainname) { (void)fflush(stdout); (void)fprintf(stderr,"  (MAIN NAME)");}
	for(lp = p->linep ; lp!=0 ; lp = lp->nextline)
	{
		if( dp = lp->depp )
		{
			(void)fflush(stdout);
			(void)fprintf(stderr,"\n depends on:");
			for(; dp!=0 ; dp = dp->nextdep)
				if(dp->depname != 0)
				{
					(void)printf(" %s", dp->depname->namep);
					(void)printf(" ");
				}
		}
		if(sp = lp->shp)
		{
			(void)printf("\n");
			(void)fflush(stdout);
			(void)fprintf(stderr," commands:\n");
			for( ; sp!=0 ; sp = sp->nextsh)
				(void)printf("\t%s\n", sp->shbp);
		}
	}
}


setflags(ac, av)
int ac;
CHARSTAR *av;
{
	register int i, j;
	register char c;
	int flflg=0;			/* flag to note `-f' option. */

	for(i=1; i<ac; ++i)
	{
		if(flflg)
		{
			flflg = 0;
			continue;
		}
		if(av[i]!=0 && av[i][0]==MINUS)
		{
			if(any(av[i], 'f'))
				flflg++;
			for(j=1 ; (c=av[i][j])!=CNULL ; ++j)
				optswitch(c);
			if(flflg)
				av[i] = "-f";
			else
				av[i] = 0;
		}
	}
}


/*
 *	Handle a single char option.
 */
optswitch(c)
register char c;
{

	switch(c)
	{

	case 'e':	/* environment override flag */
		setmflgs(c);
		break;

	case 'd':	/* debug flag */
		TURNON(DBUG);
		setmflgs(c);
		break;

	case 'p':	/* print description */
		TURNON(PRTR);
		break;

	case 's':	/* silent flag */
		TURNON(SIL);
		setmflgs(c);
		break;

	case 'i':	/* ignore errors */
		TURNON(IGNERR);
		setmflgs(c);
		break;

	case 'S':
		TURNOFF(KEEPGO);
		setmflgs(c);
		break;

	case 'k':
		TURNON(KEEPGO);
		setmflgs(c);
		break;

	case 'n':	/* do not exec any commands, just print */
		TURNON(NOEX);
		setmflgs(c);
		break;

	case 'r':	/* turn off internal rules */
		TURNOFF(INTRULE);
		break;

	case 't':	/* touch flag */
		TURNON(TOUCH);
		setmflgs(c);
		break;

	case 'q':	/* question flag */
		TURNON(QUEST);
		setmflgs(c);
		break;

	case 'g':	/* turn default $(GET) of files not found */
		TURNON(GET);
		setmflgs(c);
		break;

	case 'm':	/* print memory map */
		TURNON(MEMMAP);
		setmflgs(c);
		break;

	case 'b':	/* use MH version of test for whether a cmd exists */
		TURNON(MH_DEP);
		setmflgs(c);
		break;
	case 'B':	/* turn off -b flag */
		TURNOFF(MH_DEP);
		setmflgs(c);
		break;

	case 'f':	/* Named makefile; already handled by setflags(). */
		break;

	default:
		fatal1("Unknown flag argument %c", c);
	}
}

/*
 *	getmflgs() set the cmd line flags into an EXPORTED variable
 *	for future invocations of make to read.
 */


getmflgs()
{
	register VARBLOCK vpr;
	register CHARSTAR *pe;
	register CHARSTAR p;

	vpr = varptr(Makeflags);
	setvar(Makeflags, "ZZZZZZZZZZZZZZZZ");
	vpr->varval[0] = CNULL;
	vpr->envflg = YES;
	vpr->noreset = YES;
	optswitch('b');
	for(pe = environ; *pe; pe++)
	{
		if(sindex(*pe, "MAKEFLAGS=") == 0)
		{
			for(p = (*pe)+sizeof Makeflags; *p; p++)
				optswitch(*p);
			return;
		}
	}
}

/*
 *	setmflgs(c) sets up the cmd line input flags for EXPORT.
 */

setmflgs(c)
register char c;
{
	register VARBLOCK vpr;
	register CHARSTAR p;

	vpr = varptr(Makeflags);
	for(p = vpr->varval; *p; p++)
	{
		if(*p == c)
			return;
	}
	*p++ = c;
	*p = CNULL;
}
