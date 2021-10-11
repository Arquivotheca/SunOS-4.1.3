
#ifndef lint
static	char sccsid[] = "@(#)r_sind.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  VMS Fortran compatibility function:
 *  sine of an angle expressed in degrees.
 */

#include <math.h>

static float 	
pi_180	= 0.017453292519943295769,
r45	= 45.0,
r360	= 360.0,
r270	= 270.0,
r180	= 180.0,
r90	= 90.0;

FLOATFUNCTIONTYPE r_sind(x)
float *x;
{
	FLOATFUNCTIONTYPE w;
	float y,t;
	long ix;
	if(!ir_finite_(x))  {t = *x - *x; RETURNFLOAT(t);}
	ix   = *((long *)x);
	*((long *)&t) = ix&0x7fffffff;		/* t = |*x| */
	w = r_fmod_(&t,&r360);
	y = *(float*)&w;
	switch ((int)(y/r45)) {
	    case 0:   t = pi_180*y;  	   w =  r_sin_(&t); break;
	    case 1: 
	    case 2:   t = pi_180*(r90-y);  w =  r_cos_(&t); break;
	    case 3:
	    case 4:   t = pi_180*(r180-y); w =  r_sin_(&t); break;
	    case 5:
	    case 6:   t = pi_180*(y-r270); w =  r_cos_(&t); ix = -ix; break;
	    default:  t = pi_180*(y-r360); w =  r_sin_(&t); break;
	    }
	if(ix<0) *(int*)&w ^= 0x80000000;	/* w = -w */
	return w;
}
