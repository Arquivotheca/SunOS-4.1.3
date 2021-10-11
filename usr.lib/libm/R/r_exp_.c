
#ifndef lint
static	char sccsid[] = "@(#)r_exp_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* r_exp_(x)
 * single precision exponential function
 * Rewrite by K.C. Ng based on Peter Tang's Table-driven paper, April, 1988.
 * Method :
 *	1. Argument Reduction: given the input x, find r and integer k such 
 *	   that
 *	                   x = (32m+j)*ln2/32 + R,  |r| <= 0.5*ln2/32 .  
 *	   R will be in double precision
 *
 *	2. exp(x) = (2^m * Sexp[j])*(P)
 *	   where
 *		P = 1+R*(1+R*(a1+R*a2))
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF)=  0;
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	Error is always less than 1 ulp (unit of last place). 
 *	Maximum error observed is less than 0.85 ulp.
 *
 * Constants:
 * Only the decimal values are given. We assume that the compiler will 
 * convert from decimal to binary accurately.
 */

#include <math.h>

static float
fhalf 		=  0.5,
mfhalf 		= -0.5,
invln2_32 	=  4.61662413084468283841e+01,
fzero 		=  0.0,
fone  		=  1.0,
p1		=  5.00000009512921380000e-01,
p2		=  1.66665188973472840000e-01,
p3		=  4.16662059758234840000e-02,
p4		=  8.36888310539362950000e-03,
p5		=  1.39504796300460640000e-03;

static double 
tiny 	= 1.0e-100,
huge	= 1.0e100,
one	= 1.0,
a1	= 5.00004053115844726562e-01,
a2	= 1.66667640209197998047e-01,
ln2_32	= 2.16608493924982901946e-02;		/* (ln2)/32 */

static double Sexp[] = {
1.00000000000000000000e+00, 1.02189714865411662714e+00,
1.04427378242741375480e+00, 1.06714040067682369717e+00,
1.09050773266525768967e+00, 1.11438674259589243221e+00,
1.13878863475669156458e+00, 1.16372485877757747552e+00,
1.18920711500272102690e+00, 1.21524735998046895524e+00,
1.24185781207348400201e+00, 1.26905095719173321989e+00,
1.29683955465100964055e+00, 1.32523664315974132322e+00,
1.35425554693689265129e+00, 1.38390988196383180053e+00,
1.41421356237309492343e+00, 1.44518080697704665027e+00,
1.47682614593949934623e+00, 1.50916442759342284141e+00,
1.54221082540794096616e+00, 1.57598084510788649659e+00,
1.61049033194925428347e+00, 1.64575547815396472373e+00,
1.68179283050742922612e+00, 1.71861929812247793414e+00,
1.75625216037329945351e+00, 1.79470907500310716820e+00,
1.83400808640934243066e+00, 1.87416763411029996256e+00,
1.91520656139714740007e+00, 1.95714412417540017941e+00,
};

FLOATFUNCTIONTYPE r_exp_(x)
float *x;
{
	double r,t,p;
	float fx,fr;
	long j,k,m,ix,iy;

	fx = *x; ix = *((long*)x); iy = ix&(~0x80000000);

    /* for |x| < ln2/2 */
	if(iy<0x3eb17218) {

	/* if |x| <= 2**-9 */
	    if(iy <= 0x3b000000) {	
		if (iy <= 0x38800000) fr = fone+fx; /* |x| <= 2**-14 */
		else fr = fone+fx*(fone+fhalf*fx);/* |x| <= 2**-9 */
	    }

	/* else if |x| < 2**-6 */
	    else if(iy<0x3c800000) fr = fone+fx*(fone+fx*(p1+fx*p2));

	/* else if |x| < ln2/2 */
	    else {
		fr = fx + (fx*fx)*(p1+fx*(p2+fx*(p3+fx*(p4+fx*p5))));
		fr += fone;
	    }
	    RETURNFLOAT(fr);
	}

    /* r_exp_(x) will overflow or underflow or x is NaN or Inf */
	if(iy > 0x42CFF1B4 || ix >= 0x42B17218) {
	    if(iy >= 0x7f800000) {
		if(iy>0x7f800000) fr = fx+fx;		/*  NaN */
		else if(ix == 0x7f800000) fr = fx;	/* +inf */
		else fr = fzero;			/* -inf */
	    } else {
		if(ix<0) fr = (float) tiny;	/* create underflow */
		else	 fr = (float) huge;	/* create overflow */
	    }
	    RETURNFLOAT(fr);
	}


    /* argument reduction  */
	k  = invln2_32*fx+((ix>0)?fhalf:mfhalf);
	r  = (double)fx - (double)k*ln2_32;
	j  = k&0x1f; m = k>>5;
	p  = one+r*(one+r*(a1+r*a2));
	t  = Sexp[j];
	if (*(long *)&one == 0)		/* Sun 386i	*/
	  *(1+(long*)&t) += (m<<20);	/* form 2^m * Sexp[j] */
	else					/* not Sun 386i	*/
	  *((long*)&t) += (m<<20);	/* form 2^m * Sexp[j] */
	fr = t*p;
	RETURNFLOAT(fr);
}

