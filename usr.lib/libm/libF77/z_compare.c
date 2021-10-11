#ifndef lint
static char     sccsid[] = "@(#)z_compare.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

/*	complex*16 comparison	*/

#include "complex.h"

int
Fz_eq(x, y)
	register dcomplex          *x, *y;
{
	return (x->dreal == y->dreal) && (x->dimag == y->dimag);
}

int
Fz_ne(x, y)
	register dcomplex          *x, *y;
{
	return (x->dreal != y->dreal) || (x->dimag != y->dimag);
}
