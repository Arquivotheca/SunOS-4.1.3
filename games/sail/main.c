/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/29/85 */
#endif not lint

#include "externs.h"

/*ARGSUSED*/
main(argc, argv)
	int argc;
	register char **argv;
{
	register char *p;
	int i;
	static char sobuf[BUFSIZ];

	setbuf(stdout, sobuf);
	(void) srand(getpid());
	issetuid = getuid() != geteuid();
	if (p = rindex(*argv, '/'))
		p++;
	else
		p = *argv;
	if (strcmp(p, "driver") == 0 || strcmp(p, "saildriver") == 0)
		mode = MODE_DRIVER;
	else if (strcmp(p, "sail.log") == 0)
		mode = MODE_LOGGER;
	else
		mode = MODE_PLAYER;
	while ((p = *++argv) && *p == '-')
		switch (p[1]) {
		case 'd':
			mode = MODE_DRIVER;
			break;
		case 's':
			mode = MODE_LOGGER;
			break;
		case 'D':
			debug++;
			break;
		case 'x':
			randomize;
			break;
		case 'l':
			longfmt++;
			break;
		case 'b':
			nobells++;
			break;
		default:
			fprintf(stderr, "SAIL: Unknown flag %s.\n", p);
			exit(1);
		}
	if (*argv)
		game = atoi(*argv);
	else
		game = -1;
	if (i = setjmp(restart))
		mode = i;
	switch (mode) {
	case MODE_PLAYER:
		return pl_main();
	case MODE_DRIVER:
		return dr_main();
	case MODE_LOGGER:
		return lo_main();
	default:
		fprintf(stderr, "SAIL: Unknown mode %d.\n", mode);
		abort();
	}
	/*NOTREACHED*/
}
