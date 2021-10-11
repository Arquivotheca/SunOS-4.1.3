
#ifndef lint
static	char sccsid[] = "@(#)d_ieee_func_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_copysign_(x,y)
double *x, *y;
{
	return copysign(*x,*y);
}

int id_finite_(x)
double *x;
{
	return finite(*x);
}

int id_ilogb_(x)
double *x;
{
	return ilogb(*x);
}

int id_isnan_(x)
double *x;
{
	return isnan(*x);
}

int id_isinf_(x)
double *x;
{
	return isinf(*x);
}

int id_isnormal_(x)
double *x;
{
	return isnormal(*x);
}

int id_issubnormal_(x)
double *x;
{
	return issubnormal(*x);
}

int id_iszero_(x)
double *x;
{
	return iszero(*x);
}

double d_nextafter_(x,y)
double *x, *y;
{
	return nextafter(*x,*y);
}

double d_scalbn_(x,n)
double *x;
int *n;
{
	return scalbn(*x,*n);
}

int id_signbit_(x)
double *x;
{
	return signbit(*x);
}
