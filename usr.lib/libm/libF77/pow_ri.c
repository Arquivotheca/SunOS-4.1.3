#ifndef lint
static char     sccsid[] = "@(#)pow_ri.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <math.h>

FLOATFUNCTIONTYPE
pow_ri(ap, bp)
	float          *ap;
	long           *bp;
{
	register double x, result;
	register long   iap, n;
	register unsigned a;
	float           w;
	long           *pw = (long *) &w;

	a = n = *bp;
	w = 1.0;
	iap = *((long *) ap);
	iap &= 0x7fffffff;

	if (n != 0) {
		x = (double) *ap;
		if (n < 0) {
			a = -n;
			x = 1.0 / x;
		}
		if (iap >= 0x7f800000 || iap <= 0x00200000 || iap == 0x3f800000)
			if (a > 7)
				a = 0x4 + (a & 3);
		/* if (!finite(x)||x==0.0||x==1.0) a = 0x4+(a&3); */
		if (a > 1100) {
			w = exp(a * log(fabs(x)));
			if (x < 0.0 && (n & 1) == 1)
				w = -w;
			RETURNFLOAT(w);
		}
		result = 1.0;
loop:
		if ((a & 1) != 0)
			result *= x;
		a >>= 1;
		if (a != 0) {
			x *= x;
			goto loop;
		}
		w = (float) result;
	}
	RETURNFLOAT(w);
}
