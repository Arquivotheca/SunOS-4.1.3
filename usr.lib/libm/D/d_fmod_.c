
#ifndef lint
static	char sccsid[] = "@(#)d_fmod_.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

double d_fmod_(x,y)
double *x,*y;
{
	return fmod(*x,*y);
}
