
#ifndef lint
static	char sccsid[] = "@(#)cosh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* cosh(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	1. Replace x by |x| (cosh(x) = cosh(-x)). 
 *	2. 
 *		                                        [ exp(x) - 1 ]^2 
 *	    0        <= x <= 0.3465  :  cosh(x) := 1 + -------------------
 *			       			           2*exp(x)
 *
 *		                                   exp(x) +  1/exp(x)
 *	    0.3465   <= x <= 22      :  cosh(x) := -------------------
 *			       			           2
 *	    22       <= x <= lnovft  :  cosh(x) := exp(x)/2 
 *	    lnovft   <= x <  INF     :  cosh(x) := scalbn(exp(x-1024*ln2),1023)
 *
 *	Note: .3465 is a number near one half of ln2.
 *
 * Special cases:
 *	cosh(x) is |x| if x is +INF, -INF, or NaN.
 *	only cosh(0)=1 is exact for finite x.
 */

#include <math.h>
#include "libm.h"

double cosh(x)
double x;
{	
	double t,w;
	w = fabs(x);
	if(!finite(w)) return w+w;	/* x is INF or NaN */
	if( w < 0.3465 ) {
		t = expm1(w);
		w = 1.0+t;
		if(w!=1.0) w =  1.0+(t*t)/(w+w);
		return w;
	} else if (w < 22.) {
		t = exp(w);
		return 0.5*(t+1.0/t);
	} else if (w <= lnovft) return 0.5*exp(w);
	else {
		w = scalbn(exp((w-1024*ln2hi)-1024*ln2lo), 1023); 
		if(!finite(w)) w = SVID_libm_err(x,x,5);
		return w;
	}
}
