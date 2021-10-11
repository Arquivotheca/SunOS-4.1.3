#ifndef lint
static	char sccsid[] = "@(#)r_tanh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_tanh(x)
float *x;
{
	return r_tanh_(x);
}
