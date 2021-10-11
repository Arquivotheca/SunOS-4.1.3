
#ifndef lint
static	char sccsid[] = "@(#)d_atan2pi_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_atan2pi_(y,x)
double *y,*x;
{
	return atan2pi(*y,*x);
}
