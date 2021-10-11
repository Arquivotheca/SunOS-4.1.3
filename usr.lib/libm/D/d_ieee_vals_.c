
#ifndef lint
static char     sccsid[] = "@(#)d_ieee_vals_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "../C/libm.h"

double d_min_subnormal_()
{
	return fmin;
}

double d_max_subnormal_() 
{
	return fmaxs;
}

double d_min_normal_() 
{
	return fminn;
}

double d_max_normal_() 
{
	return fmax;
}

double d_infinity_() 
{
	return Inf;
}

double d_quiet_nan_(n)
long            n;
{
	return NaN;
}

double d_signaling_nan_(n)
long            n;
{
	return sNaN;
}
