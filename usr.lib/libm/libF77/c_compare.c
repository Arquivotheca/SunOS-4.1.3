#ifndef lint
static char     sccsid[] = "@(#)c_compare.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

/*	complex*8 comparison	*/

#include "complex.h"

int
_Fc_eq(x, y)
	complex *x, *y;
{
	return  (x->real == y->real) && (x->imag == y->imag);
}

int
_Fc_ne(x, y)
	complex *x, *y;
{
	return  (x->real != y->real) || (x->imag != y->imag);
}
