
#ifndef lint
static	char sccsid[] = "@(#)annuity.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* annuity(r,n)
 *
 * Returns (1-(1+r)**-n)/r (return n when r==0); undefined for r <= -1.
 */

#include <math.h>
double annuity(r,n)
double r,n;
{
	double one=1.0,zero=0.0;
	if(r==zero||n!=n) 
		return n*one;
	else if(r!=r||r<= -1.0) 
		return (r-r)/zero;
	else if(n==zero) 
		return zero/r;
	else
		return -expm1(-n*log1p(r))/r;
}
