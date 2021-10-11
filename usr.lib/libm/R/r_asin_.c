
#ifndef lint
static	char sccsid[] = "@(#)r_asin_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_asin_(x)
float *x;
{
	float w;
	w = (float) asin((double)*x);
	RETURNFLOAT(w);
}
