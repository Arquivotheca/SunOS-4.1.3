#ifndef lint
static char     sccsid[] = "@(#)foption.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <string.h>

/*
 * FOPTION(1) - function to determine if particular floating point options
 * are available at run time.
 */

main(argc, argv)
	int             argc;
	char           *argv[];
{

#ifdef sparc
	if (argc > 0)
		exit(1);	/* fail if a specific argument requested */
#endif
#ifdef mc68000
	if (argc == 1) {	/* printf name of -fswitch option. */
		if (winitfp_() == 1)
			printf("fpa\n");
		else if (minitfp_() == 1)
			printf("68881\n");
		else if (sinitfp_() == 1)
			printf("sky\n");
		else
			printf("soft\n");
	} else {		/* Exit status true if argv[1] can run. */
		int             result = 0;
		char           *option;
		option = argv[1];	/* Get pointer to option name. */
		if (strcmp(option, "-ffpa") == 0)
			result = winitfp_();
		else if (strcmp(option, "-f68881") == 0)
			result = minitfp_();
		else if (strcmp(option, "-fsky") == 0)
			result = sinitfp_();
		else if (strcmp(option, "-fsoft") == 0)
			result = 1;
		else if (strcmp(option, "-fswitch") == 0)
			result = 1;
		exit(1 - result);
	}
#endif
	exit(0);
	/* NOTREACHED */
}
