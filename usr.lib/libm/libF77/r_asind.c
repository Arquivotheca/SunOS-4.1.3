#ifndef lint
 static	char sccsid[] = "@(#)r_asind.c 1.1 92/07/30 SMI"; /* from UCB 1.1" */
#endif

/*
 *  VMS Fortran compatibility function:
 *  arc sine in degrees.
 */

#include <math.h>
static float pitod = 57.2957795130823208767981548141; /* 180/pi */

FLOATFUNCTIONTYPE r_asind(x)
float *x;
{
	FLOATFUNCTIONTYPE w;
	float z;
	w = r_asin_(x);
	z = *((float *)&w) * pitod;
	RETURNFLOAT(z);
}

