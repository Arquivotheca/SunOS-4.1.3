
#ifndef lint
static	char sccsid[] = "@(#)d_cosd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 *  VMS Fortran compatibility function:
 *  cosine of an angle expressed in degrees.
 */

#include <math.h>
static double pi_180 = 0.017453292519943295769;

double d_cosd(x)
double *x;
{
	double y;
	int i;
	if(!finite(*x))  return *x-(*x);
	y = fabs(fmod(*x,360.0));
	switch ((int)(y/45.0)) {
	    case 0:   return  cos(y*pi_180);
	    case 1:   
	    case 2:   return  sin((90.0-y)*pi_180);
	    case 3:  
	    case 4:   return -cos((180.0-y)*pi_180);
	    case 5:   return  sin((y-270.0)*pi_180);
	    case 6:   return -sin((270.0-y)*pi_180);
	    default:  return  cos((y-360.0)*pi_180);
	    }
}
