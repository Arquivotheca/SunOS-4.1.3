
#ifndef lint
static	char sccsid[] = "@(#)rint.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * aint(x)	return x chopped to integral value
 * anint(x)	return sign(x)*(|x|+0.5) chopped to integral value
 * ceil(x)	return the biggest integral value below x
 * floor(x)	return the least integral value above x
 * irint(x)	return rint(x) in integer format
 * nint(x)	return anint(x) in integer format
 * rint(x)	return x rounded to integral according to the rounding direction
 *
 * NOTE: aint(x), anint(x), ceil(x), floor(x), and rint(x) return result 
 * with the same sign as x's,  including 0.0.
 */

#include <math.h>
#include "libm.h"

static double zero = 0.0;

double aint(x)
double x ;
{
	double t,w;
	w = fabs(x);
	t = rint(w);
	if (x!=x||t<=w) return copysign(t,x);	/* NaN or already aint(|x|) */
	else return copysign(t-1.0,x); 		/* |t|>|x| case */
}

double anint(x)
double x ;
{
	double t,w,z;
	w = fabs(x);
	t = rint(w);
	if (x!=x||t==w) return copysign(t,x);
	z = t - w;
	if(z >   0.5)  t -= 1.0; else
	if(z <= -0.5)  t += 1.0; 
	return copysign(t,x);
}


double ceil(x)
double x ;
{
	double t;
	t = rint(x);
	if (x!=x||t>=x) return t;	/* NaN or already ceil(x) */
	else return copysign(t+1.0,x); 	/* t < x case: return t+1  */
}

double floor(x)
double x ;
{
	double t;
	t = rint(x);
	if (x!=x||t<=x) return t;	/* NaN or already floor(x) */
	else return copysign(t-1.0,x); 	/* x < t case: return t-1  */
}

int irint(x)
double x;
{
	return (int) rint(x);
}
	
int nint(x)
double x;
{
	return (int) anint(x);
}
	

double rint(x)
double x;
{
	enum fp_precision_type _swapRP(),rp;
	double t,w;
	char *out;
	t = fabs(x);
	if (x!=x||fabs(x)>=two52) return x-zero;/* NaN or already an integer */
	t = copysign(two52,x);
	rp = _swapRP(fp_double);/* make sure precision is double */	
	w = x+t;		/* x+sign(x)*2**52 rounded to integer */
	_swapRP(rp);		/* restore precision mode */
	if(w==t) return copysign(zero,x);	/* x rounded to zero */
	else return w - t;
}
