#ifndef lint
static	char sccsid[] = "@(#)_Q_neg.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"

QUAD 
_Q_neg(x)
	QUAD x;
{
	QUAD z;
	int	*pz = (int*) &z;
	double	dummy = 1.0;
	z = x;
	if((*(int*)&dummy)!=0) {
	    pz[0] ^= 0x80000000; 
	} else {
	    pz[3] ^= 0x80000000;
	}
	return z;
}
