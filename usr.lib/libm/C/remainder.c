
#ifndef lint
static	char sccsid[] = "@(#)remainder.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* remainder(x,p)
 * Code originated from 4.3bsd.
 * Modified by K.C. Ng for SUN 4.0 libm.
 * Return :                  
 * 	returns  x REM p  =  x - [x/p]*p as if in infinite precise arithmetic,
 *	where [x/p] is the (inifinite bit) integer nearest x/p (in half way 
 *	case choose the even one).
 * Method : 
 *	Based on fmod() return x-[x/p]chopped*p exactly.
 */

#include <math.h>
static double zero = 0.0;

double remainder(x,p)
double x,p;
{
double	one	= 1.0;
static	long	hfmaxx[2], twom1021x[2];

	double hp;
	long sx;

	if ((* (int *) &one) != 0) {
	  hfmaxx[0] = 0x7fdfffff; hfmaxx[1] = 0xffffffff;
	  twom1021x[0] = 0x00200000; twom1021x[1] = 0x0;
	}						/* not a i386	*/
	else {
	  hfmaxx[0] = 0xffffffff; hfmaxx[1] = 0x7fdfffff;
	  twom1021x[0] = 0x0; twom1021x[1] = 0x00200000;
	}						/* is a i386	*/

#define	hfmax (*(double *)hfmaxx)
#define	twom1021 (*(double *)twom1021x)

	if(p!=p) return x+p;
	if(!finite(x)) return x-x;
	p  = fabs(p);
	if(p<=hfmax) x = fmod(x,p+p);
	sx = signbit(x);
	x  = fabs(x);
	if (p<twom1021) {
	    if(x+x>p) {
		if(x==p) x=zero; else x-=p;	/* avoid x-x=-0 in RM mode */
		if(x+x>=p) x -= p;
	    }
	} else {
	    hp = 0.5*p;
	    if(x>hp) {
		if(x==p) x=zero; else x-=p;	/* avoid x-x=-0 in RM mode */
		if(x>=hp) x -= p;
	    }
	}
	return (sx==0)? x: -x;
}

double drem(x,p)
double x,p;
{
	return(remainder(x,p));
}
