
#ifndef lint
static	char sccsid[] = "@(#)r_hypot_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_hypot_(x,y)
float *x, *y;
{
	double dx,dy; float w; int ix,iy;
	ix = (*(long*)x)&0x7fffffff;
	iy = (*(long*)y)&0x7fffffff;
	if (ix>=0x7f800000) {
		if(ix==0x7f800000)      *(int*)&w = ix; /* w = |x| = inf */
		else if(iy==0x7f800000) *(int*)&w = iy; /* w = |y| = inf */
		else w = *x + *y; 
	} else if(iy>=0x7f800000) {
		if(iy == 0x7f800000)    *(int*)&w = iy; /* w = |y| = inf */
		else w = *x + *y;
	} else if(ix ==0) 		*(int*)&w = iy; /* w = |y|  */
	else if(iy ==0) 		*(int*)&w = ix; /* w = |x|  */
	else {
	    dx=(double)*x; dy=(double)*y;
	    w = (float) sqrt(dx*dx+dy*dy);
	}
	RETURNFLOAT(w);
}
