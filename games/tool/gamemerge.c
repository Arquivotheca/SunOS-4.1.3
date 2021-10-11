#ifndef lint
static  char sccsid[] = "@(#)gamemerge.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

/* 
 * Merges chesstool, gammontool, boggletool, life and canfieldtool
 */


#include <stdio.h>

#define streq(a,b) (strcmp(a,b) == 0)

main(argc, argv)
	char **argv;
{
	char *p;
	
	p = (char *)rindex(argv[0], '/');
	if (p)
		p++;
	else
		p = argv[0];
	if (streq(p, "canfieldtool"))
		canfieldtool_main(argc, argv); 
	else if (streq(p, "chesstool"))
		chesstool_main(argc, argv); 
	else if (streq(p, "gammontool"))
		gammontool_main(argc, argv); 
	else if (streq(p, "boggletool"))
		boggletool_main(argc, argv); 
	else if (streq(p, "life"))
		life_main(argc, argv);
	else {
		fprintf(stderr, "Couldn't run %s\n", argv[0]);
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}
