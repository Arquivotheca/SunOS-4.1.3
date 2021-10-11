
#ifndef lint
static	char sccsid[] = "@(#)log1p.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* LOG1P(x) 
 * RETURN THE LOGARITHM OF 1+x
 * IEEE DOUBLE PRECISION 
 * CODE BASED ON 4.3BSD, MODIFIED BY K.C. NG, 6/29/87. 
 * 
 * Required system supported functions:
 *	scalbn(x,n) 
 *	copysign(x,y)
 *	ilogb(x)	
 *	finite(x)
 *
 * Required kernel function:
 *	log__L(z)
 *
 * Method :
 *	1. Argument Reduction: find k and f such that 
 *			1+x  = 2^k * (1+f), 
 *	   where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *	2. Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *		 = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *	   log(1+f) is computed by
 *
 *	     		log(1+f) = 2s + s*log__L(s*s)
 *	   where
 *		log__L(z) = z*(L1 + z*(L2 + z*(... (L6 + z*L7)...)))
 *
 *	   See log__L() for the values of the coefficients.
 *
 *	3. Finally,  log(1+x) = k*ln2 + log(1+f).  
 *
 *	Remarks 1. In step 3 n*ln2 will be stored in two floating point numbers
 *		   n*ln2hi + n*ln2lo, where ln2hi is chosen such that the last 
 *		   20 bits (for VAX D format), or the last 21 bits ( for IEEE 
 *		   double) is 0. This ensures n*ln2hi is exactly representable.
 *		2. In step 1, f may not be representable. A correction term c
 *	 	   for f is computed. It follows that the correction term for
 *		   f - t (the leading term of log(1+f) in step 2) is c-c*x. We
 *		   add this correction term to n*ln2lo to attenuate the error.
 *
 *
 * Special cases:
 *	log1p(x) is NaN with signal if x < -1; log1p(NaN) is NaN with no signal;
 *	log1p(INF) is +INF; log1p(-1) is -INF with signal;
 *	only log1p(0)=0 is exact for finite argument.
 *
 * Accuracy:
 *	log1p(x) returns the exact log(1+x) nearly rounded. In a test run 
 *	with 1,536,000 random arguments on a VAX, the maximum observed
 *	error was .846 ulps (units in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include "libm.h"

double log1p(x)
double x;
{
	static double zero=0.0, negone= -1.0, one=1.0, 
		      half=1.0/2.0, small=1.0E-20;   /* 1+small == 1 */
	double log__L(),z,s,t,c;
	int ilogb();
	int k,finite();

	if(!finite(x)) return Inf+x;	/* x is +-INF or NaN */
	else { if( x > negone ) {

	   /* argument reduction */
	      if(fabs(x)<small) {
		   dummy(fmax-fabs(x));	/* raise inexact if x is not zero */
		   return(x);
	      }
	      if (x>1e300) k=ilogb(x); else k=ilogb(one+x); 
	      z=scalbn(x,-k); t=scalbn(one,-k);
	      if(z+t >= sqrt2 ) 
		  { k += 1 ; z *= half; t *= half; }
	      t += negone; x = z + t;
	      c = (t-x)+z ;		/* correction term for x */

 	   /* compute log(1+x)  */
              s = x/(2+x); t = x*x*half;
	      c += (k*ln2lo-c*x);
	      z = c+s*(t+log__L(s*s));
	      x += (z - t) ;

	      return(k*ln2hi+x);
	   }
	/* end of if (x > negone) */

	    else {
		if ( x == negone ) return( negone/zero );
	        else return ( zero / zero );
	   }
	}
}

#ifdef VAX	/* VAX D format (56 bits) */
/* static double */
/* L1     =  6.6666666666666703212E-1    , Hex  2^  0   *  .AAAAAAAAAAAAC5 */
/* L2     =  3.9999999999970461961E-1    , Hex  2^ -1   *  .CCCCCCCCCC2684 */
/* L3     =  2.8571428579395698188E-1    , Hex  2^ -1   *  .92492492F85782 */
/* L4     =  2.2222221233634724402E-1    , Hex  2^ -2   *  .E38E3839B7AF2C */
/* L5     =  1.8181879517064680057E-1    , Hex  2^ -2   *  .BA2EB4CC39655E */
/* L6     =  1.5382888777946145467E-1    , Hex  2^ -2   *  .9D8551E8C5781D */
/* L7     =  1.3338356561139403517E-1    , Hex  2^ -2   *  .8895B3907FCD92 */
/* L8     =  1.2500000000000000000E-1    , Hex  2^ -2   *  .80000000000000 */
static long        L1x[] = { 0xaaaa402a, 0xaac5aaaa};
static long        L2x[] = { 0xcccc3fcc, 0x2684cccc};
static long        L3x[] = { 0x49243f92, 0x578292f8};
static long        L4x[] = { 0x8e383f63, 0xaf2c39b7};
static long        L5x[] = { 0x2eb43f3a, 0x655ecc39};
static long        L6x[] = { 0x85513f1d, 0x781de8c5};
static long        L7x[] = { 0x95b33f08, 0xcd92907f};
static long        L8x[] = { 0x00003f00, 0x00000000};
#define       L1    (*(double*)L1x)
#define       L2    (*(double*)L2x)
#define       L3    (*(double*)L3x)
#define       L4    (*(double*)L4x)
#define       L5    (*(double*)L5x)
#define       L6    (*(double*)L6x)
#define       L7    (*(double*)L7x)
#define       L8    (*(double*)L8x)
#else	/* IEEE double */
static double
L1     =  6.6666666666667340202E-1    , /*Hex  2^ -1   *  1.5555555555592 */
L2     =  3.9999999999416702146E-1    , /*Hex  2^ -2   *  1.999999997FF24 */
L3     =  2.8571428742008753154E-1    , /*Hex  2^ -2   *  1.24924941E07B4 */
L4     =  2.2222198607186277597E-1    , /*Hex  2^ -3   *  1.C71C52150BEA6 */
L5     =  1.8183562745289935658E-1    , /*Hex  2^ -3   *  1.74663CC94342F */
L6     =  1.5314087275331442206E-1    , /*Hex  2^ -3   *  1.39A1EC014045B */
L7     =  1.4795612545334174692E-1    ; /*Hex  2^ -3   *  1.2F039F0085122 */
#endif

static double log__L(z)
double z;
{
#ifdef VAX
    return(z*(L1+z*(L2+z*(L3+z*(L4+z*(L5+z*(L6+z*(L7+z*L8))))))));
#else	/* IEEE double */
    return(z*(L1+z*(L2+z*(L3+z*(L4+z*(L5+z*(L6+z*L7)))))));
#endif
}

static dummy(x)
double x;
{
	return 1;
}
