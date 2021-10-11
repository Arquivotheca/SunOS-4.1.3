#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)exp.c 1.1 92/07/30 SMI"; /* from S5R3 1.13 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	exp returns the exponential function of its double-precision argument.
 *	Returns ERANGE error and value 0 if argument too small,
 *	   value HUGE if argument too large.
 *	Algorithm and coefficients from Cody and Waite (1980).
 *	Calls ldexp.
 */

#include <math.h>
#include <values.h>
#include <errno.h>

double
exp(x)
register double x;
{
	static double p[] = {
		0.31555192765684646356e-4,
	        0.75753180159422776666e-2,
	        0.25000000000000000000e0,
	}, q[] = {
		0.75104028399870046114e-6,
	        0.63121894374398503557e-3,
	        0.56817302698551221787e-1,
	        0.50000000000000000000e0,
	};
	register double y;
	register int n;
	struct exception exc;

	exc.arg1 = x;
	if (x < 0)
		x = -x;
	if (x < X_EPS) /* use first term of Taylor series expansion */
		return (1.0 + exc.arg1);
	exc.name = "exp";
	if (exc.arg1 <= LN_MINDOUBLE) {
		if (exc.arg1 == LN_MINDOUBLE) /* protect against roundoff */
			return (MINDOUBLE); /* causing ldexp to underflow */
		exc.type = UNDERFLOW;
		exc.retval = 0.0;
		if (!matherr(&exc))
			errno = ERANGE;
		return (exc.retval);
	}
	if (exc.arg1 >= LN_MAXDOUBLE) {
		if (exc.arg1 == LN_MAXDOUBLE) /* protect against roundoff */
			return (MAXDOUBLE); /* causing ldexp to overflow */
		exc.type = OVERFLOW;
		exc.retval = HUGE;
		if (!matherr(&exc)) 
			errno = ERANGE;
		return (exc.retval);
	}
	n = (int)(x * M_LOG2E + 0.5);
	y = (double)n;
	_REDUCE(int, x, y, 0.693359375, -2.1219444005469058277e-4);
	if (exc.arg1 < 0) {
		x = -x;
		n = -n;
	}
	y = x * x;
	x *= _POLY2(y, p);
	return (ldexp(0.5 + x/(_POLY3(y, q) - x), n + 1));
}
