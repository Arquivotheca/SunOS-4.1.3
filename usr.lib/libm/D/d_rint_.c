
#ifndef lint
static	char sccsid[] = "@(#)d_rint_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * see rint.c
 */

#include <math.h>

double d_aint_(x)
double *x;
{
	return aint(*x);
}

double d_anint_(x)
double *x;
{
	return anint(*x);
}

double d_ceil_(x)
double *x;
{
	return ceil(*x);
}

double d_floor_(x)
double *x;
{
	return floor(*x);
}

int id_irint_(x)
double *x;
{
	return irint(*x);
}

int id_nint_(x)
double *x;
{
	return nint(*x);
}

double d_rint_(x)
double *x;
{
	return rint(*x);
}
