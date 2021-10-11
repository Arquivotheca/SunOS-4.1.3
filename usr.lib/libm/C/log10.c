
#ifndef lint
static	char sccsid[] = "@(#)log10.c 1.1 92/07/30 SMI"; 
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* LOG10(X)
 * RETURN THE BASE 10 LOGARITHM OF x
 * code is based on 4.3bsd
 * modified by K.C. Ng for sun 4.0, Jan 30, 1987.
 * 
 * Method :
 *	Let log10_2hi = leading 40 bits of log10(2) and
 *	    log10_2lo = log10(2) - log10_2hi,
 *	    ivln10   = 1/log(10) rounded.
 *	Then
 *		n = ilogb(x), 
 *		if(n<0)  n = n+1;
 *		x = scalbn(x,-n);
 *		LOG10(x) := n*log10_2hi + (n*log10_2lo + ivln10*log(x))
 *
 * Note1:
 *	For fear of destroying log10(10**n)=n, the rounding mode is
 *	set to Round-to-Nearest.
 * Note2:
 *	[1/log(10)] rounded to 53 bits has error  .198   ulps;
 *	log10 is monotonic at all binary break points (tested on a SUN 2).
 *
 * Special cases:
 *	log10(x) is NaN with signal if x < 0; 
 *	log10(+INF) is +INF with no signal; log10(0) is -INF with signal;
 *	log10(NaN) is that NaN with no signal;
 *	log10(10**N) = N  for N=0,1,...,22.
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following constants.
 * The decimal values may be used, provided that the compiler will convert
 * from decimal to binary accurately enough to produce the hexadecimal values
 * shown.
 */

#include <math.h>
#include "libm.h"

static double
ivln10    = 0.43429448190325181667,	/* 2^ -2  * Hex 1.BCB7B1526E50E */
log10_2hi = 0.30102999566361177130,	/* 2^ -2  * Hex 1.34413509F6000 */
log10_2lo = 3.6942390771589305366E-13;	/* 2^ -42 * Hex 1.9FEF311F12B36 */

double log10(x)
double x;
{
	register double y,z;
	enum fp_direction_type rd, _swapRD();
	register n;
	if(!finite(x)) {
		return Inf + x;		/* x is +-INF or NaN */
	} else if (x>0.0) {
		n = ilogb(x);
		if (n < 0 ) n += 1;
		rd = _swapRD(fp_nearest);	/* set default Rounding */
		y  = n;
		x  = scalbn(x,-n);
		z  = y*log10_2lo + ivln10*log(x);
		z += y*log10_2hi;
		if(rd!=fp_nearest) _swapRD(rd);	/* restore Rounding */
		return z;
	} else if (x == 0.0) {		/* -INF */
		return SVID_libm_err(x,x,18);
	} else {	/* x <0 */
		return SVID_libm_err(x,x,19);	/* NaN  */
	}
}
