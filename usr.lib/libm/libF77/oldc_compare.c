#ifndef lint
static	char sccsid[] = "@(#)oldc_compare.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*	complex*8 comparison	*/

#include "oldcomplex.h"

int
Fc_eq(xr, xi , yr, yi)
	FLOATPARAMETER xr, xi, yr, yi;
{
	return  (FLOATPARAMETERVALUE(xr) == FLOATPARAMETERVALUE(yr)) &&
		(FLOATPARAMETERVALUE(xi) == FLOATPARAMETERVALUE(yi));
}

int
Fc_ne(xr, xi , yr, yi)
	FLOATPARAMETER xr, xi, yr, yi;
{
	return  (FLOATPARAMETERVALUE(xr) != FLOATPARAMETERVALUE(yr)) ||
		(FLOATPARAMETERVALUE(xi) != FLOATPARAMETERVALUE(yi));
}
