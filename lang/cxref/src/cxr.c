#ifndef lint
static	char sccsid[] = "@(#)cxr.c 1.1 92/07/30 SMI"; /* from S5R2 1.5 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "owner.h"

#ifdef FLEXNAMES
#define MAXWORD (BUFSIZ / 2)
#define MAXRFW MAXWORD
#define MAXRFL MAXWORD
#else
#define MAXWORD	100	/* maximum length of word */
#define MAXRFW	20
#define MAXRFL	50
#endif
#define MAX	BUFSIZ	/* maximum length of line */
#define MAXARG	60	/* maximum number of arguments */
#define MAXDEFS	20	/* maximum number of defines or undefines */
#define MAXINC	10	/* maximum number of include directories */
#define BASIZ	50	/* funny */
#define LPROUT	70	/* output */
#define TTYOUT	30	/* numbers */
#define YES	1
#define NO	0
#define LETTER	'a'
#define DIGIT	'0'
#define XCPP	"cpp"
#define XREF	"xpass"
#define SRT	"/usr/bin/sort"

extern char
	*calloc(),
	*strcat(),
	*strcpy(),
	*mktemp();

char	*xref,	/* path to the cross-referencer */
	*xcpp,	/* path to cxref pre-processor */
	*cfile,	/* current file name */
	*tmp1,	/* contains preprocessor result */
	*tmp2,	/* contains cpp and xpass cross-reference */
	*tmp3,	/* contains transformed tmp2 */
	*tmp4;	/* contains sorted output */

char	*clfunc = "   --   ";	/* clears function name for ext. variables */

char	*myalloc();

char	xflnm[MAXWORD],	/* saves current filename */
	funcname[MAXWORD],	/* saves current function name */
	sv1[MAXRFL],	/* save areas used for output printing */
	sv2[MAXRFL],
	sv3[MAXRFL],
	sv4[MAXRFL],
	buf[BUFSIZ];

char	*arv[MAXARG],
	*predefs[MAXDEFS],
	*preunds[MAXDEFS],
	*incdirs[MAXINC];

int	cflg = NO,	/* prints all given files as one cxref listing */
	silent = NO,	/* print filename before cxref? */
	tmpmade = NO,	/* indicates if temp file has been made */
	inafunc = NO,	/* in a function? */
	fprnt = YES,	/* first printed line? */
	addlino = NO,	/* add line number to line?? */
	hflag = YES,	/* print header? */
	ddbgflg = NO,	/* debugging? */
	idbgflg = NO,
	bdbgflg = NO,
	tdbgflg = NO,
	edbgflg = NO,
	xdbgflg = NO,
	LSIZE = LPROUT,	/* default size */
	lsize = 0,	/* scanline: keeps track of line length */
	sblk = 0,	/* saves first block number of each new function */
	fsp = 0,	/* format determined by prntflnm */
	defs = 0,	/* number of defines */
	undefs = 0,	/* number of undefines */
	incs = 0;	/* number of '-I' directories */

int	nword,	/* prntlino: new word? */
	nflnm,	/* new filename? */
	nfnc,	/* new function name? */
	nlino;	/* new line number? */

