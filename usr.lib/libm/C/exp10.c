
#ifndef lint
static	char sccsid[] = "@(#)exp10.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* exp10(x)
 * Code by K.C. Ng for SUN 4.0 libm.
 * Method :
 *	n = nint(x*(log10/log2)) ;
 *	exp10(x) = 10**x = exp(x*ln(10)) = exp(n*ln2+(x*ln10-n*ln2))
 *		 = 2**n*exp(ln10*(x-n*log2/log10)))
 *	If x is an integer < 23 then use repeat multiplication. For
 *	10**22 is the largest representable integer.
 */

#include <math.h>
#include "libm.h"

static double 
lg10    =  3.3219280948736234787,	/* log(10)/log(2)      */
ln10    =  2.3025850929940456840,	/* log(10)	       */
logt2hi =  3.0102999565860955045E-1,	/* log(2)/log(10) high */
logt2lo =  5.3716447674669983622E-12,	/* log(2)/log(10) low  */
tiny	=  1.0e-30;

double exp10(x)
double x ;
{
	register double t,ten;
	register k;
	if (!finite(x)) { if(x!=x||x>0) return x+x; else return 0.0;}
	if(fabs(x)<tiny) return 1+x;
	if( x < 310.0)
	    if ( x > -330.0) {
		k = x;
		if (0<=k&&k<23&&(double)k==x) { /* x is a small +integer */
		    t = 1.0;
		    ten = 10.0; if(k&1) t*=ten; k >>=1;
		    while(k) {ten*=ten; if(k&1) t*=ten; k>>=1;}
		    return t;
		}
		t = anint(x*lg10);
		return scalbn(exp(ln10*((x-t*logt2hi)-t*logt2lo)), (int)t);
	    } else 
		return scalbn( 1.0, -5000);	/* underflow */
	else
		return scalbn( 1.0,  5000);	/* overflow  */
}
