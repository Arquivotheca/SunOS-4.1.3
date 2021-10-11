
#ifndef lint
static	char sccsid[] = "@(#)r_asinpi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

static double invpi = 0.3183098861837906715377675;

FLOATFUNCTIONTYPE r_asinpi_(x)
float *x;
{
	float w;
	w = (float) (invpi*asin((double)*x));
	RETURNFLOAT(w);
}
