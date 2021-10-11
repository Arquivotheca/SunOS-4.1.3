
#ifdef sccsid
static	char sccsid[] = "@(#)r_atan_.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* r_atan_(x)
 * Table look-up algorithm
 * By K.C. Ng, March 9, 1989
 *
 * Algorithm.
 *
 * The algorithm is based on atan(x)=atan(y)+atan((x-y)/(1+x*y)).
 * We use poly1(x) to approximate atan(x) for x in [0,1/8] with 
 * error (relative)
 * 	|(atan(x)-poly1(x))/x|<= 2^-115.94 	long double
 * 	|(atan(x)-poly1(x))/x|<= 2^-58.85	double 
 * 	|(atan(x)-poly1(x))/x|<= 2^-25.53 	float
 * and use poly2(x) to approximate atan(x) for x in [0,1/65] with
 * error (absolute)
 *	|atan(x)-poly2(x)|<= 2^-122.15		long double
 *	|atan(x)-poly2(x)|<= 2^-64.79		double
 *	|atan(x)-poly2(x)|<= 2^-35.36		float
 * and use poly3(x) to approximate atan(x) for x in [1/8,7/16] with
 * error (relative, on for single precision)
 * 	|(atan(x)-poly1(x))/x|<= 2^-25.53 	float
 *
 * Here poly1-3 are odd polynomial with the following form:
 *		x + x^3*(a1+x^2*(a2+...))
 *
 * (0). Purge off Inf and NaN and 0
 * (1). Reduce x to positive by atan(x) = -atan(-x).
 * (2). For x <= 1/8, use
 *	(2.1) if x < 2^(-prec/2-2), atan(x) =  x  with inexact
 *	(2.2) Otherwise
 *	   	atan(x) = poly1(x)
 * (3). For x >= 8 then
 *	(3.1) if x >= 2^(prec+2),   atan(x) = atan(inf) - pio2lo
 *	(3.2) if x >= 2^(prec/3+2), atan(x) = atan(inf) - 1/x
 *	(3.3) if x >  65,           atan(x) = atan(inf) - poly2(1/x)
 *	(3.4) Otherwise,	    atan(x) = atan(inf) - poly1(1/x)
 *
 * (4). Now x is in (0.125, 8)
 *      Find y that match x to 4.5 bit after binary (easy).
 *	If iy is the high word of y, then
 *		single : j = (iy - 0x3e000000) >> 19
 *		(single is modified to (iy-0x3f000000)>>19)
 *		double : j = (iy - 0x3fc00000) >> 16
 *		quad   : j = (iy - 0x3ffc0000) >> 12
 *
 *	Let s = (x-y)/(1+x*y). Then
 *	atan(x) = atan(y) + poly1(s)
 *		= _tbl_r_atan_hi[j] + (_tbl_r_atan_lo[j] + poly2(s) )
 *
 *	Note. |s| <= 1.5384615385e-02 = 1/65. Maxium occurs at x = 1.03125
 *
 */

#define GENERIC float

extern GENERIC _tbl_r_atan_hi[], _tbl_r_atan_lo[];
static GENERIC
big	=   1.0e37,
one	=   1.0,
p1      =  -3.333185951111688247225368498733544672172e-0001,
p2      =   1.969352894213455405211341983203180636021e-0001,
q1      =  -3.332921964095646819563419704110132937456e-0001,
a1	=  -3.333323465223893614063523351509338934592e-0001,
a2	=   1.999425625935277805494082274808174062403e-0001,
a3	=  -1.417547090509737780085769846290301788559e-0001,
a4	=   1.016250813871991983097273733227432685084e-0001,
a5	=  -5.137023693688358515753093811791755221805e-0002,
pio2hi  =   1.570796371e+0000,
pio2lo  =  -4.371139000e-0008;

#include <math.h>

FLOATFUNCTIONTYPE r_atan_(xx)
GENERIC *xx;
{
	GENERIC x,y,z,r,p,s;
	static dummy();
	int ix,iy,sign,j;

	x    = *xx;
	ix   = *(int*)&x;
	sign = ix&0x80000000; ix ^= sign;


    /* for |x| < 1/8 */
	if(ix < 0x3e000000) {
	    if(ix<0x38800000) {		/* if |x| < 2**(-prec/2-2) */
		dummy(big+x);	/* get inexact flag if x!=0 */
		RETURNFLOAT(x);
	    }
	    z = x*x;
	    if(ix<0x3c000000) {		/* if |x| < 2**(-prec/4-1) */
		x = x+(x*z)*p1;
		RETURNFLOAT(x);
	    } else {
		x = x+(x*z)*(p1+z*p2);
		RETURNFLOAT(x);
	    }
	}

    /* for |x| >= 8.0 */
	if(ix >= 0x41000000) {
	    *(int*)&x  = ix;
	    if(ix < 0x42820000) {		/* x <  65 */
		r = one/x; z = r*r;
		y = r*(one+z*(p1+z*p2));	/* poly1 */
		y-= pio2lo;
	    } else if(ix <  0x44800000) {	/* x <  2**(prec/3+2) */
		r = one/x; z = r*r;
		y = r*(one+z*q1);		/* poly2 */
		y-= pio2lo;
	    } else if(ix <  0x4c800000) {	/* x <  2**(prec+2) */
		y = one/x-pio2lo;
	    } else if(ix <  0x7f800000) {	/* x <  inf */	
		y = -pio2lo;
	    } else {				/* x is inf or NaN */
		if(ix > 0x7f800000) {
		    x = x-x;
		    RETURNFLOAT(x);
		}
		y = -pio2lo;
	    }

	    if(sign==0) 
		x =  pio2hi-y;
	    else 
		x =  y-pio2hi;
	    RETURNFLOAT(x);
	}


    /* now x is between 1/8 and 8 */
	if(ix<0x3f000000) {		/* between 1/8 and 1/2 */
	    z = x*x;
	    x = x+(x*z)*(a1+z*(a2+z*(a3+z*(a4+z*a5))));
	    RETURNFLOAT(x);
	}
	*(int*)&x  = ix;
	iy     = (ix+0x00040000)&0x7ff80000;
	*(int*)&y  = iy;
	j      = (iy-0x3f000000)>>19;

	if(ix==iy) 
	    p = x-y;	/* p=0.0 */
	else {
	    if(sign==0) s = (x-y)/(one+x*y);
	    else        s = (y-x)/(one+x*y);
	    z = s*s;
	    p = s*(one+z*q1);
	}
	if(sign==0) {
	    r = p+_tbl_r_atan_lo[j];
	    x = r+_tbl_r_atan_hi[j];
	} else {
	    r = p-_tbl_r_atan_lo[j];
	    x = r-_tbl_r_atan_hi[j];
	}
	RETURNFLOAT(x);
}

static dummy(x)
GENERIC x;
{
	return 0;
}

