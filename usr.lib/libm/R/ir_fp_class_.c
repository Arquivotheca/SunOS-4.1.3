
#ifndef lint
static	char sccsid[] = "@(#)ir_fp_class_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

enum fp_class_type ir_fp_class_(x)
float *x;
{
	long *px = (long *) x, k;
	k = px[0]&0x7f800000;
	if(k==0) { 
		if ((px[0]&0x7fffffff)==0) 	
			return fp_zero; 		/* 0 */
		else 		
			return fp_subnormal; 		/* 1 */
	}
	else if(k!=0x7f800000) 	return fp_normal;	/* 2 */
	else {
		k=(px[0]&0x007fffff);
		if(k==0) 	return fp_infinity; 	/* 3 */
		else if((px[0]&0x00400000)!=0) 
				return fp_quiet; 	/* 4 */
		else 		return fp_signaling;	/* 5 */
	}
}
