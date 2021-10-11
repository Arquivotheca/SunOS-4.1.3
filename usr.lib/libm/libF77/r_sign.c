#ifndef lint
static char     sccsid[] = "@(#)r_sign.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <math.h>

/* if y is negative but not -0 return -|y| */

FLOATFUNCTIONTYPE
r_sign(x, y)
	float          *x, *y;
{
	union {
		float           d;
		unsigned        i;
	}               klugex, klugey;

	klugex.d = *x;
	klugey.d = *y;
	if (klugey.i > 0x80000000)
		klugex.i |= 0x80000000;
	else
		klugex.i &= 0x7fffffff;
	RETURNFLOAT(klugex.d);
}
