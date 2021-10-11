#ifndef lint
 static	char sccsid[] = "@(#)z_sqrt.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

z_sqrt(r, z)
dcomplex *r, *z;
{
double mag, sqrt(), hypot();

if( (mag = hypot(z->dreal, z->dimag)) == 0.)
	r->dreal = r->dimag = 0.;
else if(z->dreal > 0)
	{
	r->dreal = sqrt(0.5 * (mag + z->dreal) );
	r->dimag = z->dimag / r->dreal / 2;
	}
else
	{
	r->dimag = sqrt(0.5 * (mag - z->dreal) );
	if(z->dimag < 0)
		r->dimag = - r->dimag;
	r->dreal = z->dimag / r->dimag / 2;
	}
}
