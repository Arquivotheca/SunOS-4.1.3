/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char sccsid[] = "@(#)nice.c 1.1 92/07/30 SMI"; /* from UCB 5.2 86/01/12 */
#endif not lint

#include <stdio.h>
#include <ctype.h>

#include <sys/time.h>
#include <sys/resource.h>

main(argc, argv)
	int argc;
	char *argv[];
{
	int nicarg = 10;

	if (argc > 1 && argv[1][0] == '-') {
		register char	*p = argv[1];

		if(*++p != '-') {
			--p;
		}
		while(*++p)
			if(!isdigit(*p)) {
				fprintf(stderr, "nice: argument must be numeric.\n");
				exit(1);
			}
		nicarg = atoi(&argv[1][1]);
		argc--, argv++;
	}
	if (argc < 2) {
		fputs("usage: nice [ -n ] command\n", stderr);
		exit(1);
	}
	if (setpriority(PRIO_PROCESS, 0, 
	    getpriority(PRIO_PROCESS, 0) + nicarg) < 0) {
		perror("nice: setpriority");
		exit(1);
	}
	(void) execvp(argv[1], &argv[1]);
	perror(argv[1]);
	exit(1);
	/* NOTREACHED */
}
