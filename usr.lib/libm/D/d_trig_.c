
#ifndef lint
static	char sccsid[] = "@(#)d_trig_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_cos_(x)
double *x;
{
	return cos(*x);
}

double d_sin_(x)
double *x;
{
	return sin(*x);
}

double d_tan_(x)
double *x;
{
	return tan(*x);
}

void d_sincos_(x,s,c)
double *x,*s,*c;
{
	sincos(*x,s,c);
}
