#ifndef lint
 static	char sccsid[] = "@(#)d_atn2d.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 *  VMS Fortran compatibility function:
 *  two-argument arc tangent in degrees.
 */

#include <math.h>
static double pitod = 57.2957795130823208767981548141; /* 180/pi */

double d_atn2d(y,x)
double *y, *x;
{
	return pitod*atan2(*y,*x);
}
