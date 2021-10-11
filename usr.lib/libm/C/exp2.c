
#ifndef lint
static	char sccsid[] = "@(#)exp2.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* exp2(x)
 * Code by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	exp2(x) = 2**x = 2**((x-anint(x))+anint(x)) 
 *		= 2**anint(x)*2**(x-anint(x))
 *		= 2**anint(x)*exp((x-anint(x))*ln2)
 */

#include <math.h>
#include "libm.h"

static double tiny = 1.0e-30;

double exp2(x)
double x ;
{
	register double t;
	if (!finite(x)) { if(x!=x||x>0) return x+x; else return 0.0;}
	t = fabs(x);
	if(t<0.5) if (t<tiny) return 1.0+x; else return exp(ln2*x);
	t = anint(x);
	if( t < 1030.0)
	    if ( t >= -1080.0) 
		return scalbn( exp(ln2*(x-t)), (int) t);
	    else 
		return scalbn( 1.0, -5000);	/* underflow */
	else
		return scalbn( 1.0,  5000);	/* overflow  */
}
