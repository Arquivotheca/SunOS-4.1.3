#ifndef lint
 static	char sccsid[] = "@(#)c_exp.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

c_exp(r, z)
complex *r, *z;
{
register double expx;
double exp(), cos(), sin();

expx = exp(z->real);
r->real = expx * cos(z->imag);
r->imag = expx * sin(z->imag);
}
