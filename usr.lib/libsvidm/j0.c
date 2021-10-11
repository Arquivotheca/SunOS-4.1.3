#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)j0.c 1.1 92/07/30 SMI"; /* from S5R3 1.15 */
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
 *	of order zero.
 *
 *	j0(x) returns the value of J0(x)
 *	for all real values of x.
 *
 *	Returns ERANGE error and value 0 for large arguments.
 *	Calls sin, cos, sqrt.
 *
 *	There is a niggling bug in J0 that
 *	causes errors up to 2e-16 for x in the
 *	interval [-8, 8].
 *	The bug is caused by an inappropriate order
 *	of summation of the series.
 *
 *	Coefficients are from Hart & Cheney.
 *	#5849 (19.22D)
 *	#6549 (19.25D)
 *	#6949 (19.41D)
 *
 *	y0(x) returns the value of Y0(x)
 *	for positive real values of x.
 *	Returns EDOM error and value -HUGE if argument <= 0.
 *
 *	Calls sin, cos, sqrt, log, j0.
 *
 *	The values of Y0 have not been checked
 *	to more than ten places.
 *
 *	Coefficients are from Hart & Cheney.
 *	#6245 (18.78D)
 *	#6549 (19.25D)
 *	#6949 (19.41D)
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#define P2_0_Q2_0	0.999999999999999999944688442
#define P3_0_Q3_0	-0.0156249999999999999611615235
#define P4_0_Q4_0	0.073804295108687225110222
#define DPOLYD(y, p, q)	for (n = d = 0, i = sizeof(p)/sizeof(p[0]); --i >= 0; ) \
				{ n = n * y + p[i]; d = d * y + q[i]; }

static double tpi = 0.6366197723675813430755350535;
static double p1[] = {
	0.4933787251794133561816813446e21,
	-.1179157629107610536038440800e21,
	0.6382059341072356562289432465e19,
	-.1367620353088171386865416609e18,
	0.1434354939140344111664316553e16,
	-.8085222034853793871199468171e13,
	0.2507158285536881945555156435e11,
	-.4050412371833132706360663322e8,
	0.2685786856980014981415848441e5,
}, q1[] = {
	0.4933787251794133562113278438e21,
	0.5428918384092285160200195092e19,
	0.3024635616709462698627330784e17,
	0.1127756739679798507056031594e15,
	0.3123043114941213172572469442e12,
	0.6699987672982239671814028660e9,
	0.1114636098462985378182402543e7,
	0.1363063652328970604442810507e4,
	1.0,
};
static double p2[] = {
	0.5393485083869438325262122897e7,
	0.1233238476817638145232406055e8,
	0.8413041456550439208464315611e7,
	0.2016135283049983642487182349e7,
	0.1539826532623911470917825993e6,
	0.2485271928957404011288128951e4,
	0.0,
}, q2[] = {
	0.5393485083869438325560444960e7,
	0.1233831022786324960844856182e8,
	0.8426449050629797331554404810e7,
	0.2025066801570134013891035236e7,
	0.1560017276940030940592769933e6,
	0.2615700736920839685159081813e4,
	1.0,
};
static double p3[] = {
	-.3984617357595222463506790588e4,
	-.1038141698748464093880530341e5,
	-.8239066313485606568803548860e4,
	-.2365956170779108192723612816e4,
	-.2262630641933704113967255053e3,
	-.4887199395841261531199129300e1,
	0.0,
}, q3[] = {
	0.2550155108860942382983170882e6,
	0.6667454239319826986004038103e6,
	0.5332913634216897168722255057e6,
	0.1560213206679291652539287109e6,
	0.1570489191515395519392882766e5,
	0.4087714673983499223402830260e3,
	1.0,
};
static double p4[] = {
	-.2750286678629109583701933175e20,
	0.6587473275719554925999402049e20,
	-.5247065581112764941297350814e19,
	0.1375624316399344078571335453e18,
	-.1648605817185729473122082537e16,
	0.1025520859686394284509167421e14,
	-.3436371222979040378171030138e11,
	0.5915213465686889654273830069e8,
	-.4137035497933148554125235152e5,
}, q4[] = {
	0.3726458838986165881989980e21,
	0.4192417043410839973904769661e19,
	0.2392883043499781857439356652e17,
	0.9162038034075185262489147968e14,
	0.2613065755041081249568482092e12,
	0.5795122640700729537480087915e9,
	0.1001702641288906265666651753e7,
	0.1282452772478993804176329391e4,
	1.0,
};

extern double j0_asympt();

double
j0(x)
register double x;
{
	register double n, d;
	register int i;

	if ((n = x) < 0)
		x = -x;
	if (x > 8)
		return (j0_asympt(x, n, 1));
	if (x < X_EPS)
		return (1);
	x *= x;
	DPOLYD(x, p1, q1);
	return (n/d);
}

double
y0(x)
register double x;
{
	register double n, d, y, z;
	register int i;

	if (x <= 0) {
		struct exception exc;

		exc.type = DOMAIN;
		exc.name = "y0";
		exc.arg1 = x;
		exc.retval = -HUGE;
		if (!matherr(&exc)) {
			(void) write(2, "y0: DOMAIN error\n", 17);
			errno = EDOM;
		}
		return (exc.retval);
	}
	if (x > 8)
		return (j0_asympt(x, x, 0));
	y = tpi * log(x);
	if (x < X_EPS)
		return (y - P4_0_Q4_0);
	z = x * x;
	DPOLYD(z, p4, q4);
	return (n/d + y * j0(x));
}

static double
j0_asympt(x, n, j0flag)
register double x, n;
int j0flag;
{
	register double z, d, pzero, qzero;
	register int i;

	if (x > X_TLOSS) {
		struct exception exc;

		exc.type = TLOSS;
		exc.name = j0flag ? "j0" : "y0";
		exc.arg1 = n;
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
	x -= M_PI_4;
	return (j0flag ? pzero * cos(x) - qzero * sin(x) :
			 pzero * sin(x) + qzero * cos(x));
}
