/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

/*	ctrace - C program debugging tool
 *
 *	main function - interprets arguments
 *
 */
#include "global.h"

/* global options */
enum	bool suppress = no;	/* suppress redundant trace output (-s) */
enum	bool pound_line = no;	/* input preprocessed so suppress #line	*/
int	tracemax = TRACE_DFLT;	/* maximum traced variables per statement (-t number) */

/* local options */
static	enum	bool preprocessor = no;	/* run the preprocessor (-P) */
static	enum	bool octal = no;	/* print variable values in octal (-o) */
static	enum	bool hex = no;		/* print variable values in hex (-x) */
static	enum	bool unsign = no;	/* print variable values as unsigned (-u) */
static	enum	bool floating = no;	/* print variable values as float (-e) */
static	enum	bool only = no;		/* trace only these functions (-f) */
static	enum	bool basic = no;	/* use basic functions only (-b) */
static	int	loopmax = LOOPMAX;	/* maximum loop size for loop recognition (-l number) */
static	char	*print_fcn = "printf(";	/* run-time print function (-p string) */
static	char	*codefile = NULL;	/* run-time package file name (-r file) */

/* global data */
char	*filename = "";		/* input file name */
enum	bool trace;		/* indicates if this function should be traced */

/* local data */
static	int	error_code = 0;		/* exiting error code */
static	int	fcn_count = 0;		/* number of functions (-f and -v) */
static	char	**fcn_names;		/* function names (-f and -v) */
static	char	*pp_command;		/* preprocessor command (-P) */
static	enum	bool fcn_traced = no;	/* indicates if at least one function was traced */
/* main function */

main(argc, argv)
int	argc;
char	*argv[];
{
	int	i;
	char	option, *s;
	char	*strsave();
	FILE	*fopen(), *popen();

	/* set the options */
	if ((pp_command = strsave(PP_COMMAND)) == NULL) {
		fprintf(stderr, "ctrace: out of storage");
		exit(1);
	}
	while (--argc > 0 && (*++argv)[0] == '-') {
		for (s = argv[0] + 1; *s != '\0'; s++)
			switch (*s) {
			case 'P':	/* run the preprocessor */
				preprocessor = yes;
				break;
			case 'D':	/* options for preprocessor */
			case 'I':
			case 'U':
				preprocessor = yes;
				append(pp_command, argv[0]);
				goto next_arg;
			case 'o':	/* variable output in octal */
				octal = yes;
				break;
			case 'x':	/* variable output in hex */
				hex = yes;
				break;
			case 'u':	/* variable output as unsigned */
				unsign = yes;
				break;
			case 'e':	/* print variable values as float */
				floating = yes;
				break;
			case 'b':	/* use basic functions only */
				basic = yes;
				break;
			case 's':	/* suppress redundant trace output */
				suppress = yes;
				break;
					/* options with string arguments */
			case 'p':
			case 'r':
			case 'T':	/* obsolete option */
				option = *s;
				if (*++s == '\0' && --argc > 0) {
					s = *++argv;
				}
				switch (option) {
				case 'p':	/* run-time print function */
					print_fcn = s;
					break;
				case 'r':	/* run-time code file name */
					codefile = s;
					break;
				case 'T':	/* define symbol as a type */
					add_type(s);
					break;
				}
				goto next_arg;
			case 'f':	/* trace only these functions */
				only = yes;
			case 'v':	/* trace all but these functions */
				fcn_names = argv + 1; /* not valid if no more args */
				/* look for the file name or next option */
				while (--argc > 0) {
					s = *++argv;	/* get next arg */
					if (s[0] == '-' || s[strlen(s) - 2] == '.') {
						--argv;	/* back up one arg */
						break;
					}
					/* shorten long function names */
					if (strlen(s) > IDMAX)
						s[IDMAX] = '\0';
					++fcn_count;
				}
				++argc;	/* back up one arg */
				goto next_arg;
			case 'l':	/* options with a numeric argument */
			case 't':
				option = *s;
				if (*++s == '\0' && --argc > 0)
					s = *++argv;

				/* convert the argument to an integer and check for a conversion error */
				if (*s < '0' || *s > '9' || ((i = atoi(s)) == 0 && option == 't')) {
					fprintf(stderr, "ctrace: -%c option: missing or invalid numeric value\n", option);
					goto usage;
				}
				switch (option) {
				case 'l':	/* print all loop trace output */
					loopmax = i;
					break;
				case 't':	/* maximum traced variables per statement */
					tracemax = i;
					if (tracemax > TRACEMAX) /* force into range */
						tracemax = TRACEMAX;
					break;
				}
				goto next_arg;
			default:
				fprintf(stderr, "ctrace: illegal option: -%c\n", *s);
usage:
				fprintf(stderr, "Usage: ctrace [-beosuxP] [C preprocessor options] [-f|v functions]\n");
				fprintf(stderr, "              [-p string] [-r file] [-l number] [-t number] [file]\n");
				exit(1);
			}
next_arg: ;
	}
	/* open the input file */
	yyin = stdin; /* default to the standard input */
	if (argc > 0) {
		filename = *argv; /* save the file name for messages */
		if ((yyin = fopen(filename, "r")) == NULL) {
			fprintf(stderr, "ctrace: cannot open file %s\n", filename);
			exit(1);
		}
	}
	/* run the preprocessor if requested */
	if (preprocessor) {
		if (strcmp(filename, "") != 0) {
			append(pp_command, filename);
		}
		if ((yyin = popen(pp_command, "r")) == NULL) {
			fprintf(stderr, "ctrace: cannot run preprocessor");
			exit(1);
		}
		pound_line = yes;	/* suppress #line insertion */
	}
	/* #define CTRACE so you can put this in your code:
	 *	#ifdef CTRACE
	 *		ctroff();
	 *	#endif
	 */
	printf("#define CTRACE 1\n");
	
	/* prepend signal.h (needed by the runtime package) so programs that
	   call signal but don't include signal.h won't get a "redeclaration
	   of signal" compiler error */
	if (!basic) {
		printf("#include <signal.h>\n");
	}
	/* prepend a #line statement with the original file name for 
	   compiler errors in programs with no preprocessor statements */
	printf("#line 1 \"%s\"\n", filename);
	
	/* put the keywords into the symbol table */
	init_symtab();
	
	/* trace the executable statements */
	yyparse();

	/* output the run-time package of trace functions */
	if (fcn_traced)
		runtime();

	/* final processing */
	quit();

	/* NOTREACHED */
}
/* append an argument to the command */

