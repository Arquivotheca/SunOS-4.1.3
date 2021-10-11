#ifndef lint
static	char sccsid[] = "@(#)d_sign.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/* if b is negative but not -0 return -|a| */

double d_sign(a,b)
double *a, *b;
{
union { double d ; unsigned i[2] } klugea, klugeb;
double	one = 1.0;
int	id0;

if ((* (int *) &one) != 0) id0 = 0;			/* not a i386 */
else id0 = 1;						/* is a i386 */

klugea.d = *a ;
klugeb.d = *b ;
if ((klugeb.i[id0] > 0x80000000) || ( (klugeb.i[1-id0] != 0) && (klugeb.i[id0] == 0x80000000) ))
	klugea.i[id0] |= 0x80000000; 
else
	klugea.i[id0] &= 0x7fffffff;
return klugea.d;
}
