
#ifndef lint
static	char sccsid[] = "@(#)ieee_func.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* IEEE functions
 *	copysign()
 *	finite()
 *	ilogb()
 *	isinf()
 *	isnan()
 *	isnormal()
 *	issubnormal()
 *	iszero()
 *	nextafter()
 *	scalbn()
 *	signbit()
 */

#include <math.h>
#include "libm.h"

#define divbyz (1<<(int)fp_division)
#define unflow (1<<(int)fp_underflow)
#define ovflow (1<<(int)fp_overflow)
#define iexact (1<<(int)fp_inexact)
#define ivalid (1<<(int)fp_invalid)

static double setexception();
extern _swapTE(),_swapEX();
extern enum fp_direction_type _swapRD();

static	double	one	= 1.0;

double copysign(x,y)
double x,y;
{
int	n0;

	long *px = (long *) &x;
	long *py = (long *) &y;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	px[n0] = (px[n0]&0x7fffffff)|(py[n0]&0x80000000);
	return x;
}

int finite(x)
double x;
{
int	n0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	return ((px[n0]&0x7ff00000)!=0x7ff00000);
}

int ilogb(x)
double x;
{
int	n0;

	long *px = (long *) &x, k;

	if ((* (int *) &one) != 0) n0 = 0;	/* not a i386 */
	else n0 = 1;				/* is a i386  */

	k = px[n0]&0x7ff00000;
	if(k==0) {
		if ((px[1-n0]|(px[n0]&0x7fffffff))==0) return 0x80000001; 
		else {
			x *= two52; 
			return ((px[n0]&0x7ff00000)>>20)-1075;
		}
	}
	else if (k!=0x7ff00000) return (k>>20)-1023;
	else return 0x7fffffff;
}

int isinf(x)
double x;
{
int	n0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	if((px[n0]&0x7fffffff)!=0x7ff00000) return FALSE;
	else return px[1-n0] == 0;
}

int isnan(x)
double x;
{
int	n0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	if((px[n0]&0x7ff00000)!=0x7ff00000) return FALSE;
	else return ((px[n0]&0x000fffff)|px[1-n0]) != 0;
}

int isnormal(x)
double x;
{
int	n0;

	long *px = (long *) &x, n;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	n = px[n0]&0x7ff00000;
	return (n!=0&&n!=0x7ff00000);
}

int issubnormal(x)
double x;
{
int	n0;

	long *px = (long *) &x, n;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	n = px[n0]&0x7ff00000;
	return (n==0&&(px[1-n0]|(px[n0]&0x7fffffff))!=0) ;
}

int iszero(x)
double x;
{
int	n0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	return  ((px[1-n0]|(px[n0]&0x7fffffff))==0) ;
}

double nextafter(x,y)
double x,y;
{
int	n0;

	long *px = (long *) &x;
	long *py = (long *) &y;
	long k;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	if(x==y) return x;
	if(x!=x||y!=y) return x+y;
	if(x==0.0) {
		px[n0] = py[n0]&0x80000000;
		px[1-n0] = 1;
	} else if(x>0.0) {
		if (x > y) {
			px[1-n0] -= 1;
			if(px[1-n0]==0xffffffff) px[n0] -= 1;
		} else {
			px[1-n0] += 1;
			if(px[1-n0]==0) px[n0] += 1;
		}
	} else {
		if (x < y) {
			px[1-n0] -= 1;
			if(px[1-n0]==0xffffffff) px[n0] -= 1;
		} else {
			px[1-n0] += 1;
			if(px[1-n0]==0) px[n0] += 1;
		}
	}
	k = px[n0]&0x7ff00000;
	if(k==0x7ff00000) setexception(2,x);	/* overflow  */
	else if(k==0)     setexception(1,x); 	/* underflow */
	return x;
}

double scalbn(x,n)
double x; int n;
{
int	n0;

	long *px = (long *) &x, k;
	double twom54=twom52*0.25;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	k = (px[n0]&0x7ff00000)>>20;
	if (k==0x7ff) return x+x;
	if ((px[1-n0]|(px[n0]&0x7fffffff))==0) return x;
	if (k==0) {x *= two52; k = ((px[n0]&0x7ff00000)>>20) - 52;}
	k = k+n;
	if (n >  5000) return setexception(2,x);
	if (n < -5000) return setexception(1,x);
	if (k >  0x7fe ) return setexception(2,x);
	if (k <= -54   ) return setexception(1,x);
	if (k > 0) {
		px[n0] = (px[n0]&0x800fffff)|(k<<20);
		return x;
	}
	k += 54;
	px[n0] = (px[n0]&0x800fffff)|(k<<20);
	return x*twom54;
}

int signbit(x)
double x;
{
int	n0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) n0 = 0;		/* not a i386 */
	else n0 = 1;					/* is a i386  */

	return  (px[n0]>>31)&1;
}

static double setexception(n,x)
int n; double x;
{
    /* n = 1     --- underflow
	 = 2     --- overflow
     */
	int te,ex,k;
	enum fp_direction_type rd;
	te = _swapTE(0); if(te!=0) _swapTE(te);
	rd = _swapRD(fp_nearest); if(rd!=fp_nearest) _swapRD(rd);
	switch(n) {
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
	}
}
