#ifndef lint
static	char sccsid[] = "@(#)pow.c 1.1 92/07/30 SMI"; /* from UCB 4.3  8/21/85 */
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* 
 * Copyright (c) 1985 Regents of the University of California.
 * 
 * Use and reproduction of this software are granted  in  accordance  with
 * the terms and conditions specified in  the  Berkeley  Software  License
 * Agreement (in particular, this entails acknowledgement of the programs'
 * source, and inclusion of this notice) with the additional understanding
 * that  all  recipients  should regard themselves as participants  in  an
 * ongoing  research  project and hence should  feel  obligated  to report
 * their  experiences (good or bad) with these elementary function  codes,
 * using "sendbug 4bsd-bugs@BERKELEY", to the authors.
 */

/* pow(x,y)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 *
 * Required static kernel functions:
 *	exp__E(a,c)	...return  exp(a+c) - 1 - a*a/2
 *	log__L(x)	...return  (log(1+x) - 2s)/s, s=x/(2+x) 
 *	pow_p(x,y)	...return  +(anything)**(finite non zero)
 *
 * Method
 *	1. Compute and return log(x) in three pieces:
 *		log(x) = n*ln2 + hi + lo,
 *	   where n is an integer.
 *	2. Perform y*log(x) by simulating muti-precision arithmetic and 
 *	   return the answer in three pieces:
 *		y*log(x) = m*ln2 + hi + lo,
 *	   where m is an integer.
 *	3. Return x**y = exp(y*log(x))
 *		= 2^m * ( exp(hi+lo) ).
 *
 * Special cases:
 *	1.  (anything) ** 0  is 1
 *	2.  (anything) ** 1  is itself
 *	3.  (anything) ** NAN is NAN
 *	4.  NAN ** (anything except 0) is NAN
 *	5.  +-(|x| > 1) **  +INF is +INF
 *	6.  +-(|x| > 1) **  -INF is +0
 *	7.  +-(|x| < 1) **  +INF is +0
 *	8.  +-(|x| < 1) **  -INF is +INF
 *	9.  +-1         ** +-INF is NAN
 *	10. +0 ** (+anything except 0, NAN)               is +0
 *	11. -0 ** (+anything except 0, NAN, odd integer)  is +0
 *	12. +0 ** (-anything except 0, NAN)               is +INF
 *	13. -0 ** (-anything except 0, NAN, odd integer)  is +INF
 *	14. -0 ** (odd integer) = -( +0 ** (odd integer) )
 *	15. +INF ** (+anything except 0,NAN) is +INF
 *	16. +INF ** (-anything except 0,NAN) is +0
 *	17. -INF ** (anything)  = -0 ** (-anything)
 *	18. (-anything) ** (integer) is (-1)**(integer)*(+anything**integer)
 *	19. (-anything except 0 and inf) ** (non-integer) is NAN
 *
 * Accuracy:
 *	pow(x,y) returns x**y nearly rounded. In particular, on a SUN, a VAX,
 *	and a Zilog Z8000,
 *			pow(integer,integer)
 *	always returns the correct integer provided it is representable.
 *	In a test run with 100,000 random arguments with 0 < x, y < 20.0
 *	on a VAX, the maximum observed error was 1.79 ulps (units in the 
 *	last place).
 *
 * Constants :
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include "libm.h"

static double log__L(), exp__E(), pow_p();

static double zero=0.0, half=1.0/2.0, one=1.0, two=2.0, negone= -1.0;


static double dummy(x)
double x;
{
	return x;
}

double pow(x,y)  	
double x,y;
{
	double t,z;

	if     (y==zero) {
		t = one;
		if (iszero(x)) t = SVID_libm_err(x,y,20);
		return t ;			/* if y is 0 return 1 */
		}
	else if(x!=x||y!=y) return x+y;		/* if x or y is NaN */
	else if(y==one) return x;      		/* if y=1 */
	else if(!finite(y)) {                   /* if y is INF */
		if((t=fabs(x))==one) return(zero/zero);
		else if(t>one) return((y>zero)?y:zero);
		else return((y<zero)?-y:zero);
	} else if(x==zero||!finite(x)) {	/* x is +-0, +-Inf */
	    	if (y<zero) {
		    z = one/fabs(x);
		    if(signbit(x)&&(t=rint(y))==y&&(t-2*floor(0.5*y))==one)
			z = -dummy(z);			/* (-x)**odd = -(x**odd) */
		    if(x==zero) z = SVID_libm_err(x,y,23);
		    return z;
		} else {
		    z = fabs(x);
		    if(signbit(x)&&(t=rint(y))==y&&(t-2*floor(0.5*y))==one)
		    	z = -dummy(z);			/* (-x)**odd = -(x**odd) */
		    return z;
		}
	} else if(y==two) z = x*x;
	else if(y==negone) z = one/x;

    /* signbit(x) = 0, x is positive */
	else if(signbit(x)==0) z = pow_p(x,y);

    /* signbit(x) = 1, x is negative; check if y is an integer */
	else if ( (t=rint(y)) == y) {
		z = pow_p( -x,y);
		if ((t-2*floor(y*0.5)==one)) 
			z= -z; 			/* (-x)**odd = -(x**odd) */
	}
    /* Henceforth neg**non-integral */
	else {		/* invalid operation */
		return SVID_libm_err(x,y,24);
	}

	if(!finite(z)) 
		z = SVID_libm_err(x,y,21);	/* overflow */
	if(z==0.0) 
		z = SVID_libm_err(x,y,22);	/* underflow */
	return z;
}

