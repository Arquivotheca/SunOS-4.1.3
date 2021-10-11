#ifndef lint
static	char sccsid[] = "@(#)asin.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* asin(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :                  
 *
 *	asin(x) = atan2(x,sqrt(1-x*x)); for better accuracy, 1-x*x is 
 *		  computed as follows
 *			1-x*x                     if x <  0.5, 
 *			2*(1-|x|)-(1-|x|)*(1-|x|) if x >= 0.5.
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return NaN with invalid signal.
 *
 */

#include <math.h>
#include "libm.h"

double asin(x)
double x;
{
	double t,w;
	w = fabs(x);
	if (x!=x)
		return x+x;
	else if(w <= 0.5) {
		if(w<1.0e-9) {dummy(1.0e9+w); return x;} else
		return atan(x/sqrt(1.-x*x));
	}
	else if(w <  1.0) {
		t = 1.0-w;
		w = t+t;
		return atan(x/sqrt(w-t*t));
		}
	else if(w == 1.0)
		return atan2(x,0.0);	/* asin(+-1) =  +- PI/2 */
	else 	{		/* |x| > 1 create invalid signal */
		return SVID_libm_err(x,x,2);
		}

}

static dummy(x)
double x;
{
	return 1;
}
