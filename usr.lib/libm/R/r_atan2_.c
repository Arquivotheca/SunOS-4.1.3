
#ifndef lint
static	char sccsid[] = "@(#)r_atan2_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>


static double 
PIo4   =  7.8539816339744827900E-1    , /*Hex  2^ -1   *  1.921FB54442D18 */
PIo2   =  1.5707963267948965580E0     , /*Hex  2^  0   *  1.921FB54442D18 */
PI     =  3.1415926535897931160E0     , /*Hex  2^  1   *  1.921FB54442D18 */
tiny   =  1.0e-30;

static float zero=0.0,one=1.0;

FLOATFUNCTIONTYPE r_atan2_(y,x)
float *y, *x;
{  
	double z;
	float fx,fy,fz;
	long *pfz = (long *)&fz;
	int k,m,signy,signx;
	fx = *x; fy = *y;

	if(fx!=fx||fy!=fy) {	/* return NaN if fx or fy is NAN */
	    fz = fx + fy; 
	    RETURNFLOAT(fz);
	}
	signy = ir_signbit_(y);  
	signx = ir_signbit_(x);  
	if(fx==one) return r_atan_(y);
	m = signy+signx+signx;

    /* when fy = 0 */
	if(fy==zero) {
	    switch(m) {
		case 0:
		case 1: fz=  fy;break;	/* atan(+-0,+anything) = +-0 */
		case 2: fz=  PI;break;	/* atan(+0,-anything)  = +PI */
		case 3: fz= -PI;break;	/* atan(-0,-anything)  = -PI */
	    }
	    RETURNFLOAT(fz);
	}

    /* when fx = 0 */
	if(fx==zero) {
	    fz = (signy==1)?  -PIo2: PIo2;
	    RETURNFLOAT(fz);
	}
	    
    /* when x is INF */
	if(!ir_finite_(x))
	    if(!ir_finite_(y)) {
		switch(m) {
		    case 0: fz=  PIo4  ;break;	/* atan(+INF,+INF) */
		    case 1: fz= -PIo4  ;break;	/* atan(-INF,+INF) */
		    case 2: fz=  3*PIo4;break;	/* atan(+INF,-INF) */
		    case 3: fz= -3*PIo4;break;	/* atan(-INF,-INF) */
		}
		RETURNFLOAT(fz);
	    } else {
		switch(m) {
		    case 0: fz=  zero;break;	/* atan(+...,+INF) */
		    case 1: fz= -zero;break;	/* atan(-...,+INF) */
		    case 2: fz=  PI  ;break;	/* atan(+...,-INF) */
		    case 3: fz= -PI  ;break;	/* atan(-...,-INF) */
		}
		RETURNFLOAT(fz);
	    }

    /* when y is INF */
	if(!ir_finite_(y)) {
	    fz = (signy==1)? -PIo2: PIo2;
	    RETURNFLOAT(fz);
	}

    /* compute y/x */
	k = ir_ilogb_(y)-ir_ilogb_(x);
	if(k > 30) z=PIo2; 
	else if(m>1&& k< -30) z=tiny;
	else {
	    fz      = fy/fx;
	    pfz[0] &= 0x7fffffff;
	    ASSIGNFLOAT(fz,r_atan_(&fz));
	    z       = fz;
	}
	switch (m) {
	    case 0: fz =   z  ;break;	/* atan(+,+) */
	    case 1: fz =   z; fz = -fz ;break;	/* atan(-,+) */
	    case 2: fz = PI-z ;break;	/* atan(+,-) */
	    case 3: fz = z-PI ;break;	/* atan(-,-) */
	}
	RETURNFLOAT(fz);
}
