
#ifndef lint
static	char sccsid[] = "@(#)hypot.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* hypot(x,y)
 * by K.C. Ng for SUN 4.0 libm.
 * Method :                  
 *	If z=x*x+y*y has error less than sqrt(2)/2 ulp than sqrt(z) has
 *	error less than 1 ulp.
 *	So, compute sqrt(x*x+y*y) with some care as follows:
 *	Assume x>y>0;
 *	1. save and set rounding to round-to-nearest
 *	2. if x > 2y  use
 *		x1*x1+(y*y+(x2*(x+x2))) for x*x+y*y
 *	where x1 = x with lower 32 bits cleared, x2 = x-x1; else
 *	3. if x <= 2y use
 *		t1*y1+((x-y)*(x-y)+(t1*y2+t2*y))
 *	where t1 = 2x with lower 32 bits cleared, t2 = 2x-t1, y1= y with
 *	lower 32 bits chopped, y2 = y-y1.
 *		
 *	NOTE: DO NOT remove parenthsis!
 *
 * Special cases:
 *	hypot(x,y) is INF if x or y is +INF or -INF; else
 *	hypot(x,y) is NAN if x or y is NAN.
 *
 * Accuracy:
 * 	hypot(x,y) returns sqrt(x^2+y^2) with error less than 1 ulps (units
 *	in the last place) 
 */

#include <math.h>
#include "libm.h"
extern enum fp_direction_type _swapRD();

static double 
two54  = 134217728.0 * 134217728.0,
twon54 = 1.0/(134217728.0 * 134217728.0),
two1022 = 4.49423283715578976932e+307,
twon1022= 2.22507385850720138309e-308;


double hypot(x,y)
double x, y;
{
double	one	= 1.0;
register	n0;

	double ox=x, oy=y;	/* keep copy of x and y for SVID */
	double t1,t2,y1,y2,w;
	long *px = (long*)&x,*py=(long*)&y;
	long *pt1 = (long*)&t1,*py1=(long*)&y1;
	enum fp_direction_type rd;
	long i,j,k,nx,ny,nz;

	if ((* (int *) &one) != 0) n0=0;		/* not a i386 */
	else n0=1;					/* is a i386  */

	px[n0] &= 0x7fffffff;	/* clear sign bit of x and y */
	py[n0] &= 0x7fffffff;
	k      = 0x7ff00000;
	nx     = px[n0]&k;	/* exponent of x and y */
	ny     = py[n0]&k;
	if(ny > nx) {w=x;x=y;y=w;nz=ny;ny=nx;nx=nz;}	
	if((nx-ny)>0x3c00000) {x += y; goto exit;} /* x/y > 2**60 */
	if(nx < 0x5f300000 && ny > 0x23d00000) {	/* medium x,y */
	    			/* save and set RD to Rounding to nearest */
	    rd = _swapRD(fp_nearest);
	    w = x-y;
	    if (w>y) {
		pt1[n0]=px[n0]; pt1[1-n0]=0;
		t2 = x-t1;
		x  = sqrt(t1*t1-(y*(-y)-t2*(x+t1)));
	    } else {
		x  = x+x;
		py1[n0] = py[n0];  py1[1-n0]=0;
		y2 = y - y1;
		pt1[n0] = px[n0];  pt1[1-n0]=0;
		t2 = x - t1;
		x  = sqrt(t1*y1-(w*(-w)-(t2*y1+y2*x)));
	    }
	    if(rd!=fp_nearest) _swapRD(rd);	/* restore rounding mode */
	    goto exit;
	} else {
	    if(nx==k||ny==k) { /* x or y is INF or NaN */
		if(isinf(x)) return x; else 
		if(isinf(y)) return y; else return x+y;
	    }
	    if (ny==0) {
	        if (y==0.0||x==0.0) return x+y;
		x *= two1022;
		y *= two1022;
		x  = twon1022*hypot(x,y);
		goto exit;
	    }
	    j = nx-0x3ff00000;
	    px[n0] -= j;
	    py[n0] -= j;
	    pt1[n0] = nx; pt1[1-n0]=0;
	    x = t1*hypot(x,y);
	}
exit:
	if (px[n0]==k) x = SVID_libm_err(ox,oy,4);
	return x;
}
