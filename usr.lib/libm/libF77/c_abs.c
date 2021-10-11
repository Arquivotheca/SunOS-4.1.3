#ifndef lint
static	char sccsid[] = "@(#)c_abs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "complex.h"

FLOATFUNCTIONTYPE
c_abs(z)
complex *z;
{
	return r_hypot_(&(z->real), &(z->imag));
}
