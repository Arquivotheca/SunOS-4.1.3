
#ifndef lint
static	char sccsid[] = "@(#)d_sind.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  VMS Fortran compatibility function:
 *  sine of an angle expressed in degrees.
 */

#include <math.h>
static double pi_180 = 0.017453292519943295769;

double d_sind(x)
double *x;
{
	double y,z;
	int i;
	if(!finite(*x))  return *x-(*x);
	z = fmod(*x,360.0);
	y = fabs(z);
	i = signbit(z);
	switch ((int)(y/45.0)) {
	    case 0:   y =  sin(y*pi_180); break;
	    case 1:   
	    case 2:   y =  cos((90.0-y)*pi_180); break;
	    case 3:  
	    case 4:   y =  sin((180.0-y)*pi_180); break;
	    case 5:
	    case 6:   y = -cos((y-270.0)*pi_180); break;
	    default:  y =  sin((y-360.0)*pi_180); break;
	    }
	return (i==0)? y: -y;
}
