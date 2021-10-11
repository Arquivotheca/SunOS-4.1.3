#ifndef lint
static char     sccsid[] = "@(#)pow_zi.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include "complex.h"

#define RETURN(R,M) { p->dreal = R ; p->dimag = M ; return ; }

#define SQUAREPOWER \
	{ treal = (powerr + powerm) * (powerr - powerm) ; \
	powerm = 2.0 * powerr * powerm ; \
	powerr = treal ; }

pow_zi(p, ap, bp)		/* p = a**b  */
	dcomplex       *p, *ap;
	long int       *bp;
{
	register double powerr, powerm, resultr, resultm, treal;
	register long int n;
	register unsigned a;
	double          pow_di();

	n = *bp;
	if (n == 0)
		RETURN(1.0, 0.0);
	if (n == 1)		/* Return *ap but catch signaling NaNs. */
		RETURN(1.0 * ap->dreal, 1.0 * ap->dimag);
	powerm = ap->dimag;
	if (powerm == 0.0)
		RETURN(pow_di(&(ap->dreal), bp), 0.0);
	powerr = ap->dreal;
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
	if (n < 0)		/* result = 1.0 / result; 	do complex
				 * division via smith algorithm */
		if (fabs(resultr) >= fabs(resultm)) {	/* larger real part */
			powerr = resultm / resultr;
			powerm = resultr + resultm * powerr;
			resultr = 1.0 / powerm;
			resultm = -powerr / powerm;
		} else {	/* larger imaginary part */
			powerr = resultr / resultm;
			powerm = resultm + resultr * powerr;
			resultr = powerr / powerm;
			resultm = -1.0 / powerm;
		}
	RETURN(resultr, resultm);
}
