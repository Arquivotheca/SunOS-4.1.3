
#ifndef lint
static	char sccsid[] = "@(#)r_bessel_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_j0_(x)
float *x;
{
	float w;
	w = (float) j0((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_j1_(x)
float *x;
{
	float w;
	w = (float) j1((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_jn_(n,x)
float *x; int *n;
{
	float w;
	w = (float) jn(*n,(double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_y0_(x)
float *x;
{
	float w;
	w = (float) y0((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_y1_(x)
float *x;
{
	float w;
	w = (float) y1((double)*x);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_yn_(n,x)
float *x; int *n;
{
	float w;
	w = (float) yn(*n,(double)*x);
	RETURNFLOAT(w);
}

