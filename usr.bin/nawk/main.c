/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.5 */
#endif

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <signal.h>
#include "awk.h"
#include "y.tab.h"

int	dbg	= 0;
int	svargc;
uchar	**svargv;
uchar	*cmdname;	/* gets argv[0] for error messages */
extern	FILE *yyin;	/* lex input file */
uchar	*lexprog;	/* points to program argument if it exists */
extern	int errorflag;	/* non-zero if any syntax errors; set by yyerror */
int	compile_time = 1;	/* 0 when machine starts.  for error printing */

main(argc, argv)
	int argc;
	uchar *argv[];
{
	uchar *progfile = NULL, *progarg = NULL, *fs = NULL, *freezename = NULL;
	extern int fpecatch();

	setlocale(LC_ALL, "");
	cmdname = argv[0];
	if (argc == 1)
		error(FATAL, "Usage: %s [-f source] [-Fc] ['cmds'] [variable=value ...] [file ...]", cmdname);
	yyin = NULL;
	while (argc > 1 && argv[1][0] == '-' && argv[1][1] != '\0') {
		switch (argv[1][1]) {
		case 'f':	/* next argument is program filename */
			argc--;
			argv++;
			if (argc <= 1)
				error(FATAL, "no argument for -f");
			yyin = (strcmp(argv[1], "-") == 0) ? stdin : fopen(argv[1], "r");
			if (yyin == NULL)
				error(FATAL, "can't open file %s", argv[1]);
			progfile = argv[1];
			break;
		case 'F':	/* set field separator */
			if (argv[1][2] != 0) {	/* arg is -Fsomething */
				if (argv[1][2] == 't' && argv[1][3] == 0)	/* special case for tab */
					fs = (uchar *) "\t";
				else
					fs = &argv[1][2];
			} else {	/* it's -F (space) something */
				argc--;
				argv++;
				if (argc <= 1)
					error(FATAL, "no argument for -F");
				if (argv[1][0] == 't' && argv[1][1] == 0)
					fs = (uchar *) "\t";
				else
					fs = &argv[1][0];
			}
			break;
		case 'd':
			dbg = 1;
			break;
		case 'R': case 'S':
			error(FATAL, "-R and -S options are no longer available");
			break;
		default:
                        error(FATAL, "unknown option: %s.\n  Usage: %s [-f source] [-Fc] ['cmds'] [variable=value ...] [file ...]", argv[1], cmdname);  
                        break;
		}
		argc--;
		argv++;
	}
	if (yyin == NULL) {	/* no -f; first argument is program */
		if (argc <= 1) {
			dbg = 0;  /* unset debug; don't need core dump */
			error(FATAL, "no cmds.\n  Usage: %s [-f source] [-Fc] ['cmds'] [variable=value ...] [file ...]", cmdname);
		}
		dprintf("program = |%s|\n", argv[1]);
		progarg = lexprog = argv[1];
		argc--;
		argv++;
	}
	while (argc > 1) {	/* do leading "name=val" */
		if (!isclvar(argv[1]))
			break;
		setclvar(argv[1]);
		argc--;
		argv++;

	}
	argv[0] = cmdname;	/* put prog name at front of arglist */
	svargc = argc;
	svargv = argv;
	dprintf("svargc=%d, svargv[0]=%s\n", svargc, svargv[0]);
	syminit(svargc, svargv);
	if (fs)
		*FS = tostring(fs);
	*FILENAME = svargv[1];	/* initial file name */
	if (argc == 1) {	/* no filenames; use stdin */
		initgetrec();
	}
	signal(SIGFPE, fpecatch);
	yyparse();
	dprintf("errorflag=%d\n", errorflag, NULL, NULL);
	if (errorflag == 0) {
		compile_time = 0;
		run(winner);
	} else
		bracecheck();
	exit(errorflag);
	/* NOTREACHED */
}
