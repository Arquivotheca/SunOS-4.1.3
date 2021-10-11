#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)atan.c 1.1 92/07/30 SMI"; /* from S5R3 1.13 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	atan returns the arctangent of its double-precision argument,
 *	in the range [-pi/2, pi/2].
 *	There are no error returns.
 *
 *	atan2(y, x) returns the arctangent of y/x,
 *	in the range [-pi, pi].
 *	atan2 discovers what quadrant the angle is in and calls atan.
 *	atan2 returns EDOM error and value 0 if both arguments are zero.
 *
 *	Coefficients are #5077 from Hart & Cheney (19.56D).
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#define SQ2_M_1	0.41421356237309504880
#define SQ2_P_1	2.41421356237309504880
#define NEG	1
#define INV	2
#define MID	4

double
atan(x)
register double x;
{
	register int type = 0;

	if (x < 0) {
		x = -x;
		type = NEG;
	}
	if (x > SQ2_P_1) {
		x = 1/x;
		type |= INV;
	} else if (x > SQ2_M_1) {
		x = (x - 1)/(x + 1);
		type |= MID;
	}
	if (x < -X_EPS || x > X_EPS) {
		/* skip for efficiency and to prevent underflow */
		static double p[] = {
			0.161536412982230228262e2,
			0.26842548195503973794141e3,
			0.11530293515404850115428136e4,
			0.178040631643319697105464587e4,
			0.89678597403663861959987488e3,
		}, q[] = {
			1.0,
			0.5895697050844462222791e2,
			0.536265374031215315104235e3,
			0.16667838148816337184521798e4,
			0.207933497444540981287275926e4,
			0.89678597403663861962481162e3,
		};
		register double xsq = x * x;

		x *= _POLY4(xsq, p)/_POLY5(xsq, q);
	}
	if (type & INV)
		x = M_PI_2 - x;
	else if (type & MID)
		x += M_PI_4;
	return (type & NEG ? -x : x);
}

double
atan2(y, x)
register double y, x;
{
	register int neg_y = 0;
	struct exception exc;
	double at;

	exc.arg1 = y;
	if (!x && !y) {
		exc.type = DOMAIN;
		exc.name = "atan2";
		exc.arg2 = x;
		exc.retval = 0.0;
		if (!matherr(&exc)) {
			(void) write(2, "atan2: DOMAIN error\n", 20);
			errno = EDOM;
		}
		return (exc.retval);
	}
	/*
	 * The next lines determine if |x| is negligible compared to |y|,
	 * without dividing, and without adding values of the same sign.
	 */
	if (exc.arg1 < 0) {
		exc.arg1 = -exc.arg1;
		neg_y++;
	}
	if (exc.arg1 - _ABS(x) == exc.arg1) 
		return (neg_y ? -M_PI_2 : M_PI_2);
	/*
	 * The next line assumes that if y/x underflows the result
	 * is zero with no error indication, so it's safe to divide.
	 */
	at = atan(y/x);
	if (x > 0)		     /* return arctangent directly */
		return (at);
	if (neg_y)		    /* x < 0, adjust arctangent for */
		return (at - M_PI);        /* correct quadrant */
	else
		return (at + M_PI);
}
