#ifndef lint
 static	char sccsid[] = "@(#)c_sqrt.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 */

#include "complex.h"

c_sqrt(r, z)
complex *r, *z;
{
double magsquared, sqrt(), hypot();

if( (magsquared = (z->real * z->real + z->imag * z->imag)) == 0.)
	r->real = r->imag = 0.;
else if(z->real > 0)
	{
	r->real = sqrt(0.5 * (sqrt(magsquared) + z->real) );
	r->imag = 0.5 * z->imag / r->real ;
	}
else
	{
	r->imag = sqrt(0.5 * (sqrt(magsquared) - z->real) );
	if(z->imag < 0)
		r->imag = - r->imag;
	r->real = 0.5 * z->imag / r->imag ;
	}
}
