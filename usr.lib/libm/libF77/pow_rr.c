#ifndef lint
static	char sccsid[] = "@(#)pow_rr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <math.h>

FLOATFUNCTIONTYPE 
pow_rr(ap, bp)
float *ap, *bp;
{
return r_pow_(ap, bp);
}
