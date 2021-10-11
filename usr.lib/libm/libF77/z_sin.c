#ifndef lint
 static	char sccsid[] = "@(#)z_sin.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

z_sin(r, z)
dcomplex *r, *z;
{
double sin(), cos(), sinh(), cosh();

r->dreal = sin(z->dreal) * cosh(z->dimag);
r->dimag = cos(z->dreal) * sinh(z->dimag);
}
