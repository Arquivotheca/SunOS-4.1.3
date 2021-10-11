
#ifndef lint
static	char sccsid[] = "@(#)ieee_test.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* IEEE functions for satisfying IEEE 754 test
 *	significand()
 *	logb()
 *	scalb()
 */

#include "libm.h"
#include <math.h>

static	double	one	= 1.0;

double logb(x)
double x;
{
int	n0;

	long *px = (long *) &x,k;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386 */

	if ((px[1-n0]|(px[n0]&0x7fffffff))==0) return -1.0/fabs(x);
	k = ((px[n0]&0x7ff00000)>>20);
	if(k==0x7ff) return x*x;
	else 
	return (double) ((k==0)? -1022: k-1023); /* IEEE 754 logb */
}

double scalb(x,fn)
double x, fn;
{
int	n0;

	long *px = (long *) &x, *py = (long*)&fn, k,n;
	double scalbn();

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386 */

	if ((py[n0]&0x7ff00000)==0x7ff00000) 
	    /* fn is inf or NaN */
	    if((py[n0]&0x80000000)!=0) return x/(-fn); else return x*fn;
	if (rint(fn)!=fn) return (fn-fn)/(fn-fn); else
	if (fn >  5000.0) return x*fmax*fmax*fmax; else
	if (fn < -5000.0) return x*fmin*fmin*fmin;
	n = (int) fn;
	return scalbn(x,n);
}

double significand(x)
double x;
{
	return scalb(x,(double) -ilogb(x));
}
