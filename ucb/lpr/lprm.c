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
static char sccsid[] = "@(#)lprm.c 1.1 92/07/30 SMI"; /* from UCB 5.2 11/17/85 */
#endif not lint

/*
 * lprm - remove the current user's spool entry
 *
 * lprm [-] [[job #] [user] ...]
 *
 * Using information in the lock file, lprm will kill the
 * currently active daemon (if necessary), remove the associated files,
 * and startup a new daemon.  Priviledged users may remove anyone's spool
 * entries, otherwise one can only remove their own.
 */

#include "lp.h"

/*
 * Stuff for handling job specifications
 */
char	*user[MAXUSERS];	/* users to process */
int	users;			/* # of users in user array */
int	requ[MAXREQUESTS];	/* job number of spool entries */
int	requests;		/* # of spool requests */
char	*person;		/* name of person doing lprm */

static char	luser[16];	/* buffer for person */

struct passwd *getpwuid();
char	*getlogin();

main(argc, argv)
	char *argv[];
{
	register char *arg;
	struct passwd *p;

	name = argv[0];
	(void)gethostname(host, sizeof host);
	openlog("lpd", 0, LOG_LPR);
	if (getuid() == 0 || (arg = getlogin()) == NULL) {
		if ((p = getpwuid(getuid())) == NULL)
			fatal("Who are you?");
		arg = p->pw_name;
	}
	if (strlen(arg) >= sizeof luser)
		fatal("Your name is too long");
	strcpy(luser, arg);
	person = luser;
	while (--argc) {
		if ((arg = *++argv)[0] == '-')
			switch (arg[1]) {
			case 'P':
				if (arg[2])
					printer = &arg[2];
				else if (argc > 1) {
					argc--;
					printer = *++argv;
				}
				break;
			case '\0':
				if (!users) {
					users = -1;
					break;
				}
			default:
				usage();
			}
		else {
			if (users < 0)
				usage();
			if (isdigit(arg[0])) {
				if (requests >= MAXREQUESTS)
					fatal("Too many requests");
				requ[requests++] = atoi(arg);
			} else {
				if (users >= MAXUSERS)
					fatal("Too many users");
				user[users++] = arg;
			}
		}
	}
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;

	rmjob();

	exit(0);
	/* NOTREACHED */
}

static
usage()
{
	printf("usage: lprm [-] [-Pprinter] [[job #] [user] ...]\n");
	exit(2);
}
