
#ifndef lint
static	char sccsid[] = "@(#)r_expm1_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_expm1_(x)
float *x;
{
	float w;
	w = (float) expm1((double)*x);
	RETURNFLOAT(w);
}
