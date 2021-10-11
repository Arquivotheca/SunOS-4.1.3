
#ifndef lint
static	char sccsid[] = "@(#)d_trigpi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <math.h>

extern double cospi(), sinpi(), tanpi();  /* Helps compile under 4.0 */

double d_cospi_(x)
double *x;
{
	return cospi(*x);
}

double d_sinpi_(x)
double *x;
{
	return sinpi(*x);
}

double d_tanpi_(x)
double *x;
{
	return tanpi(*x);
}

void d_sincospi_(x,s,c)
double *x,*s,*c;
{
	sincospi(*x,s,c);
}
