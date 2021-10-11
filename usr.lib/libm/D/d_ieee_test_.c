
#ifndef lint
static	char sccsid[] = "@(#)d_ieee_test_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_logb_(x)
double *x;
{
	return logb(*x);
}

double d_scalb_(x,fn)
double *x,*fn;
{
	return scalb(*x,*fn);
}

double d_significand_(x)
double *x;
{
	return significand(*x);
}
