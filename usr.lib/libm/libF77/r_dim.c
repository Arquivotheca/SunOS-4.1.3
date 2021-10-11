#ifndef lint
static	char sccsid[] = "@(#)r_dim.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_dim(x,y)
float *x, *y;
{
	float result;

	result = (*x > *y) ? *x - *y : 0.0;
	RETURNFLOAT(result);
}
