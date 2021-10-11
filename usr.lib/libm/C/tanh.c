
#ifdef lint
static	char sccsid[] = "@(#)tanh.c 1.1 92/07/30 Copyr 1986 Sun Micro;
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* TANH(X)
 * RETURN THE HYPERBOLIC TANGENT OF X
 * code based on 4.3bsd
 * Modified by K.C. Ng for sun 4.0, Jan 31, 1987
 *
 * Method :
 *	1. reduce x to non-negative by tanh(-x) = - tanh(x).
 *	2.
 *	    0      <  x <=  1.e-10 :  tanh(x) := x
 *					          -expm1(-2x)
 *	    1.e-10 <  x <=  1      :  tanh(x) := --------------
 *					         expm1(-2x) + 2
 *							  2
 *	    1      <= x <=  22.0   :  tanh(x) := 1 -  ---------------
 *						      expm1(2x) + 2
 *	    22.0   <  x <= INF     :  tanh(x) := 1.
 *
 *	Note: 22 was chosen so that fl(1.0+2/(expm1(2*22)+2)) == 1.
 *
 * Special cases:
 *	tanh(NaN) is NaN;
 *	only tanh(0)=0 is exact for finite argument.
 */

#include <math.h>

static double one=1.0, two=2.0, small = 1.0e-10, big = 1.0e10;

double tanh(x)
double x;
{
	double t,y,z;
	int signx;
	if(x!=x) return x+x;	/* x is NaN */
	signx = signbit(x); 
	t = fabs(x);
	z = one;
	if(t <= 22.0) {
	    if( t > one ) z = one - two/(expm1(t+t) +two);
	    else if ( t > small ) {
		y = expm1(-t-t);
		z = -y/(y+two);
	    } else {		/* raise the INEXACT flag for non-zero t */
		dummy(t+big);
		return x;
	    }
	}
	else if(!finite(t)) return copysign(1.0,x);
	else return (signx==1)? -z+small*small: z-small*small;

	return (signx==1)? -z: z;
}

static dummy(x)
double x;
{
	return 1;
}
