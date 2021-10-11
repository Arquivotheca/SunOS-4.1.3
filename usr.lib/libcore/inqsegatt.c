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
static char sccsid[] = "@(#)inqsegatt.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/*
 * Inquire segment detectability
 */
inquire_segment_detectability(segname, detectbl)
    int segname;
    int *detectbl;
{
    char *funcname;
    int errnum;
    segstruc *segptr;

    funcname = "inquire_segment_detectability";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    *detectbl = segptr->segats.detectbl;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname, errnum);
	    return (errnum);
	}
    }
    errnum = 29;
    _core_errhand(funcname, errnum);
    return (errnum);
}

/*
 * Inquire segment highlighting
 */
inquire_segment_highlighting(segname, highlght)
    int segname;
    int *highlght;
{
    char *funcname;
    int errnum;
    segstruc *segptr;

    funcname = "inquire_segment_highlighting";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    *highlght = segptr->segats.highlght;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname, errnum);
	    return (errnum);
	}
    }
    errnum = 29;
    _core_errhand(funcname, errnum);
    return (errnum);
}

/*
 * Inquire image transformation type
 */
inquire_image_transformation_type(segtype)
    int *segtype;
{
    *segtype = _core_csegtype;
    return (0);
}

/*
 * Inquire segment image transformation type.
 */
inquire_segment_image_transformation_type(segname, segtype)
    int segname;
    int *segtype;
{
    char *funcname;
    int errnum;
    segstruc *segptr;

    funcname = "inquire_segment_image_transformation_type";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    *segtype = segptr->type;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname, errnum);
	    return (errnum);
	}
    }
    errnum = 29;
    _core_errhand(funcname, errnum);
    return (errnum);
}

/*
 * Inquire segment visibility
 */
inquire_segment_visibility(segname, visbilty)
    int segname;
    int *visbilty;
{
    char *funcname;
    int errnum;
    segstruc *segptr;

    funcname = "inquire_segment_visibility";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    *visbilty = segptr->segats.visbilty;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname, errnum);
	    return (1);
	}
    }
    errnum = 29;
    _core_errhand(funcname, errnum);
    return (1);
}

/*
 * Inquire segment image transformation.
 */
inquire_segment_image_transformation_2(segname, sx, sy, a, tx, ty)
    int segname;
    float *sx, *sy, *a, *tx, *ty;
{
    char *funcname;
    segstruc *segptr;

    funcname = "inquire_segment_image_transformation_2";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    if (segptr->type < XLATE2) {
		_core_errhand(funcname, 30);
		return (2);
	    }
	    *a = segptr->segats.rotate[2];
	    *sx = segptr->segats.scale[0];
	    *sy = segptr->segats.scale[1];
	    *tx = segptr->segats.translat[0] / (float) MAX_NDC_COORD;
	    *ty = segptr->segats.translat[1] / (float) MAX_NDC_COORD;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    _core_errhand(funcname, 29);
	    return (1);
	}
    }
    _core_errhand(funcname, 29);
    return (1);
}

/*
 * Inquire segment image translation
 */
inquire_segment_image_translate_2(segname, tx, ty)
    int segname;
    float *tx, *ty;
{
    char *funcname;
    segstruc *segptr;

    funcname = "inquire_segment_image_translate_2";
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if ((segptr->type != DELETED) && (segname == segptr->segname)) {
	    if (segptr->type == NORETAIN || segptr->type == RETAIN) {
		_core_errhand(funcname, 30);
		return (2);
	    }
	    *tx = segptr->segats.translat[0] / (float) MAX_NDC_COORD;
	    *ty = segptr->segats.translat[1] / (float) MAX_NDC_COORD;
	    return (0);
	}
	if (segptr->type == EMPTY) {
	    _core_errhand(funcname, 29);
	    return (1);
	}
    }
    _core_errhand(funcname, 29);
    return (1);
}
