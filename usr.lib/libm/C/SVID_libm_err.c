
#ifndef lint
static	char sccsid[] = "@(#)SVID_libm_err.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/errno.h>
#include <math.h>
#include "libm.h"

/* 
 * Report libm exception error according to System V Interface Definition
 * (SVID).
 * Error mapping:
 *	1 -- acos(|x|>1)
 *	2 -- asin(|x|>1)
 *	3 -- atan2(+-0,+-0)
 *	4 -- hypot overflow
 *	5 -- cosh overflow
 *	6 -- exp overflow
 *	7 -- exp underflow
 *	8 -- y0(0)
 *	9 -- y0(-ve)
 *	10-- y1(0)
 *	11-- y1(-ve)
 *	12-- yn(0)
 *	13-- yn(-ve)
 *	14-- lgamma(finite) overflow
 *	15-- lgamma(-integer)
 *	16-- log(0)
 *	17-- log(x<0)
 *	18-- log10(0)
 *	19-- log10(x<0)
 *	20-- pow(0.0,0.0)
 *	21-- pow(x,y) overflow
 *	22-- pow(x,y) underflow
 *	23-- pow(0,negative) 
 *	24-- neg**non-integral
 *	25-- sinh(finite) overflow
 *	26-- sqrt(negative)
 *
 * Constants: the following constants are defined in libm.h.
 *	NaN		--- Not a Number
 *	Inf		--- infinity
 *	PI_RZ		--- PI chopped to double precision format
 */

extern _swapTE(),_swapEX();
extern enum fp_direction_type _swapRD();



double SVID_libm_err(x,y,type) double x,y; int type;
{
	struct exception exc;
	char p[40];
	double t,w, setexception();
	exc.arg1 = x;
	exc.arg2 = y;
	switch(type) {
	    case 1:
		/* acos(|x|>1) */
		exc.type = DOMAIN;
		exc.name = "acos";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "acos: DOMAIN error\n", 19);
			errno = EDOM;
		}
		break;
	    case 2:
		/* asin(|x|>1) */
		exc.type = DOMAIN;
		exc.name = "asin";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "asin: DOMAIN error\n", 19);
			errno = EDOM;
		}
		break;
	    case 3:
		/* atan2(+-0,+-0) */
		exc.arg1 = y;
		exc.arg2 = x;
		exc.type = DOMAIN;
		exc.name = "atan2";
		exc.retval = (copysign(1.0,x)==1.0)? y: copysign(PI_RZ,y);
		if (!matherr(&exc)) {
			(void) write(2, "atan2: DOMAIN error\n", 20);
			errno = EDOM;
		}
		break;
	    case 4:
		/* hypot(finite,finite) overflow */
		exc.type = OVERFLOW;
		exc.name = "hypot";
		exc.retval = Inf;
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 5:
		/* cosh(finite) overflow */
		exc.type = OVERFLOW;
		exc.name = "cosh";
		exc.retval = Inf;
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 6:
		/* exp(finite) overflow */
		exc.type = OVERFLOW;
		exc.name = "exp";
		exc.retval = setexception(2,1.0);
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 7:
		/* exp(finite) underflow */
		exc.type = UNDERFLOW;
		exc.name = "exp";
		exc.retval = setexception(1,1.0);
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 8:
		/* y0(0) = -inf */
		exc.type = SING;
		exc.name = "y0";
		exc.retval = setexception(0,-1.0);
		if (!matherr(&exc)) {
			(void) write(2, "y0: SING error\n", 15);
			errno = EDOM;
		}
		break;
	    case 9:
		/* y0(x<0) = NaN */
		exc.type = DOMAIN;
		exc.name = "y0";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "y0: DOMAIN error\n", 17);
			errno = EDOM;
		}
		break;
	    case 10:
		/* y1(0) = -inf */
		exc.type = SING;
		exc.name = "y1";
		exc.retval = setexception(0,-1.0);
		if (!matherr(&exc)) {
			(void) write(2, "y1: SING error\n", 15);
			errno = EDOM;
		}
		break;
	    case 11:
		/* y1(x<0) = NaN */
		exc.type = DOMAIN;
		exc.name = "y1";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "y1: DOMAIN error\n", 17);
			errno = EDOM;
		}
		break;
	    case 12:
		/* yn(0) = -inf */
		exc.type = SING;
		exc.name = "yn";
		exc.retval = setexception(0,-1.0);
		if (!matherr(&exc)) {
			(void) write(2, "yn: SING error\n", 15);
			errno = EDOM;
		}
		break;
	    case 13:
		/* yn(x<0) = NaN */
		exc.type = DOMAIN;
		exc.name = "yn";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "yn: DOMAIN error\n", 17);
			errno = EDOM;
		}
		break;
	    case 14:
		/* lgamma(finite) overflow */
		exc.type = OVERFLOW;
		exc.name = "gamma";
		exc.retval = Inf;
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 15:
		/* lgamma(-integer) */
		exc.type = SING;
		exc.name = "gamma";
		exc.retval = setexception(0,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "gamma: SING error\n", 18);
			errno = EDOM;
		}
		break;
	    case 16:
		/* log(0) */
		exc.type = SING;
		exc.name = "log";
		exc.retval = setexception(0,-1.0);
		if (!matherr(&exc)) {
			(void) write(2, "log: SING error\n", 16);
			errno = EDOM;
		}
		break;
	    case 17:
		/* log(x<0) */
		exc.type = DOMAIN;
		exc.name = "log";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "log: DOMAIN error\n", 18);
			errno = EDOM;
		}
		break;
	    case 18:
		/* log10(0) */
		exc.type = SING;
		exc.name = "log10";
		exc.retval = setexception(0,-1.0);
		if (!matherr(&exc)) {
			(void) write(2, "log10: SING error\n", 18);
			errno = EDOM;
		}
		break;
	    case 19:
		/* log10(x<0) */
		exc.type = DOMAIN;
		exc.name = "log10";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "log10: DOMAIN error\n", 20);
			errno = EDOM;
		}
		break;
	    case 20:
		/* pow(0.0,0.0) */
		exc.type = DOMAIN;
		exc.name = "pow";
		exc.retval = 1.0;
		if (!matherr(&exc)) {
			(void) write(2, "pow(0,0): DOMAIN error\n", 23);
			errno = EDOM;
		}
		break;
	    case 21:
		/* pow(x,y) overflow */
		exc.type = OVERFLOW;
		exc.name = "pow";
		exc.retval = Inf;
		if(signbit(x)) {
		    t = rint(y);
		    if(t==y) {
			w = rint(0.5*y);
			if(t!=(w+w)) exc.retval = -exc.retval;/* y is odd */
		    }
		}
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 22:
		/* pow(x,y) underflow */
		exc.type = UNDERFLOW;
		exc.name = "pow";
		exc.retval = 0.0;
		if(signbit(x)) {
		    t = rint(y);
		    if(t==y) {
			w = rint(0.5*y);
			if(t!=(w+w)) exc.retval = -exc.retval;/* y is odd */
		    }
		}
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 23:
		/* 0**neg */
		exc.type = SING;
		exc.name = "pow";
		exc.retval = Inf;
		if(signbit(x)&&(t=rint(y))==y&&(t-2*floor(0.5*y))==1.0)
			exc.retval = -Inf;
		if (!matherr(&exc)) {
			(void) write(2, "pow(0,neg): SING error\n", 23);
			errno = EDOM;
		}
		break;
	    case 24:
		/* neg**non-integral */
		exc.type = DOMAIN;
		exc.name = "pow";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "neg**non-integral: DOMAIN error\n", 32);
			errno = EDOM;
		}
		break;
	    case 25:
		/* sinh(finite) overflow */
		exc.type = OVERFLOW;
		exc.name = "sinh";
		exc.retval = copysign(Inf,x);
		if (!matherr(&exc)) {
			errno = ERANGE;
		}
		break;
	    case 26:
		/* sqrt(x<0) */
		exc.type = DOMAIN;
		exc.name = "sqrt";
		exc.retval = setexception(3,1.0);
		if (!matherr(&exc)) {
			(void) write(2, "sqrt: DOMAIN error\n", 19);
			errno = EDOM;
		}
		break;
	}
	return exc.retval;
}


