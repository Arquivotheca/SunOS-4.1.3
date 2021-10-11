#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)tanh.c 1.1 92/07/30 SMI"; /* from S5R3 1.10 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	tanh returns the hyperbolic tangent of its double-precision argument.
 *	It calls exp for absolute values of the argument > ~0.55.
 *	There are no error returns.
 *	Algorithm and coefficients from Cody and Waite (1980).
 */

#include <math.h>
#include <values.h>
#define X_BIG	(0.5 * (LN_MAXDOUBLE + M_LN2))
#define LN_3_2	0.54930614433405484570

double
tanh(x)
register double x;
{
	register int neg = 0;

	if (x < 0) {
		x = -x;
		neg++;
	}
	if (x > X_BIG)
		x = 1.0;
	else if (x > LN_3_2) {
		x = 0.5 - 1.0/(exp(x + x) + 1.0); /* two steps recommended */
		x += x;
	} else if (x > X_EPS) { /* skip for efficiency and to prevent underflow */
		static double p[] = {
			-0.96437492777225469787e0,
			-0.99225929672236083313e2,
			-0.16134119023996228053e4,
		}, q[] = {
			 1.0,
			 0.11274474380534949335e3,
			 0.22337720718962312926e4,
			 0.48402357071988688686e4,
		};
		register double y = x * x;

		x += x * y * _POLY2(y, p)/_POLY3(y, q);
	}
	return (neg ? -x : x);
}
