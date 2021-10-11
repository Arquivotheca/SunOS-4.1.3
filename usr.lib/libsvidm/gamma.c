#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)gamma.c 1.1 92/07/30 SMI"; /* from S5R3 1.16 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	gamma returns the log of the absolute value of the gamma function
 *	of its double-precision argument.
 *	The sign of the gamma function is returned in the
 *	external integer signgam.
 *	Returns EDOM error and value HUGE if argument is non-negative integer.
 *	Returns ERANGE error and value HUGE if the correct value would overflow.
 *
 *	The coefficients for expansion around zero
 *	are #5243 from Hart & Cheney; for expansion
 *	around infinity they are #5404.
 *
 *	Calls log and sin.
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#define X_MAX	(3.0 * H_PREC)
#define GOOBIE	0.9189385332046727417803297

int signgam;

double
gamma(x)
register double x;
{
	extern double pos_gamma();
	struct exception exc;

	exc.type = 0;
	exc.name = "gamma";
	exc.arg1 = x;
	exc.retval = HUGE;
	signgam = 1;
	if (x > 0)
		x = pos_gamma(x, &exc);
	else {
		static double pi = M_PI;
		double temp; /* can't be in register because of modf() below */

		if (!modf(x = -x, &temp)) { /* SING if x is negative integer */
			exc.type = SING;
			if (!matherr(&exc)) {
				(void) write(2, "gamma: SING error\n", 18);
				errno = EDOM;
			}
			return (exc.retval);
		}
		if (x >= X_MAX)
			exc.type = OVERFLOW;
		else {
			if ((temp = sin(pi * x)) < 0)
				temp = -temp;
			else
				signgam = -1;
			return (-(log(x * temp/pi) + pos_gamma(x, &exc)));
		}
	}
	if (exc.type != OVERFLOW)
		return (x);
	if (!matherr(&exc))
		errno = ERANGE;
	return (exc.retval);
}

static double
pos_gamma(x, excp)
register double x;
struct exception *excp;
{
	static double p2[] = {
		-0.67449507245925289918e1,
		-0.50108693752970953015e2,
		-0.43933044406002567613e3,
		-0.20085274013072791214e4,
		-0.87627102978521489560e4,
		-0.20886861789269887364e5,
		-0.42353689509744089647e5,
	}, q2[] = {
		 1.0,
		-0.23081551524580124562e2,
		 0.18949823415702801641e3,
		-0.49902852662143904834e3,
		-0.15286072737795220248e4,
		 0.99403074150827709015e4,
		-0.29803853309256649932e4,
		-0.42353689509744090010e5,
	};
	register double y, z;

	if (x > 8) { /* asymptotic approximation */
		static double p[] = {
			-0.1633436431e-2,
			 0.83645878922e-3,
			-0.5951896861197e-3,
			 0.793650576493454e-3,
			-0.277777777735865004e-2,
			 0.83333333333333101837e-1,
		};
	
		if (x >= MAXDOUBLE/LN_MAXDOUBLE) {
			excp->type = OVERFLOW;
			return (excp->retval);
		}
		z = (x - 0.5) * log(x) - x + GOOBIE;
		if (x > X_MAX)
			return (z);
		x = 1/x;
		y = x * x;
		return (z + x * _POLY5(y, p));
	}
	y = 1;
	if (x < y)
		y /= (x * (y + x));
	else if (x < 2) {
		y /= x;
		x -= 1;
	} else {
		for ( ; x >= 3; y *= x)
			x -= 1;
		x -= 2;
	}
	return (log(y * _POLY6(x, p2)/_POLY7(x, q2)));
}
