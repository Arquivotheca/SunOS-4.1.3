#ifndef lint
 static	char sccsid[] = "@(#)c_log.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

c_log(r, z)
complex *r, *z;
{
double log(), hypot(), atan2();

r->imag = atan2(z->imag, z->real);
r->real = 0.5 * log( (z->real * z->real) + (z->imag * z->imag));
}
