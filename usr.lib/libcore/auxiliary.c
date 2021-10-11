/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)auxiliary.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <stdio.h>

static float cidmatrix[4][4] = {1., 0., 0., 0.,
				0., 1., 0., 0.,
				0., 0., 1., 0.,
				0., 0., 0., 1.};

/*
 * Set a 4x4 array to the identity matrix.
 */
_core_identity(arrayptr)
    register float *arrayptr;
{
    register float *temptr;
    register short i;

    temptr = &cidmatrix[0][0];
    for (i = 0; i < 16; i++)
	*arrayptr++ = *temptr++;
}

/*
 * Multiply a point by a scalar.
 */
_core_scalept(s, pt)
    float s;
    register pt_type *pt;
{
    pt->x *= s;
    pt->y *= s;
    pt->z *= s;
}

/*
 * Integer square root.
 */
_core_jsqrt(n)
    register unsigned n;
{
    register unsigned q, r, x2, x;
    unsigned t;

    if (n > 0xFFFE0000)
	return 0xFFFF;		/* algorithm doesn't cover this case */
    if (n == 0xFFFE0000)
	return 0xFFFE;		/* or this case */
    if (n < 2)
	return n;		/* or this case */
    t = x = n;
    while (t >>= 2)
	x >>= 1;
    x++;
    for (;;) {
	q = n / x;
	r = n % x;
	if (x <= q) {
	    x2 = x + 2;
	    if (q < x2 || q == x2 && r == 0)
		break;
	}
	x = (x + q) >> 1;
    }
    return x;
}
