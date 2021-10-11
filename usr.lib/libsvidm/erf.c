#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)erf.c 1.1 92/07/30 SMI"; /* from S5R3 1.13 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	erf returns the error function of its double-precision argument.
 *	erfc(x) returns 1 - erf(x).
 *
 *	erf(x) is defined by
 *	${2 over sqrt pi} int from 0 to x e sup {- t sup 2} dt$.
 *
 *	The entry for erfc is provided because of the
 *	extreme loss of relative accuracy if erf(x) is
 *	called for large x and the result subtracted
 *	from 1 (e.g. for x = 5, 12 places are lost).
 *
 *	There are no error returns.
 *
 *	Calls exp for |x| > 0.5.
 *
 *	Coefficients for large x are #5667 from Hart & Cheney (18.72D).
 */

#include <math.h>
/* approx sqrt(log(MAXDOUBLE)) */
#if u3b || M32
#define MAXVAL	27.23
#else
#define MAXVAL	 9.27
#endif
#define DPOLYD(y, p, q)	for (n = d = 0, i = sizeof(p)/sizeof(p[0]); --i >= 0; ) \
				{ n = n * y + p[i]; d = d * y + q[i]; }

static double p1[] = {
	0.804373630960840172832162e5,
	0.740407142710151470082064e4,
	0.301782788536507577809226e4,
	0.380140318123903008244444e2,
	0.143383842191748205576712e2,
	-.288805137207594084924010e0,
	0.007547728033418631287834e0,
}, q1[]  = {
	0.804373630960840172826266e5,
	0.342165257924628539769006e5,
	0.637960017324428279487120e4,
	0.658070155459240506326937e3,
	0.380190713951939403753468e2,
	1.0,
	0.0,
};
static double p2[]  = {
	0.18263348842295112592168999e4,
	0.28980293292167655611275846e4,
	0.2320439590251635247384768711e4,
	0.1143262070703886173606073338e4,
	0.3685196154710010637133875746e3,
	0.7708161730368428609781633646e2,
	0.9675807882987265400604202961e1,
	0.5641877825507397413087057563e0,
	0.0,
}, q2[]  = {
	0.18263348842295112595576438e4,
	0.495882756472114071495438422e4,
	0.60895424232724435504633068e4,
	0.4429612803883682726711528526e4,
	0.2094384367789539593790281779e4,
	0.6617361207107653469211984771e3,
	0.1371255960500622202878443578e3,
	0.1714980943627607849376131193e2,
	1.0,
};

double
erf(x)
register double x;
{
	int neg = 0;

	if (x < 0) {
		x = -x;
		neg++;
	}
	if (x > 0.5)
		x = 1 - erfc(x);
	else {
		register double n, d, xsq = x * x;
		register int i;

		DPOLYD(xsq, p1, q1);
		x *= M_2_SQRTPI * n/d;
	}
	return (neg ? -x : x);
}

double
erfc(x)
register double x;
{
	register double n, d;
	register int i;

	if (x < 0.5)
		return (1 - erf(x));
	if (x >= MAXVAL) /* exp(-x * x) sure to underflow */
		return (0.0);
	DPOLYD(x, p2, q2);
	return (exp(-x * x) * n/d);
}
