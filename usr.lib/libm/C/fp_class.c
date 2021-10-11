#ifndef lint
static	char sccsid[] = "@(#)fp_class.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* IEEE functions
 * 	fp_class()
 */

#include <math.h>
#include "libm.h"

enum fp_class_type fp_class(x)
double x;
{
double	one	= 1.0;
register	n0;

	double w=x;
	long *pw = (long *) &w, k;

	if ((* (int *) &one) != 0) n0=0;	/* not a i386 */
	else n0=1;				/* is a i386  */

	k = pw[n0]&0x7ff00000;
	if(k==0) { 
		k = (pw[n0]&0x7fffffff)|pw[1-n0];
		if (k==0) 	return fp_zero; 	/* 0 */
		else 		return fp_subnormal; 	/* 1 */
	}
	else if(k!=0x7ff00000) 	return fp_normal;	/* 2 */
	else {
		k=(pw[n0]&0x000fffff)|pw[1-n0];
		if(k==0) 	return fp_infinity; 	/* 3 */
		else if((pw[n0]&0x00080000)!=0) 
				return fp_quiet; 	/* 4 */
		else 		return fp_signaling;	/* 5 */
	}
}
