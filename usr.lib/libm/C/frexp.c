#ifdef sccsid
static	char sccsid[] = "@(#)frexp.c 1.1 92/07/30 SMI"; /* from UCB 4.3 8/21/85 */
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * for non-zero x 
 *	x = frexp(arg,&exp);
 * return a double fp quantity x such that 0.5 <= |x| <1.0
 * and the corresponding binary exponent "exp". That is
 *	arg = x*2^exp.
 * If arg is inf, 0.0, or NaN, then frexp(arg,&exp) returns arg 
 * with *exp=0. 
 */

#include <math.h>

double frexp(value, eptr)
double value; int *eptr;
{
	if(finite(value)&&value!=0.0) *eptr = ilogb(value)+1;
	else *eptr = 0;
	return scalbn(value, -(*eptr));
}
