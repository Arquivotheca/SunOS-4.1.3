
#ifndef lint
static	char sccsid[] = "@(#)r_ieee_test_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

static double fmin = 1.0e-300, fmax = 1.0e300, fzero = 0.0;

FLOATFUNCTIONTYPE r_logb_(x)
float *x;
{
	float w;
	long *px = (long *) x, k;
	k = (px[0]&0x7f800000);
	if((px[0]&0x7fffffff)==0) w= -fmax/fzero;
	else if(k==0x7f800000) w = (*x) * (*x) ;
		/* logb(+-inf) -> +inf; logb(snan) gets signal. */
	else w = (float) ((k==0) ? (-126): ((k>>23)-127));
	RETURNFLOAT(w);
}


FLOATFUNCTIONTYPE r_scalb_(x,fn)
float *x,*fn;
{
	float w;
	w = (float) scalb((double)*x,(double) *fn);
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_significand_(x)
float *x;
{
	float f;
	f = - (float) ir_ilogb_(x);
	return r_scalb_(x,&f);
}