/* pow_p(x,y) return x**y for non-zero finite x with sign=1 and finite y */

static double pow_p(x,y)       
double x,y;
{
        double c,s,t,z,tx,ty;
        float sx,sy;
	long k=0;
        int n,m,is, ilogb();

	if(x==1.0) return(x);	/* if x=1.0, return 1 since y is finite */

    /* reduce x to z in [sqrt(1/2)-1, sqrt(2)-1] */
        z=scalbn(x,-(n=ilogb(x)));  
        if(z >= sqrt2 ) {n += 1; z *= half;}  z -= one ;

    /* log(x) = nlog2+log(1+z) ~ nlog2 + t + tx */
	s=z/(two+z); c=z*z*half; tx=s*(c+log__L(s*s)); 
	t= z-(c-tx); tx += (z-t)-c;

   /* if y*log(x) is neither too big nor too small */
	if((is=ilogb(y)+ilogb(n+t)) < 12) 
	    if(is>-60) {

	/* compute y*log(x) ~ mlog2 + t + c */
        	s=y*(n+invln2*t);
		m=nint(s);		/* m := nint(y*log(x)) */ 
		k=y; 
		if((double)k==y) {	/* if y is an integer */
		    k = m-k*n;
		    sx=t; tx+=(t-sx); }
		else	{		/* if y is not an integer */    
		    k =m;
	 	    tx+=n*ln2lo;
		    sx=(c=n*ln2hi)+t; tx+=(c-sx)+t; }
	   /* end of checking whether k==y */

                sy=y; ty=y-sy;          /* y ~ sy + ty */
		s=(double)sx*sy-k*ln2hi;        /* (sy+ty)*(sx+tx)-kln2 */
		z=(tx*ty-k*ln2lo);
		tx=tx*sy; ty=sx*ty;
		t=ty+z; t+=tx; t+=s;
		c= -((((t-s)-tx)-ty)-z);

	    /* return exp(y*log(x)) */
		t += exp__E(t,c); return(scalbn(one+t,m));
	     }
	/* end of if log(y*log(x)) > -60.0 */
	    
	    else
		/* exp(+- tiny) = 1 with inexact flag */
			{ln2hi+ln2lo; return(one);}
	    else if(copysign(one,y)*(n+invln2*t) <zero)
		/* exp(-(big#)) underflows to zero */
	        	return(scalbn(one,-5000)); 
	    else
	        /* exp(+(big#)) overflows to INF */
	    		return(scalbn(one, 5000)); 

}

static double
L1     =  6.6666666666667340202E-1    , /*Hex  2^ -1   *  1.5555555555592 */
L2     =  3.9999999999416702146E-1    , /*Hex  2^ -2   *  1.999999997FF24 */
L3     =  2.8571428742008753154E-1    , /*Hex  2^ -2   *  1.24924941E07B4 */
L4     =  2.2222198607186277597E-1    , /*Hex  2^ -3   *  1.C71C52150BEA6 */
L5     =  1.8183562745289935658E-1    , /*Hex  2^ -3   *  1.74663CC94342F */
L6     =  1.5314087275331442206E-1    , /*Hex  2^ -3   *  1.39A1EC014045B */
L7     =  1.4795612545334174692E-1    ; /*Hex  2^ -3   *  1.2F039F0085122 */

static double log__L(z)
double z;
{
    return(z*(L1+z*(L2+z*(L3+z*(L4+z*(L5+z*(L6+z*L7)))))));
}

static double 
p1     =  1.3887401997267371720E-2    , /*Hex  2^ -7   *  1.C70FF8B3CC2CF */
p2     =  3.3044019718331897649E-5    , /*Hex  2^-15   *  1.15317DF4526C4 */
q1     =  1.1110813732786649355E-1    , /*Hex  2^ -4   *  1.C719538248597 */
q2     =  9.9176615021572857300E-4    ; /*Hex  2^-10   *  1.03FC4CB8C98E8 */

static double exp__E(x,c)
double x,c;
{
	double static zero=0.0, one=1.0, half=1.0/2.0, small=1.0E-19;
	double copysign(),z,p,q,xp,xh,w;
	if(copysign(x,one)>small) {
           z = x*x  ;
	   p = z*( p1 +z* p2 );
           q = z*( q1 +z*  q2 );
           xp= x*p     ; 
	   xh= x*half  ;
           w = xh-(q-xp)  ;
	   p = p+p;
	   c += x*((xh*w-(q-(p+xp)))/(one-w)+c);
	   return(z*half+c);
	}
	/* end of |x| > small */

	else {
	    if(x!=zero) one+small;	/* raise the inexact flag */
	    return(copysign(zero,x));
	}
}