main(argc,argv)
	int argc;
	char *argv[];
{

	xref = myalloc(sizeof(OWNERBIN) + sizeof(XREF) - 1, 1);
	strcat(strcpy(xref, OWNERBIN), XREF);
	xcpp = myalloc(sizeof(XCPPBIN) + sizeof(XCPP) - 1, 1);
	strcat(strcpy(xcpp, XCPPBIN), XCPP);

	mktmpfl();	/* make temp files */
	while (--argc > 0) {
		++argv;
		sblk = 0;
		if (**argv == '-') {
			switch (*++*argv) {
				/* prints cross reference for all files combined */
				case 'c':
					cflg = YES;
					continue;
				case 'd':
					ddbgflg = YES;
					continue;
				case 'h':
					hflag = NO;
				case 'i':
					idbgflg = YES;
					continue;
				case 'b':
					bdbgflg = YES;
					continue;
				case 'T':
					tdbgflg = YES;
					continue;
				case 'e':
					edbgflg = YES;
					continue;
				case 'x':
					xdbgflg = YES;
					continue;
				case 'D':
					if (defs < MAXDEFS) {
						predefs[defs++] = --*argv;
					}
					else {
						fprintf(stderr, "Too many defines\n");
						dexit(1);
					};
					continue;
				case 'U':
					if (undefs < MAXDEFS) {
						preunds[undefs++] = --*argv;
					}
					else {
						fprintf(stderr, "Too many undefines\n");
						dexit(1);
					};
					continue;
				case 'I' :
					if (incs < MAXINC) {
						incdirs[incs++] = --*argv;
					}
					else {
						fprintf(stderr, "Too many '-I' options\n");
						dexit(1);
					};
					continue ;
				/* length option when printing on terminal */
				case 't':
				case 'w':
					LSIZE = getnumb(++*argv) - BASIZ;
					if (LSIZE <= 0)
					LSIZE = TTYOUT;
					continue;
				case 's':
					silent = YES;
					continue;
				/* output file */
				case 'o':
					if (freopen(*++argv, "w", stdout) == NULL) {
						perror(*argv);
						dexit(1);
					};
					argc--;
					continue;
				default:
					fprintf(stderr, "Invalid option '%c' - ignored\n", **argv);
			};
		}
		else if (strcmp(*argv + strlen(*argv) - 2, ".c") != 0)
			continue;		/* not a .c file, skip */
		else {

			cfile = *argv;
			if (silent == NO)
				printf("%s:\n\n", cfile);
		};

		doxref();	/* find variables in the input files */
		if (cflg == NO) {
			sortfile();	/* sorts temp file */
			prtsort();	/* print sorted temp file when -c option is not used */
			tunlink();	/* forget everything */
			mktmpfl();	/* and start over */
		};
	};
	if (cflg == YES) {
		sortfile();	/* sorts temp file */
		prtsort();	/* print sorted temp file when -c option is used */
	};
	tunlink();	/* unlink temp files */
	exit (0);
	/*NOTREACHED*/
}

	/*
	 * This routine calls the program "xpass" which parses
	 * the input files, breaking the file down into
	 * a list with the variables, their definitions and references,
	 * the beginning and ending function block numbers,
	 * and the names of the functions---The output is stored on a
	 * temporary file and then read.  The file is then scanned
	 * by tmpscan(), which handles the processing of the variables
	 */

doxref()
{

	register int i, n;
	FILE *fp, *tfp;
	char line[MAXRFL], s1[MAXRFW], s2[MAXRFW];

	arv[0] = xcpp;
	arv[1] = "-F";
	arv[2] = tmp2;
	arv[3] = cfile;
	arv[4] = tmp1;
	for (n = 5, i = 0; i < defs; i++) {
		arv[n++] = predefs[i];
	};
	for (i = 0; i < undefs; i++) {
		arv[n++] = preunds[i];
	};
	for (i = 0; i < incs; i++) {
		arv[n++] = incdirs[i];
	};
#ifndef S5EMUL
	arv[n++] = "-I/usr/ucbinclude";
#else
	arv[n++] = "-I/usr/5include";
#endif
	arv[n] = 0 ;
	if ( callsys(xcpp, arv) > 0) {
		fprintf(stderr, "cpp failed on %s!\n", cfile);
		dexit(1);
	};

	i = 0;
	arv[i++] = "xpass";
	if (ddbgflg)
		arv[i++] = "-d";
	if (idbgflg)
		arv[i++] = "-I";
	if (bdbgflg)
		arv[i++] = "-b";
	if (tdbgflg)
		arv[i++] = "-t";
	if (edbgflg)
		arv[i++] = "-e";
	if (xdbgflg)
		arv[i++] = "-x";
	arv[i++] = "-f";
	arv[i++] = cfile;
	arv[i++] = "-i";
	arv[i++] = tmp1;
	arv[i++] = "-o";
	arv[i++] = tmp2;
	arv[i++] = 0;
	if ( callsys(xref, arv) > 0) {
		fprintf(stderr, "xpass failed on %s!\n", cfile);
		dexit(1);
	};

	/* open temp file produced by "xpass" for reading */
	if ((fp = fopen(tmp2, "r")) == NULL) {
		perror(tmp2);
		dexit(1);
	}
	else {
		setbuf(fp, buf);
		/*
		 * open a new temp file for writing
		 * the output from tmpscan()
		 */
		if ((tfp = fopen(tmp3, "a")) == NULL) {
			perror(tmp3);
			dexit(1);
		}
		else {

			/*
			 * read a line from tempfile 2,
			 * break it down and scan the
			 * line in tmpscan()
			 */

			while (fgets(line, MAX, fp) != NULL) {
				if (line[0] == '"') {
					/* removes quotes */
					for (i=0; line[i + 1] != '"'; i++) {
						xflnm[i] = line[i + 1];
					};
					xflnm[i] = '\0';
					continue;
				};
				sscanf(line, "%s%s", s1, s2);
				tmpscan(s1, s2, tfp);
			};
			if (tfp && (feof(tfp) || ferror(tfp)))
			{
				perror("cxref.tmpscan");
				dexit(1);
			}
			fclose(tfp);
		};
		if (fp && ferror(fp))
		{
			perror("cxref.tmpscan");
			dexit(1);
		}
		fclose(fp);
	};
}

	/*
	 * general purpose routine which does a fork
	 * and executes what was passed to it--
	 */

