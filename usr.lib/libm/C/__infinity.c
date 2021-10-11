
#ifndef lint
static	char sccsid[] = "@(#)__infinity.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

double __infinity()
{
	double x,one=1.0;
	long *px = (long*)&x;
	if((*(int*)&one)==0) 
	    {px[0]=0;px[1]=0x7ff00000;}
	else
	    {px[1]=0;px[0]=0x7ff00000;}
	return x;
}
