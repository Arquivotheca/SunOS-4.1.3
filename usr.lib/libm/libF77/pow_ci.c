#ifndef lint
static char     sccsid[] = "@(#)pow_ci.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "complex.h"

#define RETURN(R,M) { p->real = R ; p->imag = M ; return ; }

#define SQUAREPOWER \
	{ treal = (powerr + powerm) * (powerr - powerm) ; \
	powerm = 2.0 * powerr * powerm ; \
	powerr = treal ; }

pow_ci(p, ap, bp)		/* p = a**b  */
	complex        *p, *ap;
	long int       *bp;
{
	register double powerr, powerm, resultr, resultm, treal;
	register long int n;
	register unsigned a;
	float           f;
	FLOATFUNCTIONTYPE pow_ri();

	n = *bp;
	if (n == 0)
		RETURN(1.0, 0.0);
	powerm = ap->imag;
	if (powerm == 0.0) {
		ASSIGNFLOAT(f, pow_ri(&(ap->real), bp));
		RETURN(f, 0.0);
	}
	powerr = ap->real;
	a = abs(n);
	while ((a & 1) == 0) {
		SQUAREPOWER;
		a = a >> 1;
	}
	resultr = powerr;
	resultm = powerm;
	a = a >> 1;
	while (a != 0) {
		SQUAREPOWER;
		if (a & 1) {	/* result *= power; */
			treal = resultr * powerr - resultm * powerm;
			resultm = resultr * powerm + resultm * powerr;
			resultr = treal;
		}
		a = a >> 1;
	}
	if (n < 0) {		/* result = 1.0 / result; *//* Simple
				 * algorithm since result is float. */
		powerr = resultr * resultr + resultm * resultm;
		resultr = resultr / powerr;
		resultm = -resultm / powerr;
	}
	RETURN(resultr, resultm);
}
