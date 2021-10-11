#ifndef lint
static  char sccsid[] = "@(#)pow_cc.c 1.1 92/07/30 SMI";
#endif
/*
 *	needs to be changed to do calculations in single FIXME
 */

#include "complex.h"

pow_cc(r,a,b)
complex *r, *a, *b;
{
double logr, logi, x, y;
double log(), exp(), cos(), sin(), atan2(), hypot();

logr = 0.5 * log( a->real * a->real + a->imag * a->imag );
logi = atan2(a->imag, a->real);

x = exp( logr * b->real - logi * b->imag );
y = logr * b->imag + logi * b->real;

r->real = x * cos(y);
r->imag = x * sin(y);
}
