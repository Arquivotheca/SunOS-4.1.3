/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)cancel.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * cancel - cancel the current entry if it belongs to the invoker
 *
 * cancel [ids] [printers]
 *
 * Privileged users may remove anyone's spool entries,
 * otherwise one can only remove his own.
 */

#include "lp.h"

/*
 * Stuff for handling job specifications
 */
char	*user[MAXREQUESTS];		/* name of users to process */
int	users;				/* number of users specified */
int	requ[MAXREQUESTS];		/* job number of spool entries */
int	requests;			/* number of entries in requ */
char	*printers[MAXREQUESTS];		/* printers to process */
int	numprint;			/* number of printers in array */
char	*person;			/* name of person doing cancel */


struct passwd *getpwuid();
char *getlogin();

main(argc, argv)
int argc;
char *argv[];
{
	register char *arg;
	struct passwd *p;
	int i;

	name = argv[0];
	gethostname(host, sizeof (host));
	openlog("lpd", 0, LOG_LPR);

	if (getuid() == 0 || (arg = getlogin()) == NULL) {
		if ((p = getpwuid(getuid())) == NULL)
			fatal("Who are you?");
		arg = p->pw_name;
	}
	person = arg;
	for (i = 1; i < argc; i++) {
		arg = argv[i];
		if (isdigit(arg[0])) {
			if (requests >= MAXREQUESTS)
				fatal("Too many requests");
			requ[requests++] = atoi(arg);
		} else {
			if (numprint >= MAXREQUESTS)
				fatal("Too many printers");
			printers[numprint++] = arg;
		}
	}

	if (requests > 0) {
		/* remove all jobs for which an id was provided */
		printer = getenv("LPDEST");
		if (printer == NULL)
			printer = getenv("PRINTER");
		if (printer == NULL)
			printer = DEFLP;
		rmjob();
		requests = 0;
	}
	users = 1;
	*user = person;
	while (numprint > 0) {
		/* remove jobs belonging to caller and running on printer */
		printer = printers[--numprint];
		rmjob();
	}
	return (0);
}
