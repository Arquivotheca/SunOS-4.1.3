#ifndef lint
static	char sccsid[] = "@(#)r_mod.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE
r_mod(x,y)
float *x, *y;
{
	return r_fmod_(x, y);
}
