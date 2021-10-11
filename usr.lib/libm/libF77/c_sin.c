#ifndef lint
 static	char sccsid[] = "@(#)c_sin.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

c_sin(r, z)
complex *r, *z;
{
double sin(), cos(), sinh(), cosh();

r->real = sin(z->real) * cosh(z->imag);
r->imag = cos(z->real) * sinh(z->imag);
}
