#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)sqrt.c 1.1 92/07/30 SMI"; /* from S5R3 1.11 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	sqrt returns the square root of its double-precision argument,
 *	using Newton's method.
 *	Returns EDOM error and value 0 if argument negative.
 *	Calls frexp and ldexp.
 */

#include <errno.h>
#include <math.h>
#define ITERATIONS	4

double
sqrt(x)
register double x;
{
	register double y;
	int iexp; /* can't be in register because of frexp() below */
	register int i = ITERATIONS;

	if (x <= 0) {
		struct exception exc;

		if (!x)
			return (x); /* sqrt(0) == 0 */
		exc.type = DOMAIN;
		exc.name = "sqrt";
		exc.arg1 = x;
		exc.retval = 0.0;
		if (!matherr(&exc)) {
			(void) write(2, "sqrt: DOMAIN error\n", 19);
			errno = EDOM;
		}
		return (exc.retval);
	}
	y = frexp(x, &iexp); /* 0.5 <= y < 1 */
	if (iexp % 2) { /* iexp is odd */
		--iexp;
		y += y; /* 1 <= y < 2 */
	}
	y = ldexp(y + 1.0, iexp/2 - 1); /* first guess for sqrt */
	do {
		y = 0.5 * (y + x/y);
	} while (--i > 0);
	return (y);
}
