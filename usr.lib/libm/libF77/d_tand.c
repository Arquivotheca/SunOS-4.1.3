
#ifndef lint
static	char sccsid[] = "@(#)d_tand.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  VMS Fortran compatibility function:
 *  tangent of an angle expressed in degrees.
 */

#include <math.h>
static double pi_180 = 0.017453292519943295769;

static void d_sincosd(x,s,c)
double *x,*s,*c;
{
	double y,z;
	int i;
	if(!finite(*x))  { *s = *c =  *x-(*x); return ; }
	z = fmod(*x,360.0);
	y = fabs(z);
	i = signbit(z); 
	switch ((int)(y/45.0)) {
	    case 0:   sincos(y*pi_180,s,c); break;
	    case 1:   
	    case 2:   sincos((90.0-y)*pi_180,c,s); break;
	    case 3:  
	    case 4:   sincos((180.0-y)*pi_180,s,c); *c = -(*c); break;
	    case 5:   sincos((y-270.0)*pi_180,c,s); *s = -(*s); break;
	    case 6:   sincos((270.0-y)*pi_180,c,s); *s = -(*s); 
						    *c = -(*c); break;
	    default:  sincos((y-360.0)*pi_180,s,c); break;
	    }
	if(i!=0) *s = -(*s);
}

double d_tand(x)
double *x;
{
	double s,c;
	(void) d_sincosd(x,&s,&c);
	return s/c;
}

