#ifndef lint
static char sccsid[] = "@(#)fftruncate.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <stdio.h>

/*
 * Routine to truncate a file opened with fopen(3S).  Will usually be used
 * with files that have been opened with type "r+".
 */
int
nse_fftruncate(filep)
	FILE		*filep;
{
	rewind(filep);
	if (ftruncate(fileno(filep), 0L)) {
		return (-1);
	}
	return (0);
}
