#ifndef lint
static	char sccsid[] = "@(#)oldzomplex.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

# include "oldcomplex.h"

Fz_div (c, a, b)
dcomplex *a, *b, *c;
{
double ratio, den;
double abr, abi;
 
abr = b->dreal;
if( abr < 0.)
        abr = - abr;
abi = b->dimag;
if( abi < 0.)
        abi = - abi;
if (abi == 0.0)
        {
        c->dreal = a->dreal/b->dreal ;
        c->dimag = a->dimag/b->dreal ;
        return ;
        }
if( abr <= abi )
        {
        ratio = b->dreal / b->dimag ;
        den = b->dimag * (1 + ratio*ratio);
        c->dreal = (a->dreal*ratio + a->dimag) / den;
        c->dimag = (a->dimag*ratio - a->dreal) / den;
        }
 
else
        {
        ratio = b->dimag / b->dreal ;
        den = b->dreal * (1 + ratio*ratio);
        c->dreal = (a->dreal + a->dimag*ratio) / den;
        c->dimag = (a->dimag - a->dreal*ratio) / den;
        }
}
 
Fz_mult(dc, a, b)
dcomplex *a, *b, *dc;
{
  	dc->dreal = (a->dreal *  b->dreal) - (a->dimag *  b->dimag);
  	dc->dimag = (a->dreal *  b->dimag) + (a->dimag *  b->dreal);
}
 

 
Fz_minus(dc, a, b)
dcomplex *a, *b, *dc;
{
 	dc->dreal = a->dreal - b->dreal;
  	dc->dimag = a->dimag - b->dimag;
}
 

 
Fz_add(dc, a, b)
dcomplex *a, *b, *dc;
{
 	dc->dreal = a->dreal + b->dreal;
 	dc->dimag = a->dimag + b->dimag;
}
 

 
Fz_neg(dc, a)
dcomplex *dc, *a;
{
  	dc->dreal = - a->dreal;
  	dc->dimag = - a->dimag;
}
 

/* convert float to double complex */
 
Ff_conv_z(dc,f)	
dcomplex *dc;
FLOATPARAMETER f;
{
 	dc->dreal = FLOATPARAMETERVALUE(f);
 	dc->dimag = 0.0;
}
 

/* convert double complex to float */

FLOATFUNCTIONTYPE
Fz_conv_f(dc)
dcomplex *dc;
{
float f ;
	f = dc->dreal ;
 	RETURNFLOAT(f);
}
 

/* convert double complex to int */
 
int
Fz_conv_i(dc)
dcomplex *dc;
{
	return (int)dc->dreal;
}
 

/* convert int to double complex */
 
Fi_conv_z(dc,i)	
dcomplex *dc;
int i;
{
 	dc->dreal = (double)i;
 	dc->dimag = 0.0;
}
 

/* convert double complex to double */
 
double
Fz_conv_d(dc)
dcomplex *dc;
{
 	return dc->dreal;
}
 

/* convert double to double complex */
 
Fd_conv_z(dc,d)	
dcomplex *dc;
double d;
{
	dc->dreal = d;
 	dc->dimag = 0.0;
}
 

/* convert double complex to complex  */
 
COMPLEXFUNCTIONTYPE
Fz_conv_c(dc)		
dcomplex *dc;
{
	RETURNCOMPLEX((float)dc->dreal,(float)dc->dimag);
}
