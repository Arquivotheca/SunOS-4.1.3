#ifndef lint
static char     sccsid[] = "@(#)complex.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "complex.h"

_Fc_div(c, a, b)
	complex        *a, *b, *c;
{
	register double ar, ai, br, bi;
	register double den;


	if (((*(int *) &(b->imag)) & 0x7fffffff) == 0) {	/* Check for division by
								 * real. */
		c->real = a->real / b->real;
		c->imag = a->imag / b->real;
		return;
	}
	br = b->real;
	bi = b->imag;
	ar = a->real;
	ai = a->imag;
	den = bi * bi + br * br;
	c->real = (ar * br + ai * bi) / den;
	c->imag = (ai * br - ar * bi) / den;
}

_Fc_mult(c, a, b)
	complex        *a, *b, *c;
{
	c->real = (a->real * b->real) - (a->imag * b->imag);
	c->imag = (a->real * b->imag) + (a->imag * b->real);
}



_Fc_minus(c, a, b)
	complex        *a, *b, *c;
{
	c->real = a->real - b->real;
	c->imag = a->imag - b->imag;
}



_Fc_add(c, a, b)
	complex        *a, *b, *c;
{
	c->real = a->real + b->real;
	c->imag = a->imag + b->imag;
}



_Fc_neg(c, a)
	complex        *c, *a;
{
	c->real = -a->real;
	c->imag = -a->imag;
}

/* Convert float to complex */

void
_Ff_conv_c(c, x)
	complex        *c;
	FLOATPARAMETER  x;
{
	c->real = FLOATPARAMETERVALUE(x);
	c->imag = 0.0;
}

/* Convert complex to float */

FLOATFUNCTIONTYPE
_Fc_conv_f(c)
	complex        *c;
{
	RETURNFLOAT(c->real);
}

/* Convert complex to int */

int
_Fc_conv_i(c)
	complex        *c;
{
	return (int) c->real;
}

/* Convert int to complex */

void
_Fi_conv_c(c, i)
	complex        *c;
	int             i;
{
	c->real = (float) i;
	c->imag = 0.0;
}

/* Convert complex to double */

double
_Fc_conv_d(c)
	complex        *c;
{
	return (double) c->real;
}


/* Convert double to complex */

void
_Fd_conv_c(c, x)
	complex        *c;
	double          x;
{
	c->real = (float) (x);
	c->imag = 0.0;
}

/* Convert complex to double complex */

void
_Fc_conv_z(result, c)
	dcomplex       *result;
	complex        *c;
{
	result->dreal = (double) c->real;
	result->dimag = (double) c->imag;
}
