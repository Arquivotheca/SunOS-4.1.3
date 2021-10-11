
#ifndef lint
static	char sccsid[] = "@(#)r_tand.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  VMS Fortran compatibility function:
 *  tangent of an angle expressed in degrees.
 */

#include <math.h>

static float 	
pi_180	= 0.017453292519943295769,
r45	= 45.0,
r360	= 360.0,
r270	= 270.0,
r180	= 180.0,
r90	= 90.0;


static void r_sincosd(x,s,c)
float *x,*s,*c;
{
	FLOATFUNCTIONTYPE w;
	float y,t;
	long ix;
	if(!ir_finite_(x))  { *s = *c = *x - *x; return; }
	ix   = *((long *)x);
	*((long *)&t) = ix&0x7fffffff;		/* t = |*x| */
	w = r_fmod_(&t,&r360);
	y = *(float*)&w;
	switch ((int)(y/r45)) {
	    case 0:   t = pi_180*y;  	  r_sincos_(&t,s,c); break;
	    case 1:
	    case 2:   t = pi_180*(r90-y); r_sincos_(&t,c,s); break;
	    case 3:
	    case 4:   t = pi_180*(r180-y);r_sincos_(&t,s,c); 
				*c = -(*c); break;
	    case 5:   t = pi_180*(y-r270);r_sincos_(&t,c,s); 
				*s = -(*s); break;
	    case 6:   t = pi_180*(r270-y);r_sincos_(&t,c,s); 
				*c = -(*c); *s = -(*s); break;
	    default:  t = pi_180*(y-r360);r_sincos_(&t,s,c); break;
	}
	if(ix<0) *s = -(*s);
}

FLOATFUNCTIONTYPE r_tand(x)
float *x;
{
	float s,c;
	(void) r_sincosd(x,&s,&c);
	s = s/c; 
	RETURNFLOAT(s);
}

