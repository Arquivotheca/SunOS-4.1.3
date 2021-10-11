
#ifndef lint
static	char sccsid[] = "@(#)compound.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* compound(r,n)
 *
 * Returns (1+r)**n; undefined for r <= -1.
 */

#include <math.h>
double compound(r,n)
double r,n;
{
	double w;
	if(n==0.0) return 1.0;
	if(r!=r) return r+r;
	w = fabs(r);
	if(r<-1.0||fabs(r)<=0.5) 
		return exp(n*log1p(r));
	else
		return pow(1.0+r,n); 
}
