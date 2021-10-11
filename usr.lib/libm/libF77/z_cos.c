#ifndef lint
 static	char sccsid[] = "@(#)z_cos.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

z_cos(r, z)
dcomplex *r, *z;
{
double sin(), cos(), sinh(), cosh();

r->dreal = cos(z->dreal) * cosh(z->dimag);
r->dimag = - sin(z->dreal) * sinh(z->dimag);
}
