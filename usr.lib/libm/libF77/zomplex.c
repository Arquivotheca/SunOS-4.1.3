#ifndef lint
static	char sccsid[] = "@(#)zomplex.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

# include "complex.h"

_Fz_div (c, a, b)
dcomplex *a, *b, *c;
{
	register double ar, ai, br, bi;
	register double ratio, den;

	br = b->dreal;
	bi = b->dimag;
	ar = a->dreal;
	ai = a->dimag;

	if (iszero(bi)) {
		c->dreal = ar / br;
		c->dimag = ai / br;
		return;
	}
	if (fabs(br) <= fabs(bi)) {
		ratio = br / bi;
		den = bi + br * ratio;
		c->dreal = (ar * ratio + ai) / den;
		c->dimag = (ai * ratio - ar) / den;
	} else {
		ratio = bi / br;
		den = br + bi * ratio;
		c->dreal = (ar + ai * ratio) / den;
		c->dimag = (ai - ar * ratio) / den;
	}

}

_Fz_mult(dc, a, b)
dcomplex *a, *b, *dc;
{
  	dc->dreal = (a->dreal *  b->dreal) - (a->dimag *  b->dimag);
  	dc->dimag = (a->dreal *  b->dimag) + (a->dimag *  b->dreal);
}
 

 
_Fz_minus(dc, a, b)
dcomplex *a, *b, *dc;
{
 	dc->dreal = a->dreal - b->dreal;
  	dc->dimag = a->dimag - b->dimag;
}
 

 
_Fz_add(dc, a, b)
dcomplex *a, *b, *dc;
{
 	dc->dreal = a->dreal + b->dreal;
 	dc->dimag = a->dimag + b->dimag;
}
 

 
_Fz_neg(dc, a)
dcomplex *dc, *a;
{
  	dc->dreal = - a->dreal;
  	dc->dimag = - a->dimag;
}
 

/* convert float to double complex */
 
_Ff_conv_z(dc,f)	
dcomplex *dc;
FLOATPARAMETER f;
{
 	dc->dreal = FLOATPARAMETERVALUE(f);
 	dc->dimag = 0.0;
}
 

/* convert double complex to float */

FLOATFUNCTIONTYPE
_Fz_conv_f(dc)
dcomplex *dc;
{
float f ;

	f = dc->dreal ;
 	RETURNFLOAT(f);
}
 

/* convert double complex to int */
 
int
_Fz_conv_i(dc)
dcomplex *dc;
{
	return (int)dc->dreal;
}
 

/* convert int to double complex */
 
_Fi_conv_z(dc,i)	
dcomplex *dc;
int i;
{
 	dc->dreal = (double)i;
 	dc->dimag = 0.0;
}
 

/* convert double complex to double */
 
double
_Fz_conv_d(dc)
dcomplex *dc;
{
 	return dc->dreal;
}
 

/* convert double to double complex */
 
_Fd_conv_z(dc,d)	
dcomplex *dc;
double d;
{
	dc->dreal = d;
 	dc->dimag = 0.0;
}
 

/* convert double complex to complex  */
 
void
_Fz_conv_c(c,dc)		
complex *c;
dcomplex *dc;
{
	c->real = (float)dc->dreal;
	c->imag = (float)dc->dimag;
}
