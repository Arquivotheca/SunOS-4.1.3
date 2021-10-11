
#ifndef lint
static	char sccsid[] = "@(#)atanh.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* atanh(x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :
 *                  1              2x                          x
 *	atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                  2             1 - x                      1 - x
 * Note: to guarantee atanh(-x) = -atanh(x), we use
 *                 sign(x)             |x|  
 *	atanh(x) = ------- * log1p(2*-------).
 *                    2              1 - |x|
 *
 * Special cases:
 *	atanh(x) is NaN if |x| > 1 with signal;
 *	atanh(NaN) is that NaN with no signal;
 *	atanh(+-1) is +-INF with signal.
 *
 */

#include <math.h>
#include "libm.h"

double atanh(x)
double x;
{	
	double t;
	t = fabs(x);
	if(t==1.0) return x/0.0;
	t = t/(1.0-t);
	return copysign(0.5,x)*log1p(t+t);
}
