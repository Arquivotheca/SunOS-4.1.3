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
static char sccsid[] = "@(#)setmatrix.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

double sin(), cos();

/*
 * Build image transformation matrix from view state variables.
 *
 * This transform scales; rotates (in x, y, z order) about point 0,0,0;
 * then translates
 */
_core_setmatrix(segptr)
    segstruc *segptr;

{
    register float *pf;
    register segstruc *asegptr;
    float xform[4][4];
    int i, j, k;

    asegptr = segptr;
    if (identchk(asegptr)) {
	asegptr->idenflag = TRUE;
	return (0);
    }
    asegptr->idenflag = FALSE;

    _core_identity(&xform[0][0]);
    if (asegptr->type >= XLATE2) {
	xform[3][0] = asegptr->segats.translat[0];	/* translate	 */
	xform[3][1] = asegptr->segats.translat[1];
	xform[3][2] = asegptr->segats.translat[2];
    }
    if (asegptr->type == XFORM2) {
	_core_push(&xform[0][0]);
	if (asegptr->segats.rotate[2] != 0.) {
	    _core_identity(&xform[0][0]);	/* rotate about z */
	    xform[0][0] = xform[1][1] = cos(asegptr->segats.rotate[2]);
	    xform[0][1] = sin(asegptr->segats.rotate[2]);
	    xform[1][0] = -xform[0][1];
	    _core_push(&xform[0][0]);
	    (void)_core_matcon();
	}
	if (asegptr->segats.rotate[1] != 0.) {
	    _core_identity(&xform[0][0]);	/* rotate about y */
	    xform[0][0] = xform[2][2] = cos(asegptr->segats.rotate[1]);
	    xform[2][0] = sin(asegptr->segats.rotate[1]);
	    xform[0][2] = -xform[2][0];
	    _core_push(&xform[0][0]);
	    (void)_core_matcon();
	}
	if (asegptr->segats.rotate[0] != 0.) {
	    _core_identity(&xform[0][0]);	/* rotate about x */
	    xform[1][1] = xform[2][2] = cos(asegptr->segats.rotate[0]);
	    xform[1][2] = sin(asegptr->segats.rotate[0]);
	    xform[2][1] = -xform[1][2];
	    _core_push(&xform[0][0]);
	    (void)_core_matcon();
	}
	if (asegptr->segats.scale[0] != 1. ||
	    asegptr->segats.scale[1] != 1. ||
	    asegptr->segats.scale[2] != 1.) {
	    _core_identity(&xform[0][0]);	/* scale  */
	    xform[0][0] = asegptr->segats.scale[0];
	    xform[1][1] = asegptr->segats.scale[1];
	    xform[2][2] = asegptr->segats.scale[2];
	    _core_push(&xform[0][0]);
	    (void)_core_matcon();
	}
	_core_pop(&xform[0][0]);
    }
    /* convert xform to integer imxform  */
    /* don't bother with last column     */
    /* since it's never used	     */
    pf = (float *) xform;
    for (i = 0; i < 3; i++) {	/* scale and rotate coefficients     */
	for (j = 0; j < 3; j++) {	/* have MSW as signed 15 bit  */
	    k = (int) (*pf);	/* integer part and LSW as signed 15 */
	    *pf -= (float) k;	/* bit fractional part ( 1= 2 to the */
	    *pf *= 32767.0;	/* -15 power, or 1/32768 ).          */
	    asegptr->imxform[i][j] = (k << 16) | (_core_roundf(pf) & 0xFFFF);
	    pf++;
	}
	pf++;
    }

    /* translate coeffs are signed int.  */
    asegptr->imxform[3][0] = _core_roundf(pf);
    pf++;
    asegptr->imxform[3][1] = _core_roundf(pf);
    pf++;
    asegptr->imxform[3][2] = _core_roundf(pf);

    return (0);
}

static float idxfrmparams[] = {1., 1., 1., 0., 0., 0., 0., 0., 0.};

/*
 * Check for identity matrix
 */
static int 
identchk(asegptr)
    register segstruc *asegptr;
{
    register float *ptr1, *ptr2;
    register short i;

    ptr1 = idxfrmparams;
    ptr2 = &asegptr->segats.scale[0];
    for (i = 0; i < 9; i++)
	if (*ptr1++ != *ptr2++)
	    return (0);
    return (1);
}
