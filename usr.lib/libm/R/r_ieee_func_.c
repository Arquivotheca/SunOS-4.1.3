
#ifndef lint
static	char sccsid[] = "@(#)r_ieee_func_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>
static double fmin = 1.0e-300, fmax = 1.0e300;
static float two25 = 33554432.0;

FLOATFUNCTIONTYPE r_copysign_(x,y)
float *x, *y;
{
	float w;
	*(int *)(&w) = ((*(int*)x)&0x7fffffff)|((*(int*)y)&0x80000000);
	RETURNFLOAT(w);
}

int ir_finite_(x)
float *x;
{
	return ((*(int*)x)&0x7f800000)!=0x7f800000;
}

int ir_ilogb_(x)
float *x;
{
	float fx = *x;
	long *px = (long *) &fx, k;
	k = px[0]&0x7fffffff;
	if(k<0x00800000) {
	    if (k==0) return 0x80000001;
	    else {
		fx *= two25;
		return ((px[0]&0x7f800000)>>23)-152;
	    }
	}
	else if (k<0x7f800000) return (k>>23)-127;
	else return 0x7fffffff;
}

int ir_isinf_(x)
float *x;
{
	return ((*(int*)x)&0x7fffffff)==0x7f800000;
}

int ir_isnan_(x)
float *x;
{
	register p = *(int*)x;
	return ((p&0x7f800000)==0x7f800000)&&((p&0x007fffff)!=0);
}

int ir_isnormal_(x)
float *x;
{
	register k = (*(int*)x)&0x7f800000;
	return (k!=0)&&(k!=0x7f800000);
}

int ir_issubnormal_(x)
float *x;
{
	register k = *(int*)x;
	return ((k&0x7f800000)==0)&&(k&0x7fffffff)!=0;
}

int ir_iszero_(x)
float *x;
{
	return  ((*(int*)x)&0x7fffffff)==0;
}

FLOATFUNCTIONTYPE r_nextafter_(x,y)
float *x, *y;
{
	float w;
	long *pw = (long *) &w;
	long *px = (long *) x;
	long *py = (long *) y;
	long ix,iy,iz;
	ix = px[0]; iy=py[0];
	if((ix&0x7f800000)==0x7f800000&&(ix&0x007fffff)!=0) 
		{w = *x+*y; RETURNFLOAT(w);}
	if((iy&0x7f800000)==0x7f800000&&(iy&0x007fffff)!=0) 
		{w = *y+*x; RETURNFLOAT(w);}
	if(ix==iy||(ix|iy)==0x80000000) {pw[0] = ix; RETURNFLOAT(w);}
	if((ix&0x7fffffff)==0) iz = 1|(iy&0x80000000); 
	else if(ix>0) {
	    if (ix > iy) iz = ix - 1;
	    else iz = ix + 1;
	} else {
	    if (iy<0&&ix<iy) iz = ix + 1;
	    else iz = ix - 1;
	}
	ix = iz&0x7f800000;
	if(ix==0x7f800000) dummy(fmax*fmax);	/* overflow  */
	else if(ix==0)     dummy(fmin*fmin);	/* underflow */
	pw[0] = iz;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE r_scalbn_(x,n)
float *x; int *n;
{
	float w;
	w = (float) scalbn((double)*x,*n);
	RETURNFLOAT(w);
}

int ir_signbit_(x)
float *x;
{
	return ((*(int*)x)>>31)&1;
}

static dummy(x)
double x;
{
	return 1;
}
