
#ifndef lint
static char     sccsid[] = "@(#)pow_di.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <math.h>

double pow_di(ap, bp)
double *ap; long *bp;
{
	register double x, result;
	register long n;
	register unsigned a;

	a = n = *bp; result=1.0;
	if (n == 0) return result;

	x = *ap;
	if (n < 0) {a= -n; x = 1.0/x;}
	if (!finite(x)||x==0.0||fabs(x)==1.0) a = 0x4+(a&3);
	if (a > 1100) return pow(*ap,(double)n);
    loop:
	if((a&1)!=0) result *= x;
	a >>= 1;
	if(a!=0) {x *= x; goto loop;}

	return result;
}
