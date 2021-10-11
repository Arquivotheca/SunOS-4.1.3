
#ifndef lint
static	char sccsid[] = "@(#)modf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double modf(x,iptr)
double x, *iptr ;
{
	*iptr = aint(x);
	if(finite(x)&&x!=(*iptr)) 
		return x - (*iptr);
	else
		return copysign(0.0,x);
}
