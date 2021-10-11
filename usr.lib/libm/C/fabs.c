
#ifndef lint
static	char sccsid[] = "@(#)fabs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

double fabs(x)
double x;
{	
double	one	= 1.0;

	long *px = (long *) &x;

	if ((* (int *) &one) != 0) { px[0] &= 0x7fffffff; }  /* not a i386 */
	else { px[1] &=  0x7fffffff; }			     /* is a i386  */

	return x;
}
