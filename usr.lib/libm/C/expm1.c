
#ifndef lint
static	char sccsid[] = "@(#)expm1.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* EXPM1(X)
 * RETURN  EXP(X) - 1
 * IEEE DOUBLE PRECISION 
 * CODE BASED ON 4.3BSD, MODIFIED by K.C. NG, 6/29/87. 
 *
 * Required system supported functions:
 *	scalbn(x,n)	
 *	copysign(x,y)	
 *	finite(x)
 *
 * Method:
 *	1. Argument Reduction: given the input x, find r and integer k such 
 *	   that
 *	                   x = k*ln2 + r,  |r| <= 0.5*ln2 .  
 *	   r will be represented as r := z+c for better accuracy.
 *
 *	2. Compute EXPM1(r)=exp(r)-1 by 
 *
 *			EXPM1(r=z+c) := z + exp__E(z,c)
 *
 *	3. EXPM1(x) =  2^k * ( EXPM1(r) + 1-2^-k ).
 *
 * 	Remarks: 
 *	   1. When k=1 and z < -0.25, we use the following formula for
 *	      better accuracy:
 *			EXPM1(x) = 2 * ( (z+0.5) + exp__E(z,c) )
 *	   2. To avoid rounding error in 1-2^-k where k is large, we use
 *			EXPM1(x) = 2^k * { [z+(exp__E(z,c)-2^-k )] + 1 }
 *	      when k>56. 
 *
 * Special cases:
 *	EXPM1(INF) is INF, EXPM1(NaN) is NaN;
 *	EXPM1(-INF)= -1;
 *	for finite argument, only EXPM1(0)=0 is exact.
 *
 * Accuracy:
 *	EXPM1(x) returns the exact (exp(x)-1) nearly rounded. In a test run with
 *	1,166,000 random arguments on a VAX, the maximum observed error was
 *	.872 ulps (units of the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include "libm.h"

double expm1(x)
double x;
{
	double static one=1.0, half=1.0/2.0; 
	double scalbn(),copysign(),exp__E(),z,hi,lo,c;
	int k,finite();
#ifdef VAX
	static prec=56;
#else	/* IEEE double */
	static prec=53;
#endif
	if(!finite(x)) {if(x!=x||x>0.0) return x+x; else return -1.0;}
	if(signbit(x)) {
	    if (x > -0.346573590279972643113) {  /* |x| < (ln2)/2 ? */
		if(x > -1e-17) {
		    dummy(x+fmax);	/* inexact unless x=0 */
		    return x;
		} else return x+exp__E(x,0.0);
	    } else if ( x > -0.693147180559945286227) {
		hi = x + ln2hi;
		z  = hi + ln2lo;
		c  = (hi-z)+ln2lo;
		return 0.5*(z+exp__E(z,c))-0.5;
	    } else if ( x > -40.0) return exp(x)-1.0;
	    else { dummy(1e-300+x); return fmin-1.0;}
	} else {
	    if (x < 0.346573590279972643113) {  /* x < (ln2)/2 ? */
		if(x < 1e-17) {
		    dummy(x-fmax);	/* inexact unless x=0 */
		    return x;
		} else return x+exp__E(x,0.0);
	    } else if ( x < 1.03972077083991792934) {
		hi = x - ln2hi;
		z  = hi - ln2lo;
		c  = (hi-z)-ln2lo;
		if (x < 0.443147180559945286227) {
		    x  = z+0.5; x += exp__E(z,c); 
		} else {
		    z += exp__E(z,c); x = 0.5 + z;
		}
		return x+x;
	    } else if ( x < 70.0) {
		k= invln2 *x+copysign(0.5,x);	/* k=NINT(x/ln2) */
		hi=x-k*ln2hi ; 
		z=hi-(lo=k*ln2lo);
		c=(hi-z)-lo;
		if(k<=prec) { x=one-scalbn(one,-k); z += exp__E(z,c);}
		else { x = exp__E(z,c)-scalbn(one,-k); x+=z; z=one;}
		return scalbn(x+z,k);
	    } else if (x <= lnovft) return exp(x)-1.0;
	    else return exp(x);
	}
}

/* exp__E(x,c)
 * ASSUMPTION: c << x  SO THAT  fl(x+c)=x.
 * (c is the correction term for x)
 * exp__E RETURNS
 *
 *			 /  exp(x+c) - 1 - x ,  1E-19 < |x| < .3465736
 *       exp__E(x,c) = 	| 		     
 *			 \  0 ,  |x| < 1E-19.
 *
 * DOUBLE PRECISION (IEEE 53 bits, VAX D FORMAT 56 BITS)
 * KERNEL FUNCTION OF EXP, EXPM1, POW FUNCTIONS
 * CODED IN C BY K.C. NG, 1/31/85;
 * REVISED BY K.C. NG on 3/16/85, 4/16/85.
 *
 * Required system supported function:
 *	copysign(x,y)	
 *
 * Method:
 *	1. Rational approximation. Let r=x+c.
 *	   Based on
 *                                   2 * sinh(r/2)     
 *                exp(r) - 1 =   ----------------------   ,
 *                               cosh(r/2) - sinh(r/2)
 *	   exp__E(r) is computed using
 *                   x*x            (x/2)*W - ( Q - ( 2*P  + x*P ) )
 *                   --- + (c + x*[---------------------------------- + c ])
 *                    2                          1 - W
 * 	   where  P := p1*x^2 + p2*x^4,
 *	          Q := q1*x^2 + q2*x^4 (for 56 bits precision, add q3*x^6)
 *	          W := x/2-(Q-x*P),
 *
 *	   (See the listing below for the values of p1,p2,q1,q2,q3. The poly-
 *	    nomials P and Q may be regarded as the approximations to sinh
 *	    and cosh :
 *		sinh(r/2) =  r/2 + r * P  ,  cosh(r/2) =  1 + Q . )
 *
 *         The coefficients were obtained by a special Remez algorithm.
 *
 * Approximation error:
 *
 *   |	exp(x) - 1			   |        2**(-57),  (IEEE double)
 *   | ------------  -  (exp__E(x,0)+x)/x  |  <= 
 *   |	     x			           |	    2**(-69).  (VAX D)
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#ifdef VAX	/* VAX D format */
/* static double */
/* p1     =  1.5150724356786683059E-2    , Hex  2^ -6   *  .F83ABE67E1066A */
/* p2     =  6.3112487873718332688E-5    , Hex  2^-13   *  .845B4248CD0173 */
/* q1     =  1.1363478204690669916E-1    , Hex  2^ -3   *  .E8B95A44A2EC45 */
/* q2     =  1.2624568129896839182E-3    , Hex  2^ -9   *  .A5790572E4F5E7 */
/* q3     =  1.5021856115869022674E-6    ; Hex  2^-19   *  .C99EB4604AC395 */
static long        p1x[] = { 0x3abe3d78, 0x066a67e1};
static long        p2x[] = { 0x5b423984, 0x017348cd};
static long        q1x[] = { 0xb95a3ee8, 0xec4544a2};
static long        q2x[] = { 0x79053ba5, 0xf5e772e4};
static long        q3x[] = { 0x9eb436c9, 0xc395604a};
#define       p1    (*(double*)p1x)
#define       p2    (*(double*)p2x)
#define       q1    (*(double*)q1x)
#define       q2    (*(double*)q2x)
#define       q3    (*(double*)q3x)
#else	/* IEEE double */
static double 
p1     =  1.3887401997267371720E-2    , /*Hex  2^ -7   *  1.C70FF8B3CC2CF */
p2     =  3.3044019718331897649E-5    , /*Hex  2^-15   *  1.15317DF4526C4 */
q1     =  1.1110813732786649355E-1    , /*Hex  2^ -4   *  1.C719538248597 */
q2     =  9.9176615021572857300E-4    ; /*Hex  2^-10   *  1.03FC4CB8C98E8 */
#endif

static double exp__E(x,c)
double x,c;
{
	double static zero=0.0, one=1.0, half=1.0/2.0;
	double copysign(),z,p,q,xp,xh,w;
           z = x*x  ;
	   p = z*( p1 +z* p2 );
#ifdef VAX
           q = z*( q1 +z*( q2 +z* q3 ));
#else	/* IEEE double */
           q = z*( q1 +z*  q2 );
#endif
           xp= x*p     ; 
	   xh= x*half  ;
           w = xh-(q-xp)  ;
	   p = p+p;
	   c += x*((xh*w-(q-(p+xp)))/(one-w)+c);
	   return(z*half+c);
}

static dummy(x)
double x;
{
	return 1;
}
