#ifndef lint
static	char sccsid[] = "@(#)r_imag.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "complex.h"

FLOATFUNCTIONTYPE
r_imag(z)
complex *z;
{
	RETURNFLOAT(z->imag);
}
