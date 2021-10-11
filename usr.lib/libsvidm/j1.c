#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)j1.c 1.1 92/07/30 SMI"; /* from S5R3 1.13 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	Double-precision Bessel's function
 *	of the first and second kinds
 *	of order one.
 *
 *	j1(x) returns the value of J1(x)
 *	for all real values of x.
 *
 *	Returns ERANGE error and value 0 for large arguments.
 *	Calls sin, cos, sqrt.
 *
 *	There is a niggling bug in J1 that
 *	causes errors up to 2e-16 for x in the
 *	interval [-8, 8].
 *	The bug is caused by an inappropriate order
 *	of summation of the series.
 *
 *	Coefficients are from Hart & Cheney.
 *	#6050 (20.98D)
 *	#6750 (19.19D)
 *	#7150 (19.35D)
 *
 *	y1(x) returns the value of Y1(x)
 *	for positive real values of x.
 *	Returns EDOM error and value -HUGE if argument <= 0.
 *
 *	Calls sin, cos, sqrt, log, j1.
 *
 *	The values of Y1 have not been checked
 *	to more than ten places.
 *
 *	Coefficients are from Hart & Cheney.
 *	#6447 (22.18D)
 *	#6750 (19.19D)
 *	#7150 (19.35D)
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#define P1_0_Q1_0	0.4999999999999999999989557017
#define P2_0_Q2_0	1.0000000000000000000646346901
#define P3_0_Q3_0	0.046874999999999999955398015174
#define DPOLYD(y, p, q)	for (n = d = 0, i = sizeof(p)/sizeof(p[0]); --i >= 0; ) \
				{ n = n * y + p[i]; d = d * y + q[i]; }

static double tpi = 0.6366197723675813430755350535;
static double p1[] = {
	0.581199354001606143928050809e21,
	-.6672106568924916298020941484e20,
	0.2316433580634002297931815435e19,
	-.3588817569910106050743641413e17,
	0.2908795263834775409737601689e15,
	-.1322983480332126453125473247e13,
	0.3413234182301700539091292655e10,
	-.4695753530642995859767162166e7,
	0.2701122710892323414856790990e4,
}, q1[] = {
	0.1162398708003212287858529400e22,
	0.1185770712190320999837113348e20,
	0.6092061398917521746105196863e17,
	0.2081661221307607351240184229e15,
	0.5243710262167649715406728642e12,
	0.1013863514358673989967045588e10,
	0.1501793594998585505921097578e7,
	0.1606931573481487801970916749e4,
	1.0,
};
static double p2[] = {
	-.4435757816794127857114720794e7,
	-.9942246505077641195658377899e7,
	-.6603373248364939109255245434e7,
	-.1523529351181137383255105722e7,
	-.1098240554345934672737413139e6,
	-.1611616644324610116477412898e4,
	0.0,
}, q2[] = {
	-.4435757816794127856828016962e7,
	-.9934124389934585658967556309e7,
	-.6585339479723087072826915069e7,
	-.1511809506634160881644546358e7,
	-.1072638599110382011903063867e6,
	-.1455009440190496182453565068e4,
	1.0,
};
static double p3[] = {
	0.3322091340985722351859704442e5,
	0.8514516067533570196555001171e5,
	0.6617883658127083517939992166e5,
	0.1849426287322386679652009819e5,
	0.1706375429020768002061283546e4,
	0.3526513384663603218592175580e2,
	0.0,
}, q3[] = {
	0.7087128194102874357377502472e6,
	0.1819458042243997298924553839e7,
	0.1419460669603720892855755253e7,
	0.4002944358226697511708610813e6,
	0.3789022974577220264142952256e5,
	0.8638367769604990967475517183e3,
	1.0,
};
static double p4[] = {
	-.9963753424306922225996744354e23,
	0.2655473831434854326894248968e23,
	-.1212297555414509577913561535e22,
	0.2193107339917797592111427556e20,
	-.1965887462722140658820322248e18,
	0.9569930239921683481121552788e15,
	-.2580681702194450950541426399e13,
	0.3639488548124002058278999428e10,
	-.2108847540133123652824139923e7,
	0.0,
}, q4[] = {
	0.5082067366941243245314424152e24,
	0.5435310377188854170800653097e22,
	0.2954987935897148674290758119e20,
	0.1082258259408819552553850180e18,
	0.2976632125647276729292742282e15,
	0.6465340881265275571961681500e12,
	0.1128686837169442121732366891e10,
	0.1563282754899580604737366452e7,
	0.1612361029677000859332072312e4,
	1.0,
};

extern double j1_asympt();

double
j1(x)
register double x;
{
	register double n, d, y;
	register int i;

	if ((y = x) < 0)
		x = -x;
	if (x > 8)
		return (j1_asympt(x, y, 1));
	if (x < X_EPS)
		return (P1_0_Q1_0 * y);
	x *= x;
	DPOLYD(x, p1, q1);
	return (y * n/d);
}

double
y1(x)
register double x;
{
	register double n, d, z;
	register int i;

	if (x <= 0) {
		struct exception exc;

		exc.type = DOMAIN;
		exc.name = "y1";
		exc.arg1 = x;
		exc.retval = -HUGE;
		if (!matherr(&exc)) {
			(void) write(2, "y1: DOMAIN error\n", 17);
			errno = EDOM;
		}
		return (exc.retval);
	}
	if (x > 8)
		return (j1_asympt(x, x, 0));
	z = x * x;
	DPOLYD(z, p4, q4);
	return (x * n/d + tpi * (j1(x) * log(x) - 1/x));
}

static double
j1_asympt(x, n, j1flag)
register double x, n;
int j1flag;
{
	register double z, d, pzero, qzero;
	register int i;
	struct exception exc;

	exc.arg1 = n;
	if (x > X_TLOSS) {
		exc.type = TLOSS;
		exc.name = j1flag ? "j1" : "y1";
		exc.retval = 0.0;
		if (!matherr(&exc)) {
			(void) write(2, exc.name, 2);
			(void) write(2, ": TLOSS error\n", 14);
			errno = ERANGE;
		}
		return (exc.retval);
	}
	if (x > X_PLOSS) {
		pzero = P2_0_Q2_0;
		qzero = P3_0_Q3_0;
	} else {
		z = 64/(x * x);
		DPOLYD(z, p2, q2);
		pzero = n/d;
		DPOLYD(z, p3, q3);
		qzero = n/d;
	}
	qzero *= 8/x;
	z = sqrt(tpi/x);
	pzero *= z;
	qzero *= z;
	x -= 3 * M_PI_4;
	if (!j1flag)
		return (pzero * sin(x) + qzero * cos(x));
	x = pzero * cos(x) - qzero * sin(x);
	return (exc.arg1 < 0 ? -x : x);
}
