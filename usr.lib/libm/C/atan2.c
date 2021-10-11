
#ifndef lint
static	char sccsid[] = "@(#)atan2.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* atan2(y,x)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *	2. Reduce x to positive by (if x and y are unexceptional): 
 *		ARG (x+iy) = arctan(y/x)   	   ... if x > 0,
 *		ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *	ATAN2((anything), NaN ) is NaN;
 *	ATAN2(NAN , (anything) ) is NaN;
 *	ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *	ATAN2(+-0, -(anything but NaN)) is +-PI ;
 *	ATAN2(+-(anything but 0 and NaN), 0) is +-PI/2;
 *	ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *	ATAN2(+-(anything but INF and NaN), -INF) is +-PI;
 *	ATAN2(+-INF,+INF ) is +-PI/4 ;
 *	ATAN2(+-INF,-INF ) is +-3PI/4;
 *	ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-PI/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include "libm.h"

static double 
PIo4   =  7.8539816339744827900E-1    , /*Hex  2^ -1   *  1.921FB54442D18 */
PIo2   =  1.5707963267948965580E0     , /*Hex  2^  0   *  1.921FB54442D18 */
PI     =  3.1415926535897931160E0     ; /*Hex  2^  1   *  1.921FB54442D18 */

static double PI_lo[] = {
1.2246467991473531772E-16,	/* lo part of pi */
1.2246063538223772582E-16,	/* lo part of 66 bits pi */
0.0,				/* lo part of 53 bits pi */
};

static double dummy(x)
double x ;
{
return x ;
}

double atan2(y,x)
double  y,x;
{  
	static double zero=0;
	double t,z;
	int k,m,signy,signx;

	if(x!=x||y!=y) return x+y; 	/* return NaN if x or y is NAN */
	signy = signbit(y) ;  
	signx = signbit(x) ;  
	if(x==1.0) return atan(y);
	m = signy+signx+signx;

    /* when y = 0 */
	if(y==zero) 
	    if(x==zero) return SVID_libm_err(x,y,3); 
	    else 
		switch(m) {
		    case 0: return y  ;	/* atan(+0,+anything) */
		    case 1: return y  ;	/* atan(-0,+anything) */
		    case 2: return  PI;	/* atan(+0,-anything) */
		    case 3: return -PI;	/* atan(-0,-anything) */
		}

    /* when x = 0 */
	if(x==zero) return (signy==1)?  -PIo2: PIo2;
	    
    /* when x is INF */
	if(!finite(x))
	    if(!finite(y)) {
		switch(m) {
		    case 0: return  PIo4  ;	/* atan(+INF,+INF) */
		    case 1: return -PIo4  ;	/* atan(-INF,+INF) */
		    case 2: return  3*PIo4;	/* atan(+INF,-INF) */
		    case 3: return -3*PIo4;	/* atan(-INF,-INF) */
		}
	    } else {
		switch(m) {
		    case 0: return  zero  ;	/* atan(+...,+INF) */
		    case 1: return -zero  ;	/* atan(-...,+INF) */
		    case 2: return  PI	  ;	/* atan(+...,-INF) */
		    case 3: return -PI	  ;	/* atan(-...,-INF) */
		}
	    }

    /* when y is INF */
	if(!finite(y)) return (signy==1)? -PIo2: PIo2;

    /* compute y/x */
	x=fabs(x); 
	y=fabs(y); 
	t=PI_lo[(int)fp_pi];
	k = (ilogb(y)-ilogb(x));
	if(k > 60) z=PIo2+0.5*t; else if(m>1&&k<-60) z=0.0; else z=atan(y/x);
	switch (m) {
	    case 0: return       z  ;	/* atan(+,+) */
	    case 1: return      -dummy(z)  ;	/* atan(-,+) */
	    case 2: return  PI-(z-t);	/* atan(+,-) */
	    case 3: return  (z-t)-PI;	/* atan(-,-) */
	}
}
