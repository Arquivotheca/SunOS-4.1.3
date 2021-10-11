#ifndef lint
static char     sccsid[] = "@(#)fabs.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

double
fabs(x)
	double          x;
{
	long           *px = (long *) &x;
#ifdef i386
	px[1] &= 0x7fffffff;
#else
	px[0] &= 0x7fffffff;
#endif
	return x;
}