append(s1, s2)
char	*s1, *s2;
{
	char	*realloc(), *strcat();

	/* increase the command string size */
	if ((s1 = realloc(s1, (unsigned) (strlen(s1) + strlen(s2) + 4))) == NULL) {
		fprintf(stderr, "ctrace: out of storage");
		exit(1);
	}
	/* append the argument with single quotes around it to preserve any special chars */
	strcat(s1, " '");
	strcat(s1, s2);
	strcat(s1, "'");
}
/* determine if this function should be traced */

tr_fcn(identifier)
char	*identifier;
{
	int	i;
	
	/* set default trace mode */
	if (only)
		trace = no;
	else
		trace = yes;
	
	/* change the mode if this function is in the list */
	for (i = 0; i < fcn_count; ++i)
		if (strcmp(identifier, fcn_names[i])  == 0){
			if (only)
				trace = yes;
			else
				trace = no;
			break;
		}
	/* indicate if any function was traced */
	if (trace)
		fcn_traced = yes;
}
/* output the run-time package of trace functions */

runtime()
{
	register int	c;
	register FILE	*code;
	char	pathname[100];
	char	*s;
	char	*getenv(), *strcat(), *strcpy();
	
	/* force the output to a new line for the preprocessor statements */
	putchar('\n');
	
	/* if stdio.h has already been included and preprocessed then
	   just define stdout.  _iob would be defined twice if stdio.h is
	   included again */
	if (stdio_preprocessed)
		printf("#undef\tstdout\n#define stdout\t\t(&_iob[1])\n");
	/* if the output is not to be buffered then include stdio.h to get the stdout symbol */
	else if (!basic)
		printf("#include <stdio.h>\n");

	/* see if setjmp.h has already been included and preprocessed.
	   jmp_buf would be defined twice if it is included again */
	if (!setjmp_preprocessed && !basic)
		printf("#include <setjmp.h>\n");
		
	/* make preprocessor definitions to tailor the code */
	printf("#define VM_CT_ %d\n", tracemax);
	printf("#define PF_CT_ %s\n", print_fcn);
	if (octal)
		printf("#define O_CT_\n");
	if (hex)
		printf("#define X_CT_\n");
	if (unsign)
		printf("#define U_CT_\n");
	if (floating)
		printf("#define E_CT_\n");
	if (basic)
		printf("#define B_CT_\n");
	if (loopmax != 0)
		printf("#define LM_CT_ %d\n", loopmax);
		
	/* construct the file name of the runtime trace package */
	strcpy(pathname, RUNTIME);
	if (codefile != NULL) {
		strcpy(pathname, codefile);
	}
	/* open the file */
	if ((code = fopen(pathname, "r")) == NULL) {
		fprintf(stderr, "ctrace: cannot open runtime code file %s\n", pathname);
		exit(1);
	}
	/* output the runtime trace package */
	printf("#line 1 \"%s\"\n", pathname);
	while ((c = getc(code)) != EOF) {
		putchar(c);
	}
}
/* error and warning message functions */
fatal(text)
char *text;
{
	error(text);
	error_code = 3;
	quit();
}
error(text)
char *text;
{
	msg_header();
	fprintf(stderr, "%s\n", text);
	error_code = 2;
}
warning(text)
char *text;
{
	msg_header();
	fprintf(stderr, "warning: %s\n", text);
}
static
msg_header()
{
	fprintf(stderr, "ctrace: ");
	if (strcmp(filename, "") != 0)
		fprintf(stderr, "\"%s\", ", filename);
	fprintf(stderr, "line %d: ", yylineno);
}
quit()
{
	if (error_code > 1) {
		fprintf(stderr, "ctrace: see man page Diagnostics section for what to do next\n");
	}
	exit(error_code);
}
