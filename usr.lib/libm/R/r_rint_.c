
#ifndef lint
static	char sccsid[] = "@(#)r_rint_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_aint_(x)
float *x;
{
	float w;
	w = (float) aint((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_anint_(x)
float *x;
{
	float w;
	w = (float) anint((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_ceil_(x)
float *x;
{
	float w;
	w = (float) ceil((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_floor_(x)
float *x;
{
	float w;
	w = (float) floor((double)*x);
	RETURNFLOAT(w);
}

int ir_irint_(x)
float *x;
{
	return irint((double)*x);
}

int ir_nint_(x)
float *x;
{
	return nint((double)*x);
}

FLOATFUNCTIONTYPE r_rint_(x)
float *x;
{
	float w;
	w = (float) rint((double)*x);
	RETURNFLOAT(w);
}

