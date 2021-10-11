#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)tan.c 1.1 92/07/30 SMI"; /* from S5R3 1.14 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	tan returns the tangent of its double-precision argument.
 *	Returns ERANGE error and value 0 if argument too large.
 *	Algorithm and coefficients from Cody and Waite (1980).
 */

#include <math.h>
#include <values.h>
#include <errno.h>

double
tan(x)
register double x;
{
	register double y;
	register int neg = 0, invert;
	struct exception exc;

	exc.name = "tan";
	exc.arg1 = x;
	if (x < 0) {
		x = -x;
		neg++;
	}
	if (x > X_TLOSS/2) {
		exc.type = TLOSS;
		exc.retval = 0.0;
		if (!matherr(&exc)) {
			(void) write(2, "tan: TLOSS error\n", 17);
			errno = ERANGE;
		}
		return (exc.retval);
	}
	y = x * M_2_PI + 0.5;
	if (x <= MAXLONG) { /* reduce using integer arithmetic if possible */
		register long n = (long)y;

		y = (double)n;
		_REDUCE(long, x, y, 1.57080078125, -4.454455103380768678308e-6);
		invert = (int)n % 2;
	} else {
		extern double modf();
		double dn;

		x = (modf(y, &dn) - 0.5) * M_PI_2;
		invert = modf(0.5 * dn, &dn) ? 1 : 0;
	}
	if (x > -X_EPS && x < X_EPS) {
		if (!x && invert)
			/*
			 * This case represents a range reduction
			 * from (2n + 1) * PI/2 which happens to return 0.
			 * This is mathematically insignficant, and extremely
			 * unlikely in any case, so it is best simply
			 * to substitute an arbitrarily small value
			 * as if the reduction were one bit away from 0,
			 * and not to deal with this as a range error.
			 */
			x = 1.0/X_TLOSS;
		y = 1.0; /* skip for efficiency and to prevent underflow */
	} else {
		static double p[] = {
			-0.17861707342254426711e-4,
			 0.34248878235890589960e-2,
			-0.13338350006421960681e+0,
		}, q[] = {
			 0.49819433993786512270e-6,
			-0.31181531907010027307e-3,
			 0.25663832289440112864e-1,
			-0.46671683339755294240e+0,
			 1.0,
		};

		y = x * x;
		x += x * y * _POLY2(y, p);
		y = _POLY4(y, q);
	}
	if (neg)
		x = -x;
	exc.retval = invert ? -y/x : x/y;
	if (exc.arg1 > X_PLOSS/2 || exc.arg1 < -X_PLOSS/2) {
		exc.type = PLOSS;
		if (!matherr(&exc))
			errno = ERANGE;
	}
	return (exc.retval);
}
