
#ifndef lint
static	char sccsid[] = "@(#)r_atanpi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static double invpi = 0.3183098861837906715377675;

FLOATFUNCTIONTYPE r_atanpi_(x)
float *x;
{
	float w;
	w = (float) (invpi*atan((double)*x));
	RETURNFLOAT(w);
}
