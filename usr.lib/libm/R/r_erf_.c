
#ifndef lint
static	char sccsid[] = "@(#)r_erf_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_erf_(x)
float *x;
{
	float w;
	w = (float) erf((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_erfc_(x)
float *x;
{
	float w;
	w = (float) erfc((double)*x);
	RETURNFLOAT(w);
}

