
#ifndef lint
static	char sccsid[] = "@(#)r_acos_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE r_acos_(x)
float *x;
{
	float w;
	w = (float) acos((double)*x);
	RETURNFLOAT(w);
}
