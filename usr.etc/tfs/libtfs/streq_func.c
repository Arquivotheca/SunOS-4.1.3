#ifndef lint
static char sccsid[] = "@(#)streq_func.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <nse/param.h>

/*
 * Function to compare two strings.
 */
int
_nse_streq_func(a, b)
	char		*a;
	char		*b;
{
	return(NSE_STREQ(a, b));
}
