#ifndef lint
static char sccsid[] = "@(#)hashutil.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <nse/param.h>

/*
 * Return a hash value of the string.
 */ 
int
_nse_hash_string(string)
	char 		*string;
{
	int sum;
	
	for (sum = 0; *string; sum += *string++)
		;
	return sum;
}

/*
 * Function to dispose of a string.
 */
void
_nse_dispose_func(s)
	char		*s;
{
	NSE_DISPOSE(s);
}
