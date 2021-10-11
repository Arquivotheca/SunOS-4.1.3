
#ifndef lint
static	char sccsid[] = "@(#)d_sinh_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_sinh_(x)
double *x;
{
	return sinh(*x);
}
