#ifndef lint
static char     sccsid[] = "@(#)oldc_div.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "complex.h"

c_div(c, a, b)
	complex        *a, *b, *c;
{
	register double ar, ai, br, bi;
	register double den;

	br = b->real;
	bi = b->imag;
	ar = a->real;
	ai = a->imag;

	if (fabs(bi) == 0.0) {
		c->real = ar / br;
		c->imag = ai / br;
		return;
	}
	den = bi * bi + br * br;
	c->real = (ar * br + ai * bi) / den;
	c->imag = (ai * br - ar * bi) / den;
}