callsys(f, v)
	char f[], *v[];
{
	register int pid, w;
	int status;

	if ((pid = fork()) == 0) {
		/* only the child gets here */
		execvp(f, v);
		perror(f);
		dexit(1);
	}
	else
		if (pid == -1) {
			/* fork failed - tell user */
			perror("cxref");
			dexit(1);
		};
	/*
	 * loop while calling "wait" to wait for the child.
	 * "wait" returns the pid of the child when it returns or
	 * -1 if the child can not be found.
	 */
	while (((w = wait(&status)) != pid) && (w != -1));
	if (w == -1) {
		/* child was lost - inform user */
		perror(f);
		dexit(1);
	}
	else {
		/* check system return status */
		if (((w = status & 0x7f) != 0) && (w != SIGALRM)) {
			/* don't complain if the user interrupted us */
			if (w != SIGINT) {
				fprintf(stderr, "Fatal error in %s", f);
				perror(" ");
			};
			/* remove temporary files */
			dexit(1);
		};
	};
	/* return child status only */
	return((status >> 8) & 0xff);
}

	/*
	 * general purpose routine which returns
	 * the numerical value found in a string
	 */

getnumb(ap)
	register char *ap;
{

	register int n, c;

	n = 0;
	while ((c = *ap++) >= '0' && c <= '9')
		n = n * 10 + c - '0';
	return(n);
}

tmpscan(s, ns, tfp)
	register char s[];
	char ns[];
	FILE *tfp;
{

	/* this routine parses the output of "xpass"*/

	/*
	 * D--variable defined; R--variable referenced;
	 * F--function name; B--block(function ) begins;
	 * E--block(function) ends
	 */

	register int lino = 0;
	char star;

	/*
	 * look for definitions and references of external variables;
	 * look for function names and beginning block numbers
	 */

	if (inafunc == NO) {
		switch (*s++) {
			case 'D':
				star = '*';
				goto ahead1;
			case 'R':
				star = ' ';
ahead1:				lino = getnumb(ns);
				fprintf(tfp, "%s\t%s\t%s\t%5d\t%c\n", s, xflnm, clfunc, lino, star);
				break;
			case 'F':
				strcpy(funcname, s);
				star = '$';
				fprintf(tfp, "%s\t%c\n", s, star);
				break;
			case 'B':
				inafunc = YES;
				sblk = getnumb(s);
				break;
			default:
				fprintf(stderr, "SWITCH ERROR IN TMPSCAN: inafunc = no\n");
				dexit(1);
		};
	}
	else {
		/*
		 * in a function:  handles local variables
		 * and looks for the end of the function
		 */

		switch (*s++) {
			case 'R':
				star = ' ';
				goto ahead2;
				/* No Break Needed */
			case 'D':
				star = '*';
ahead2:				lino = getnumb(ns);
				fprintf(tfp, "%s\t%s\t%s\t%5d\t%c\n", s, xflnm, funcname, lino, star);
				break;
			case 'B':
				break;
			case 'E':
				lino = getnumb(s);
				/*
				 * lino is used to hold the ending block
				 * number at this point
				 *
				 * if the ending block number is the
				 * same as the beginning block number
				 * of the function, indicate that the
				 * next variable found will be external.
				 */

				if (sblk == lino) {
					inafunc = NO;
				}
				break;
			case 'F':
				star = '$';
				fprintf(tfp, "%s\t%c\n", s, star);
				break;
			default:
				fprintf(stderr, "SWITCH ERROR IN TMPSCAN: inafunc = yes\n");
				dexit(1);
		};
	};
}

mktmpfl()
{

	/* make temporary files */

	tmp1 = tempnam((char *)NULL, "xr1");
	tmp2 = tempnam((char *)NULL, "xr2");	/* holds output of "xpass" */
	tmp3 = tempnam((char *)NULL, "xr3");	/* holds output of tmpscan() routine */
	tmp4 = tempnam((char *)NULL, "xr4");	/* holds output of tempfile 3 */
	tmpmade = YES;	/* indicates temporary files have been made */
	setsig();
}

