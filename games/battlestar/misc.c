#ifndef lint
static	char sccsid[] = "@(#)misc.c 1.1 92/07/30 SMI"; /* from UCB 1.2 85/04/24 */
#endif

/*
 * Copyright (c) 1983 Regents of the University of California,
 * All rights reserved.  Redistribution permitted subject to
 * the terms of the Berkeley Software License Agreement.
 */

#include "externs.h"

card(array, size)		/* for beenthere, injuries */
	register char *array;
	int size;
{
	register char *end = array + size;
	register int i = 0;

	while (array < end)
		if (*array++)
			i++;
	return (i);
}

ucard(array)
	register unsigned *array;
{
	register int j = 0, n;

	for (n = 0; n < NUMOFOBJECTS; n++)
		if (testbit(array, n))
			    j++;
	return (j);
}
