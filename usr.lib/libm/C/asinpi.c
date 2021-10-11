#ifndef lint
static	char sccsid[] = "@(#)asinpi.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* asinpi(x)
 *
 *	asinpi(x) = asin(x)/pi
 *
 */

static double invpi = 0.3183098861837906715377675;

double asinpi(x)
double x;
{
	double asin(); return asin(x)*invpi;
}