tunlink()
{

	/* unlink temporary files */

	if (tmpmade == YES) {	/* if tempfiles exist */
		unlink(tmp1);
		unlink(tmp2);
		unlink(tmp3);
		unlink(tmp4);
	};
}

dexit(n)
	int n;
{
	/* remove temporary files and exit with error status */

	tunlink();
	exit(n);
}

setsig()
{

	/* set up check on signals */

	int sigout();

	if (isatty(1)) {
		if (signal(SIGHUP, SIG_IGN) == SIG_DFL)
			signal(SIGHUP, sigout);
		if (signal(SIGINT, SIG_IGN) == SIG_DFL)
			signal(SIGINT, sigout);
	}
	else {
		signal(SIGHUP, SIG_IGN);
		signal(SIGINT, SIG_IGN);
	};

	signal(SIGQUIT, sigout);
	signal(SIGTERM, sigout);
}

sigout()
{

	/* signal caught; unlink tmp files */

	tunlink();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	dexit(1);
}

sortfile()
{
	/* sorts temp file 3 --- stores on 4 */

	register int status;

	arv[0] = "sort";
	arv[1] = "-o";
	arv[2] = tmp4;
	arv[3] = tmp3;
	arv[4] = 0;
	/* execute sort */
	if ((status = callsys(SRT, arv)) > 0) {
		fprintf(stderr, "Sort failed with status %d\n", status);
		dexit(1);
	};
}

prtsort()
{

	/* prints sorted files and formats output for cxref listing */

	FILE *fp;
	char line[MAX];

	/* open tempfile of sorted data */
	if ((fp = fopen(tmp4, "r")) == NULL) {
		fprintf(stderr, "CAN'T OPEN %s\n", tmp4);
		dexit(1);
	}
	else {
		if (hflag) {
			fprintf(stdout, "SYMBOL\t\tFILE\t\t\tFUNCTION   LINE\n");
		};
		while (fgets(line, MAX, fp) != NULL) {
			scanline(line);	/* routine to format output */
		};
		fprnt = YES;	/* reinitialize for next file */
		if (fp && ferror(fp))
		{
			perror("cxref.prtsort");
			dexit(1);
		}
		fclose(fp);
		putc('\n', stdout);
	};
}

scanline(line)
	char *line;
{

	/* formats what is to be printed on the output */

	register char *sptr1;
	register int och, nch;
	char s1[MAXRFL], s2[MAXRFL], s3[MAXRFL], s4[MAXRFL], s5[MAXRFL];
	
	/*
	 * break down line into variable name, filename,
	 * function name, and line number
	 */
	sscanf(line, "%s%s%s%s%s", s1, s2, s3, s4, s5);
	if (strcmp(s2, "$") == 0) {
		/* function name */
		if (strcmp(sv1, s1) != 0) {
			strcpy(sv1, s1);
			printf("\n%s()", s1);	/* append '()' to name */
			*sv2 = *sv3 = *sv4 = '\0';
			fprnt = NO;
		};
		return;
	};
	if (strcmp(s5, "*") == 0) {
		/* variable defined at this line number */
		*s5 = '\0';
		sptr1 = s4;
		och = '*';
		/* prepend a star '*' */
		for ( nch = *sptr1; *sptr1 = och; nch = *++sptr1)
			och = nch;
	}
	if (fprnt == YES) {
		/* if first line--copy the line to a save area */
		prntwrd( strcpy(sv1, s1) );
		prntflnm( strcpy(sv2, s2) );
		prntfnc( strcpy(sv3, s3) );
		prntlino( strcpy(sv4, s4) );
		fprnt = NO;
		return;
	}
	else {
		/*
		 * this part checks to see what variables have changed
		 */
		if (strcmp(sv1, s1) != 0) {
			nword = nflnm = nfnc = nlino = YES;
		}
		else {
			nword = NO;
			if (strcmp(sv2, s2) != 0) {
				nflnm = nfnc = nlino = YES;
			}
			else {
				nflnm = NO;
				if (strcmp(sv3, s3) != 0) {
					nfnc = nlino = YES;
				}
				else {
					nfnc = NO;
					nlino = (strcmp(sv4, s4) != 0) ? YES : NO;
					if (nlino == YES) {
						/*
						 * everything is the same
						 * except line number
						 * add new line number
						 */
						addlino = YES;
						prntlino( strcpy(sv4, s4) );
					};
					/*
					 * Want to return if we get to
					 * this point. Case 1: nlino
					 * is NO, then entire line is
					 * same as previous one.
					 * Case 2: only line number is
					 * different, add new line number
					 */
					return;
				};
			};
		};
	};

	/*
	 * either the word, filename or function name
	 * are different; this part of the routine handles  
	 * what has changed...
	 */

	addlino = NO;
	lsize = 0;
	if (nword == YES) {
		/* word different--print line */
		prntwrd( strcpy(sv1, s1) );
		prntflnm( strcpy(sv2, s2) );
		prntfnc( strcpy(sv3, s3) );
		prntlino( strcpy(sv4, s4) );
		return;
	}
	else {
		fputs("\n\t\t", stdout);
		if (nflnm == YES) {
			/*
			 * different filename---new name,
			 * function name and line number are
			 * printed and saved
			 */
			prntflnm( strcpy(sv2, s2) );
			prntfnc( strcpy(sv3, s3) );
			prntlino( strcpy(sv4, s4) );
			return;
		}
		else {
			/* prints filename as formatted by prntflnm()*/
			switch (fsp) {
			case 1:
				printf("%s\t\t\t", s2);
				break;
			case 2:
				printf("%s\t\t", s2);
				break;
			case 3:
				printf("%s\t", s2);
				break;
			case 4:
				printf("%s  ", s2);
			};
			if (nfnc == YES) {
				/*
				 * different function name---new name
				 * is printed with line number;
				 * name and line are saved
				 */
				prntfnc( strcpy(sv3, s3) );
				prntlino( strcpy(sv4, s4) );
			};
		};
	};
}

