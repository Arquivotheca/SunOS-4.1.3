
#ifndef lint
static	char sccsid[] = "@(#)d_atan2_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_atan2_(y,x)
double *y,*x;
{
	return atan2(*y,*x);
}
