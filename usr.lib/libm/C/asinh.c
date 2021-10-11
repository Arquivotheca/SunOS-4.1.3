
#ifndef lint
static	char sccsid[] = "@(#)asinh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* asinh(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	Based on 
 *		asinh(x) = sign(x) * log [ |x| + sqrt(x*x+1) ]
 *	we have
 *	asinh(x) := x  if  1+x*x=1,
 *		 := sign(x)*(log(x)+ln2))	 if sqrt(1+x*x)=x, else
 *		 := sign(x)*log1p(|x| + |x|/(1/|x| + sqrt(1+(1/|x|)^2)) )  
 */

#include <math.h>
#include "libm.h"

double asinh(x)
double x;
{	
	double t,w;
	w = fabs(x);
	if(x!=x) return x+x; 	/* x is NaN */
	if(w < 1.E-10) {
		dummy(x+1.0e10);/* create inexact flag if x != 0 */
		return x;	/* tiny x */
		}
	else if (w < 1.E10) {
		t = 1.0/w;
		return copysign(log1p(w+w/(t+sqrt(1.0+t*t))),x);
		}
	else	
		return copysign(log(w)+ln2,x);
}

static dummy(x)
double x;
{
	return 1;
}