prntwrd(w)
	char *w;
{

	/* formats word(variable)*/
	register int wsize;	/*16 char max. word length */

	if ((wsize = strlen(w)) < 8) {
		printf("\n%s\t\t", w);
	}
	else
		if ((wsize >= 8) && (wsize < 16)) {
			printf("\n%s\t", w);
		}
		else {
			printf("\n%s  ", w);
		};
}

prntflnm(fn)
	char *fn;
{

	/* formats filename */
	register int fsize;	/*24 char max. fliename length */

	if ((fsize = strlen(fn)) < 8) {
		printf("%s\t\t\t", fn);
		fsp = 1;
	}
	else {
		if ((fsize >= 8) && (fsize < 16)) {
			printf("%s\t\t", fn);
			fsp = 2;
		}
		else
			if ((fsize >= 16) && (fsize < 24)) {
				printf("%s\t", fn);
				fsp = 3;
			}
			else {
				printf("%s  ", fn);
				fsp = 4;
			};
	};
}

prntfnc(fnc)
	char *fnc;
{

	/* formats function name */
	register int fsize;

	if ((fsize = strlen(fnc)) < 8) {
		printf("%s\t  ", fnc);
	}
	else {
		switch (fsize) {

		case 8:
			printf("%s   ", fnc);
			break;

		case 9:
			printf("%s  ", fnc);
			break;

		default:
			printf("%s ", fnc);
			break;
		}
	};
}

char *
myalloc(num, size)
	unsigned num, size;
{
	register char *ptr;

	if ((ptr = calloc(num, size)) == NULL) {
		perror("cxref");
		dexit(1);
	};
	return(ptr);
}

prntlino(ns)
	register char *ns;
{

	/* formats line numbers */

	register int lino, i;
	char star;

	i = lino = 0;

	if (*ns == '*') {
		star = '*';
		ns++;	/* get past star */
	}
	else {
		star = ' ';
	};
	lino = getnumb(ns);
	if (lino < 10)	/* keeps track of line width */
		lsize += (i = 3);
	else if ((lino >=10) && (lino < 100))
			lsize += (i = 4);
	else if ((lino >= 100) && (lino < 1000))
				lsize += (i = 5);
	else if ((lino >= 1000) && (lino < 10000))
					lsize += (i = 6);
	else /* lino > 10000 */
						lsize += (i = 7);
	if (addlino == YES) {
		if (lsize <= LSIZE) {
			/* line length not exceeded--print line number */
			fprintf(stdout, " %c%d", star, lino);
		}
		else {
			/* line to long---format lines overflow */
			fprintf(stdout, "\n\t\t\t\t\t\t   %c%d", star, lino);
			lsize = i;
		};
		addlino = NO;
	}
	else {
		fprintf(stdout, " %c%d", star, lino);
	};
}
