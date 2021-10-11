#ifndef lint
static	char sccsid[] = "@(#)r_atan.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_atan(x)
float *x;
{
	return r_atan_(x);
}
