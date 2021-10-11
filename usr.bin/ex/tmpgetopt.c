/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	tmpgetopt.c	*/

#ifndef lint
static	char *sccsid = "@(#)tmpgetopt.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1 */
#endif

#define	NULL		0
#define	DONE		-1
#define	OPTIONS		'-'
#define	COMMANDS	'+'
#define EQUAL(a,b)	!strcmp(a,b)
#define	ERR(s1,s2,s3,c)	if (argerr) {\
			extern int strlen(), write();\
			char errbuf[2];\
			errbuf[0] = c; errbuf[1] = '\n';\
			(void) write (2, argv[0], (unsigned)strlen(argv[0]));\
			(void) write (2, s1, (unsigned) strlen(s1));\
			(void) write (2, s2, (unsigned) strlen(s2));\
			(void) write (2, s3, (unsigned) strlen(s3));\
			(void) write (2, errbuf, 2); }

extern	int	strcmp();
extern	int	strlen();
extern	char	*strchr();

int	argerr = 1;
int	argind = 1;
int	argopt;
char   *argval;
char	argtyp;

int
tmpgetopt(argc, argv, opts, cmds)
int	argc;
char  **argv, *opts, *cmds;
{
	static int 	argptr = 1;
	static char    *bufptr;
	char	       *bufpos;
	char           *errptr;

	if (argptr == 1) {

		/*
	 	 * When starting any argument, if there are no more
	 	 * arguments to process, return (DONE).
	 	 */

		if (argind >= argc)
			return(DONE);

		/* 
	 	 * Set up pointers based on argument type.
	 	 * If arguments not prefixed with the valid
	 	 * markers, then return (DONE).
	 	 */

		switch (argv[argind][0]) {

		case OPTIONS:
			errptr = "option";
			bufptr = opts;
			argtyp = OPTIONS;
			break;

		case COMMANDS:
			errptr = "commands";
			bufptr = cmds;
			argtyp = COMMANDS;
			break;

		default:
			return(DONE);
		}

		/* 
	 	 * When starting to parse an argument,
	 	 * if it is "++", or "--" then it means 
	 	 * the end of the options, or commands.
	 	 * Skip over this argument
	 	 * and return (DONE).
	 	 */

		if ((strlen(argv[argind]) == 2)
		    && (argv[argind][0] == argv[argind][1])) {
			argind++;
			return(DONE);
		}

		/* 
	 	 * If the length of the argument is 1,
	 	 * then make sure this is permitted by
	 	 * checking for appropriate marker.
	 	 */

		if (strlen(argv[argind]) == 1) {
			if (!strchr(bufptr, argtyp)) {
				ERR(": illegal ", errptr, " -- ", argtyp);
				return('?');
			}
			argind++;
			return(NULL);
		}
	}

	/* 
	 * Get next character from argument.  If it is ':', ';', or
	 * not in the argument list, it is an error.
	 */

	argopt = argv[argind][argptr];
	if (argopt == ':' || argopt == ';' || (bufpos=strchr(bufptr, argopt)) == NULL) {
		/*
		 * If this process accepts parameters for a command
		 * or option with marker only (- parm / + parm)
		 * stop taking options and commands.
		 */
		if (((bufpos=strchr(bufptr, argtyp)) != NULL) 
		  && (*(bufpos+1) == ':')) {
			argval = &argv[argind++][argptr];
			argptr = 1;
			return(NULL);
		} else {
			ERR(": illegal ",errptr," -- ", argopt);
			if (argv[argind][++argptr] == '\0') {
				argind++;
				argptr = 1;
			}
			return ('?');
		}
	}

	/* 
	 * Check the correct argument list, based on argument type,
	 * to see if this command/option may have an argument
	 * associated with it.
	 */

	if ((*++bufpos == ':') 
	||  (*bufpos == ';')) {
		if (argv[argind][argptr+1] != '\0') {

		/*
		 * If command or option has an argument
	 	 * associated with it, set argval to the argument.
		 */

		if ((*bufpos == ';')
		&& ((argv[argind][argptr+1] == '-')
		||  (argv[argind][argptr+1] == '+')))
				argval = NULL;
			else
				argval = &argv[argind++][argptr+1];
	 } else if (++argind >= argc) {

			/*
		 	 * If out of arguments, and this command or option
	 	 	 * needs one, then there is an error.
		 	 */

			if (*bufpos == ';') {
				argval = NULL;
				return(argopt);
			} else {
				ERR(": ",errptr," requires an argument -- ", argopt);
				argptr = 1;
				return('?');
			} 
		} else if ((*bufpos == ';')
			|| (argv[argind][0] == '-')
			|| (argv[argind][0] == '+'))

				argval = NULL;
			else
				argval = argv[argind++];
		argptr = 1;
	} else {

		/*
		 * If the option/command does not have
		 * an argument associated with it, then set
	 	 * argval to NULL.
		 */

		if (argv[argind][++argptr] == '\0') {
			argptr = 1;
			argind++;
		}
		argval = NULL;
	}
	return(argopt);
}
