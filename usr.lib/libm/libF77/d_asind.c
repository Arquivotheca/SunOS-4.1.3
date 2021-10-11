#ifndef lint
 static	char sccsid[] = "@(#)d_asind.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif
/*
 *  VMS Fortran compatibility function:
 *  arc sine in degrees.
 */

#include <math.h>
static double pitod = 57.2957795130823208767981548141; /* 180/pi */

double d_asind(x)
double *x;
{
	return pitod*asin(*x);
}