#define divbyz (1<<(int)fp_division)
#define unflow (1<<(int)fp_underflow)
#define ovflow (1<<(int)fp_overflow)
#define iexact (1<<(int)fp_inexact)
#define ivalid (1<<(int)fp_invalid)

static double setexception(n,x)
int n; double x;
{
    /* n = 0     --- divided by zero
	 = 1     --- underflow
	 = 2     --- overflow
	 = 3     --- invalid
     */
	int te,ex,k;
	enum fp_direction_type rd;
	te = _swapTE(0); if(te!=0) _swapTE(te);
	rd = _swapRD(fp_nearest); if(rd!=fp_nearest) _swapRD(rd);
	switch(n) {
	    case 0:			/* divided by zero */
		if((te&divbyz)==0) 
		    {ex= _swapEX(0); _swapEX(ex|divbyz); 
		     return copysign(Inf,x);}
		else return copysign(fmax,x)/0.0;
	    case 1:			/* underflow */
		if((te&unflow)==0&&rd==fp_nearest) 
		    {ex= _swapEX(0); _swapEX(ex|unflow|iexact); 
		     return copysign(0.0,x);}
		else return fmin*copysign(fmin,x);
	    case 2:			/* overflow  */
		if((te&ovflow)==0&&rd==fp_nearest) 
		    {ex= _swapEX(0); _swapEX(ex|ovflow|iexact); 
		     return copysign(Inf,x);}
		else return fmax*copysign(fmax,x);
	    case 3:			/* invalid */
		if((te&ivalid)==0)
		    {ex= _swapEX(0); _swapEX(ex|ivalid); 
		     return copysign(NaN,x);}
		else return Inf-Inf;
	}
}
