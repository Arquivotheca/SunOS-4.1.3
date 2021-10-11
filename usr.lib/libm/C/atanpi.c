#ifndef lint
static	char sccsid[] = "@(#)atanpi.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* atanpi(x)
 *
 *	atanpi(x) = atan(x)/pi
 *
 */

static double invpi = 0.3183098861837906715377675;

double atanpi(x)
double x;
{
	double atan(); return atan(x)*invpi;
}
