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
static char sccsid[] = "@(#)zbuffer.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include "colorshader.h"

#define ZBimagelines	480	/* lines per image */
#define	ZBpixels	640	/* pixels per line */
#define INfinitedepth	MAX_NDC_COORD

static short *zbufbuf;
static short *zbufcut;
static short touched[ZBimagelines];

_core_set_ZBuffer_cut(xarr, zarr, n)
    float xarr[], zarr[];
int n;

{
    int i, j, x0, z0, x1, z1, dz;

    for (i = 0; i < n; i++) {
	if (xarr[i] < 0. || xarr[i] > _core_ndc.width
	    || zarr[i] < 0. || zarr[i] > _core_ndc.depth) {
	    _core_errhand("set_zbuffer_cut", 71);
	    return (71);
	}
    }
    x0 = 639. * xarr[0];
    z0 = MAX_NDC_COORD * zarr[0];
    for (i = 1; i < n; i++) {
	x1 = 639. * xarr[i];
	z1 = MAX_NDC_COORD * zarr[i];
	if (x1 <= x0)
	    continue;
	dz = (z1 - z0) / (x1 - x0);
	for (j = x0; j <= x1; j++) {
	    zbufcut[j] = z0;
	    z0 += dz;
	}
	x0 = x1;
	z0 = z1;
    }
    return (0);
}

_core_INit_zbuffer()
{
    register int i;

    zbufbuf = (short *) malloc(ZBimagelines * ZBpixels * 2);	/* entire zbuffer */
    zbufcut = (short *) malloc(ZBpixels * 2);
    for (i = 0; i < ZBimagelines; i++)
	touched[i] = FALSE;
    if (zbufbuf && zbufcut)
	return (0);
    return (1);
}

_core_CLear_zbuffer()
{
    int i;
    short *ptr;

    for (i = 0; i < ZBimagelines; i++)
	touched[i] = FALSE;
    ptr = zbufcut;
    for (i = 0; i < ZBpixels; i++)	/* and no cross-sectioning */
	*ptr++ = 0;
}

_core_TErminate_zbuffer()
{
    free((char *) zbufbuf);
    free((char *) zbufcut);
}
