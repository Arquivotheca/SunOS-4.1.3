
#ifndef lint
static	char sccsid[] = "@(#)acos.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* acos(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :                  
 *			      ________
 *                           / 1 - x
 *	acos(x) = 2*atan2(  / -------- , 1 ) 
 *                        \/   1 + x
 *
 *			      ________
 *                           / 1 - x
 *		= 2*atan (  / -------- ) for non-exceptional x.
 *                        \/   1 + x
 *
 * Special cases:
 *	if x is NaN, return x itself;
 *	if |x|>1, return NaN with invalid signal.
 *
 */

#include <math.h>
#include "libm.h"

double acos(x)
double x;
{
	double t;
	if (x!=x)
		return x+x;
	else if(fabs(x)<1.0) 
		x=atan(sqrt((1.-x)/(1.+x)));
	else if(x==-1.0)
		x=atan2(1.0,0.0);	/* x <- PI/2 */
	else if(x==1.0)
		x=0.0;
	else {		/* |x| > 1  create invalid signal */
		return SVID_libm_err(x,x,1);
	}

	return x+x;
}
