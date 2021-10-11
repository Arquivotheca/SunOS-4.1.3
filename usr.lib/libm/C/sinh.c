
#ifndef lint
static	char sccsid[] = "@(#)sinh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* sinh(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :                  
 *	1. reduce x to non-negative by sinh(-x) = - sinh(x).
 *	2. 
 *
 *	                                      expm1(x) + expm1(x)/(expm1(x)+1)
 *	    0 <= x <= lnovft     : sinh(x) := --------------------------------
 *			       		                      2
 *     lnovft <= x <  INF	 : sinh(x) := exp(x-1024*ln2)*2**1023
 *	
 *
 * Special cases:
 *	sinh(x) is x if x is +INF, -INF, or NaN.
 *	only sinh(0)=0 is exact for finite argument.
 *
 */

#include <math.h>
#include "libm.h"

double sinh(x)
double x;
{
	double r,t;
	if(!finite(x)) return x+x;	/* sinh of NaN or +-INF is itself */
	r=fabs(x);
	if(r<lnovft) {t=expm1(r); r = copysign((t+t/(1.0+t))*0.5,x);}
	else { 
	   if (r<1000.0) x = copysign(exp((r-1024*ln2hi)-1024*ln2lo),x); 
	   r = scalbn(x,1023);
	}
	if (!finite(r)) r = SVID_libm_err(x,x,25);
	return r;
}
