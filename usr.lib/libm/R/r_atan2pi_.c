
#ifndef lint
static	char sccsid[] = "@(#)r_atan2pi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static double invpi = 0.3183098861837906715377675;

FLOATFUNCTIONTYPE r_atan2pi_(y,x)
float *y,*x;
{
	float w;
	w = (float) (invpi*atan2((double)*y,(double)*x));
	RETURNFLOAT(w);
}
