
#ifndef lint
static	char sccsid[] = "@(#)d_log2_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_log2_(x)
double *x;
{
	return log2(*x);
}
