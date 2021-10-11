#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)log.c 1.1 92/07/30 SMI"; /* from S5R3 1.14 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	log returns the natural logarithm of its double-precision argument.
 *	log10 returns the base-10 logarithm of its double-precision argument.
 *	Returns EDOM error and value -HUGE if argument <= 0.
 *	Algorithm and coefficients from Cody and Waite (1980).
 *	Calls frexp.
 */

#include <math.h>
#include <errno.h>
#define C1	0.693359375
#define C2	-2.121944400546905827679e-4

extern double log_error();

double
log(x)
register double x;
{
	static double p[] = {
		-0.78956112887491257267e0,
		 0.16383943563021534222e2,
		-0.64124943423745581147e2,
	}, q[] = {
		 1.0,
		-0.35667977739034646171e2,
		 0.31203222091924532844e3,
		-0.76949932108494879777e3,
	};
	register double y;
	int n; /* can't be in register because of frexp() below */

	if (x <= 0)
		return (log_error(x, "log", 3));
	y = 1.0;
	x = frexp(x, &n);
	if (x < M_SQRT1_2) {
		n--;
		y = 0.5;
	}
	x = (x - y)/(x + y);
	x += x;
	y = x * x;
	x += x * y * _POLY2(y, p)/_POLY3(y, q);
	y = (double)n;
	x += y * C2;
	return (x + y * C1);
}

double
log10(x)
register double x;
{
	return (x > 0 ? log(x) * M_LOG10E : log_error(x, "log10", 5));
}

static double
log_error(x, f_name, name_len)
double x;
char *f_name;
unsigned int name_len;
{
	register char *err_type;
	unsigned int mess_len;
	struct exception exc;

	exc.name = f_name;
	exc.retval = -HUGE;
	exc.arg1 = x;
	if (x) {
		exc.type = DOMAIN;
		err_type = ": DOMAIN error\n";
		mess_len = 15;
	} else {
		exc.type = SING;
		err_type = ": SING error\n";
		mess_len = 13;
	}
	if (!matherr(&exc)) {
		(void) write(2, f_name, name_len);
		(void) write(2, err_type, mess_len);
		errno = EDOM;
	}
	return (exc.retval);
}
