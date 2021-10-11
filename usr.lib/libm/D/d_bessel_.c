#ifndef lint
static	char sccsid[] = "@(#)d_bessel_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_j0_(x)
double *x;
{
	return j0(*x);
}

double d_j1_(x)
double *x;
{
	return j1(*x);
}

double d_jn_(n,x)
double *x; int *n;
{
	return jn(*n,*x);
}

double d_y0_(x)
double *x;
{
	return y0(*x);
}

double d_y1_(x)
double *x;
{
	return y1(*x);
}

double d_yn_(n,x)
double *x; int *n;
{
	return yn(*n,*x);
}
