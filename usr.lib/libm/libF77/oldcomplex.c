#ifndef lint
static	char sccsid[] = "@(#)oldcomplex.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "oldcomplex.h"

COMPLEXFUNCTIONTYPE 
Fc_div ( xr, xi, yr, yi )
FLOATPARAMETER  xr, xi, yr, yi;
{
	complex a, b, c ;

	a.real = FLOATPARAMETERVALUE(xr);
	a.imag = FLOATPARAMETERVALUE(xi);
	b.real = FLOATPARAMETERVALUE(yr);
	b.imag = FLOATPARAMETERVALUE(yi);
	c_div( &c, &a, &b );
	RETURNCOMPLEX(c.real,c.imag);
}


COMPLEXFUNCTIONTYPE 
Fc_mult ( xr, xi, yr, yi)
FLOATPARAMETER xr, xi, yr, yi;
{
	RETURNCOMPLEX(
		(FLOATPARAMETERVALUE(xr) * FLOATPARAMETERVALUE(yr) -
       	  	 FLOATPARAMETERVALUE(xi) * FLOATPARAMETERVALUE(yi)),
		(FLOATPARAMETERVALUE(xr) * FLOATPARAMETERVALUE(yi) +
		 FLOATPARAMETERVALUE(xi) * FLOATPARAMETERVALUE(yr)));
}

COMPLEXFUNCTIONTYPE 
Fc_minus ( xr, xi, yr, yi)
FLOATPARAMETER xr, xi, yr, yi;
{
	RETURNCOMPLEX(
		FLOATPARAMETERVALUE(xr) - FLOATPARAMETERVALUE(yr),
		FLOATPARAMETERVALUE(xi) - FLOATPARAMETERVALUE(yi));
}

COMPLEXFUNCTIONTYPE 
Fc_add ( xr, xi, yr, yi)
FLOATPARAMETER xr, xi, yr, yi;
{
	RETURNCOMPLEX(
		FLOATPARAMETERVALUE(xr) + FLOATPARAMETERVALUE(yr),
		FLOATPARAMETERVALUE(xi) + FLOATPARAMETERVALUE(yi));
}

COMPLEXFUNCTIONTYPE 
Fc_neg ( xr, xi)
FLOATPARAMETER xr, xi;
{
	RETURNCOMPLEX(
		-FLOATPARAMETERVALUE(xr),
		-FLOATPARAMETERVALUE(xi));
}

/* Convert float to complex */

COMPLEXFUNCTIONTYPE 
Ff_conv_c ( x )
FLOATPARAMETER x;
{
	RETURNCOMPLEX(
		FLOATPARAMETERVALUE(x),
		0.0);
}

/* Convert complex to float */

FLOATFUNCTIONTYPE
Fc_conv_f( xr,xi )
FLOATPARAMETER  xr, xi;
{
	RETURNFLOAT(FLOATPARAMETERVALUE(xr));
}

/* Convert complex to int */

int
Fc_conv_i ( xr, xi )
FLOATPARAMETER xr, xi;
{
	return (int) FLOATPARAMETERVALUE(xr);
}

/* Convert int to complex */

COMPLEXFUNCTIONTYPE
Fi_conv_c ( x )
int x;
{
	RETURNCOMPLEX(
		(float)(x),
		0.0);
}

/* Convert complex to double */

double
Fc_conv_d ( xr, xi )
FLOATPARAMETER xr, xi;
{
	return (double) FLOATPARAMETERVALUE(xr);
}


/* Convert double to complex */

COMPLEXFUNCTIONTYPE
Fd_conv_c ( x )
double x;
{
	complex result;

	result.imag = 0.0;
	result.real = (float)(x);
	RETURNCOMPLEX(
		(float)(x),
		0.0);
}

/* Convert complex to double complex */

Fc_conv_z( result, xr, xi)
dcomplex *result;
FLOATPARAMETER xr, xi;
{

	result->dreal = FLOATPARAMETERVALUE(xr);
	result->dimag = FLOATPARAMETERVALUE(xi);
}
