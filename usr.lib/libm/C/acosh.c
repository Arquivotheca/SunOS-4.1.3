
#ifndef lint
static	char sccsid[] = "@(#)acosh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* acosh(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	Based on 
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log(x)+ln2,	if (x > 1.0E10); else		
 *		acosh(x) := log1p( sqrt(x-1) * (sqrt(x-1) + sqrt(x+1)) ) .
 *	These formulae avoid the over/underflow complication.
 *
 * Special cases:
 *	acosh(x) is NaN with signal if x<1.
 *	acosh(NaN) is NaN without signal.
 */

#include <math.h>
#include "libm.h"

double acosh(x)
double x;
{	
	double t;
	if(x!=x) 
		return x+x;	/* x is NaN */
	else if (x > 1.E10) 
		return log(x)+ln2;
	else if (x > 1.0) {
		t = sqrt(x-1.0);
		return log1p(t*(t+sqrt(x+1.0)));
	} else if (x==1.0) return 0.0;
	else return (x-x)/(x-x);
}
