
#ifndef lint
static	char sccsid[] = "@(#)erf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/* double erf,erfc(double x)
 * K.C. Ng, March, 1989.
 *			     x
 *		      2      |\
 *     erf(x)  =  ---------  | exp(-t*t)dt
 *	 	   sqrt(pi) \| 
 *			     0
 *
 *     erfc(x) =  1-erf(x)
 *
 * Part of the algorithm is based on cody's erf,erfc program.
 */

extern double exp();
static double
zero	    = 0.0,
tiny	    = 1e-300,
one	    = 1.0,
invsqrtpi_2 = 1.1283791670955125738961589031,
invsqrtpi   = 5.6418958354775628695e-1,
/*
 * Coefficients for approximation to  erf  in first interval
 */
a0 =	3.20937758913846947e03,
a1 =	3.77485237685302021e02,
a2 =	1.13864154151050156e02,
a3 =	3.16112374387056560e00,
a4 =	1.85777706184603153e-1,
b0 =	2.84423683343917062e03,
b1 =	1.28261652607737228e03,
b2 =	2.44024637934444173e02,
b3 = 	2.36012909523441209e01,
/*
 * Coefficients for approximation to  erfc  in second interval
 */
c7 =	5.64188496988670089e-1,
c6 =	8.88314979438837594e0,
c5 =	6.61191906371416295e01,
c4 =	2.98635138197400131e02,
c3 =	8.81952221241769090e02,
c2 =	1.71204761263407058e03,
c1 =	2.05107837782607147e03,
c0 =	1.23033935479799725e03,
c8 =	2.15311535474403846e-8,
d7 =	1.57449261107098347e01,
d6 =	1.17693950891312499e02,
d5 =	5.37181101862009858e02,
d4 =	1.62138957456669019e03,
d3 =	3.29079923573345963e03,
d2 =	4.36261909014324716e03,
d1 =	3.43936767414372164e03,
d0 =	1.23033935480374942e03,
/*
 * Coefficients for approximation to  erfc  in third interval
 */
p4 =	3.05326634961232344e-1,
p3 =	3.60344899949804439e-1,
p2 =	1.25781726111229246e-1,
p1 =	1.60837851487422766e-2,
p0 =	6.58749161529837803e-4,
p5 =	1.63153871373020978e-2,
q4 =	2.56852019228982242e00,
q3 =	1.87295284992346047e00,
q2 =	5.27905102951428412e-1,
q1 =	6.05183413124413191e-2,
q0 =	2.33520497626869185e-3;


#include <math.h>
double erf(x) 
double x;
{
	double erfc(),s,y,t;
	long *px = (long*)&x;
	int ix,iy,i0;

	if(!finite(x)) {
	    if(x!=x) return x+x; 	/* NaN */
	    return copysign(1.0,x);	/* Inf */
	}

	y = fabs(x);
	if(y <= 0.46875){
	    t = one+y;
	    if(one==t) return invsqrtpi_2*x;
	    s = y*y;
	    y = (a0+s*(a1+s*(a2+s*(a3+s*a4))))/(b0+s*(b1+s*(b2+s*(b3+s))));
	    return x*y;
	}

	if(y>10.0) t=tiny; else t = erfc(y);
	return (x>zero)? one-t:t-one;
}

double erfc(x) 
double x;
{
	double erf(),y,s,t,z,r;
	int i;

	if(!finite(x)) {
	    if(x!=x) return x+x; 	/* NaN */
	    return (x>zero)? zero:2.0;	/* +-Inf */
	}
	y = fabs(x);
	if(y <= 0.46875) return one-erf(x);
	if(y <= 4.0) {
	    z  = d0+y*(d1+y*(d2+y*(d3+y*(d4+y*(d5+y*(d6+y*(d7+y)))))));
	    t  = c0+y*(c1+y*(c2+y*(c3+y*(c4+y*(c5+y*(c6+y*(c7+y*c8)))))));
	    s  = y*y;
	    r  = t/z;
	} else {	/* erfc for |x| > 4.0 */
	    if(x<zero&&y>10.0) return 2.0-tiny;
	    if(y>28.0) return tiny*tiny;	/* underflow */
	    s  = one/(y*y);
	    z  = q0+s*(q1+s*(q2+s*(q3+s*(q4+s))));
	    t  = p0+s*(p1+s*(p2+s*(p3+s*(p4+s*p5))));
	    r  = t/z;
	    r  = (invsqrtpi-s*r)/y;
	}
	i  = (int)(y*16.0); t=(double)i*0.0625;
	r *= exp(-t*t)*exp(-(y-t)*(y+t));
	return (x>zero)? r: 2.0-r;
}
