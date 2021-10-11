
#ifndef lint
static	char sccsid[] = "@(#)r_acospi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static double invpi = 0.3183098861837906715377675;

FLOATFUNCTIONTYPE r_acospi_(x)
float *x;
{
	float w;
	w = (float) (invpi*acos((double)*x));
	RETURNFLOAT(w);
}
