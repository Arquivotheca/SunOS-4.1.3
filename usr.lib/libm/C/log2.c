
#ifdef sccsid
static	char sccsid[] = "@(#)log2.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * LOG2(x)
 * RETURN THE BASE 2 LOGARITHM OF X
 *
 * Method:
 *	purge off 0,INF, and NaN.
 *	n = ilogb(x)
 *	if(n<0) n+=1
 *	z = scalbn(x,-n)
 *	LOG2(x) = n + 1.4426950408889634074*log(x) 
 *
 * where the constant 1.442.. is the inverse of ln2 rounded 
 */

#include <math.h>

double log2(x)
double x;
{
	int n;
	if(x==0.0||!finite(x)) return log(x);
	n = ilogb(x);
	if(n<0) n+=1;
	x = scalbn(x,-n); if(x==0.5) return n-1.0;
	return n+1.44269504088896340736*log(x);
}
