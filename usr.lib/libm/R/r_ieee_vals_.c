
#ifndef lint
static char     sccsid[] = "@(#)r_ieee_vals_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <math.h>

FLOATFUNCTIONTYPE 
r_min_subnormal_()
{
	float w;
	*(long *)(&w) = 0x00000001;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_max_subnormal_()
{
	float w;
	*(long *)(&w) = 0x007fffff;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_min_normal_()
{
	float w;
	*(long *)(&w) = 0x00800000;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_max_normal_()
{
	float w;
	*(long *)(&w) = 0x7f7fffff;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_infinity_()
{
	float w;
	*(long *)(&w) = 0x7f800000;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_quiet_nan_(n)
long *n;
{
	float w;
	*(long *)(&w) = 0x7fffffff;
	RETURNFLOAT(w);
}

FLOATFUNCTIONTYPE 
r_signaling_nan_(n)
long *n;
{
	float w;
	*(long *)(&w) = 0x7f800001;
	RETURNFLOAT(w);
}
