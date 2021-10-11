#ifndef lint
static char sccsid[] = "@(#)nse_malloc.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <nse/param.h>

/*
 * Check the results of malloc() and calloc() and exit with an error
 * message if the desired space can't be allocated.
 */

char *
nse_malloc(size)
	unsigned	size;
{
	char		*data;

	data = malloc(size);
	if (data != NULL)
		return data;
	else {
		fprintf(stderr,
			"malloc(%u) failed (Out of swap space?)\n",
			size);
		exit(1);
		/* NOTREACHED */
	}
}


char *
nse_calloc(nelem, elsize)
	unsigned	nelem;
	unsigned	elsize;
{
	char		*data;

	data = calloc(nelem, elsize);
	if (data != NULL)
		return data;
	else {
		fprintf(stderr,
			"calloc(%u, %u) failed (Out of swap space?)\n",
			nelem, elsize);
		exit(1);
		/* NOTREACHED */
	}
}
