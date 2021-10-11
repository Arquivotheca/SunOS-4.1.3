#ifndef lint
static	char sccsid[] = "@(#)r_abs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_abs(x)
float *x;
{
	return r_fabs_(x);
}
