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
static char sccsid[] = "@(#)credelseg.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#define NULL 0

set_image_transformation_type(type)
    int type;
{
    char *funcname;
    int errnum;

    funcname = "set_segment_image_transformation_type";
    if ((type < RETAIN) || (type > XFORM3)) {
	errnum = 28;
	_core_errhand(funcname, errnum);
	return (errnum);
    }
    _core_csegtype = type;
    return (0);
}

create_retained_segment(segname)
    int segname;
{
    char *funcname;
    int errnum;
    register int index;
    int anyhphardwr, allhphardwr;
    viewsurf *surfp;
    short ptype;
    segstruc *segptr;
    ddargtype ddstruct;

    funcname = "create_retained_segment";
    if (!segname && _core_csegtype > NORETAIN) {
	_core_errhand(funcname, 29);
	return (29);
    }
    if (_core_osexists) {
	_core_errhand(funcname, 8);
	return (8);
    }
    /* if we get here, _core_openseg == NULL */
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if (segptr->type == EMPTY) {
	    if (_core_openseg == NULL)
		_core_openseg = segptr;
	    break;
	} else if (segptr->type == DELETED) {
	    if (_core_openseg == NULL)
		_core_openseg = segptr;
	} else if (segname == segptr->segname) {
	    _core_errhand(funcname, 31);
	    _core_openseg = NULL;
	    return (31);
	}
    }
    if (_core_openseg == NULL) {
	_core_errhand(funcname, 16);
	return (16);
    }
    if (_core_vtchang)
	if (_core_validvt(TRUE)) {	/*** valid viewing transformation? ***/
	    _core_errhand(funcname, 34);
	    _core_openseg = NULL;
	    return (34);
	}
    _core_critflag++;
    anyhphardwr = FALSE;
    allhphardwr = TRUE;
    errnum = 33;		/* assume error */
    _core_openseg->vsurfnum = 0;/* SET NO. RELATED SURFACES TO SEGMENT = 0 */
    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++) {
	if (surfp->selected) {
	    errnum = 0;		/* no error because one is selected */
	    /* PLACE POINTERS TO RELATED SURFACES IN OPEN SEGMENT STRUCTURE.   */
	    _core_openseg->vsurfptr[(_core_openseg->vsurfnum)++] = surfp;
	    if (surfp->hphardwr)
		anyhphardwr = TRUE;
	    else
		allhphardwr = FALSE;
	}
    }
    if (errnum == 33) {		/* never found one selected */
	_core_errhand(funcname, errnum);
	_core_openseg = NULL;
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
	return (errnum);
    }
    _core_fastflag = allhphardwr && (_core_csegtype == NORETAIN);
    _core_ndcset |= 1;
    _core_openseg->type = _core_csegtype;

    if (_core_csegtype >= RETAIN) {
	_core_openseg->segname = segname;
	_core_openseg->pdfptr = _core_pdfwrite(0, &ptype);	/* next pdf slot */
	_core_openseg->segsize = 0;
	ptype = PDFVWPORT;	/* put viewport in PDF */
	(void) _core_pdfwrite(SHORT, &ptype);
	(void) _core_pdfwrite(FLOAT * 6, (short *) &_core_vwstate.viewport);

	/*
	 * SET SEGMENT DYNAMIC ATTRIBUTES WHICH APPLY TO ALL RETAINED
	 * SEGMENTS 
	 */
	_core_openseg->segats.visbilty = _core_defsegat.visbilty;
	_core_openseg->segats.detectbl = _core_defsegat.detectbl;
	_core_openseg->segats.highlght = _core_defsegat.highlght;

	/*
	 * SET INITIAL BOUNDING BOX FOR ALL RETAINED SEGMENTS 
	 */
	_core_openseg->bndbox_min.x = MAX_NDC_COORD;
	_core_openseg->bndbox_min.y = MAX_NDC_COORD;
	_core_openseg->bndbox_min.z = MAX_NDC_COORD;
	_core_openseg->bndbox_max.x = -MAX_NDC_COORD;
	_core_openseg->bndbox_max.y = -MAX_NDC_COORD;
	_core_openseg->bndbox_max.z = -MAX_NDC_COORD;
    }
    if (_core_csegtype > RETAIN) {

	/*
	 * SET SEGMENT DYNAMIC ATTRIBUTES WHICH APPLY TO XFORM OR XLATE
	 * RETAINED SEGMENTS 
	 */
	_core_openseg->segats.scale[0] = _core_defsegat.scale[0];
	_core_openseg->segats.scale[1] = _core_defsegat.scale[1];
	_core_openseg->segats.scale[2] = _core_defsegat.scale[2];
	_core_openseg->segats.translat[0] = _core_defsegat.translat[0];
	_core_openseg->segats.translat[1] = _core_defsegat.translat[1];
	_core_openseg->segats.translat[2] = _core_defsegat.translat[2];
	_core_openseg->segats.rotate[0] = _core_defsegat.rotate[0];
	_core_openseg->segats.rotate[1] = _core_defsegat.rotate[1];
	_core_openseg->segats.rotate[2] = _core_defsegat.rotate[2];

	/*
	 * CONSTRUCT IMAGE TRANSFORMATION MATRIX 
	 */
	_core_setmatrix(_core_openseg);
    } else
	_core_openseg->idenflag = TRUE;


    /*
     * SETUP SEGMENT STATE VARIABLES 
     */
    _core_prevseg = TRUE;
    _core_osexists = TRUE;
    _core_segnum++;


    /*
     * For high perf view surfaces, send the necessary matrices,etc. to the
     * DDs 
     */
    if (anyhphardwr) {
	for (index = 0; index < _core_openseg->vsurfnum; index++) {
	    surfp = _core_openseg->vsurfptr[index];
	    if (surfp->hphardwr) {
		ddstruct.instance = surfp->vsurf.instance;
		ddstruct.opcode = SETVIEWPORT;
		ddstruct.ptr1 = (char *) (&_core_vwstate.viewport);
		(*surfp->vsurf.dd) (&ddstruct);
		ddstruct.opcode = SETIMGXFORM;
		ddstruct.int1 = _core_openseg->idenflag;
		ddstruct.ptr1 = (char *) (_core_openseg->imxform);
		(*surfp->vsurf.dd) (&ddstruct);
		if (_core_fastflag && (&surfp->vsurf != _core_xformvs))
		    _core_update_hphardwr(&surfp->vsurf);

		/*
		 * _core_validvt already updated _core_xformvs 
		 */
	    }
	}
    }

    /*
     * ADJUST OUTPUT PRIMITIVE FLAGS TO INITIAL OPEN SEGMENT VALUES 
     */

    _core_linecflag = TRUE;
    _core_fillcflag = TRUE;
    _core_textcflag = TRUE;
    _core_lsflag = TRUE;
    _core_pisflag = TRUE;
    _core_pesflag = TRUE;
    _core_lwflag = TRUE;
    _core_penflag = TRUE;
    _core_fntflag = TRUE;
    _core_upflag = TRUE;
    _core_pathflag = TRUE;
    _core_spaceflag = TRUE;
    _core_justflag = TRUE;
    _core_qualflag = TRUE;
    _core_markflag = TRUE;
    _core_ropflag = TRUE;
    _core_linatt = FALSE;
    _core_polatt = FALSE;
    _core_texatt = FALSE;
    _core_rasatt = FALSE;


    /* Performance Hack Variables */
    if (_core_fastflag) {
	_core_set_fastwidth();
	_core_set_fastpoly();
    }
    _core_vtxnmlcnt = 0;


    /*
     * SET CURRENT POSITION 
     */

    move_rel_3(0.0, 0.0, 0.0);

    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    return (0);
}


create_temporary_segment()
{
    int i;

    i = _core_csegtype;
    _core_csegtype = NORETAIN;
    (void) create_retained_segment(0);
    _core_segnum--;
    _core_csegtype = i;
    return (0);
}

close_temporary_segment()
{
    (void) close_retained_segment();
    return (0);
}

close_retained_segment()
{
    char *funcname;
    int errnum;
    short ptype;
    int pdfnxt;

    funcname = "close_retained_segment";
    if (!_core_osexists) {
	errnum = 26;
	_core_errhand(funcname, errnum);
	return (errnum);
    }
    _core_critflag++;
    if (_core_openseg->type == NORETAIN) {
	_core_openseg->type = DELETED;
    }

    /*
     * PLACE END OF SEGMENT MARKER IN PDF 
     */
    if (_core_openseg->type >= RETAIN) {
	ptype = PDFENDSEGMENT;
	pdfnxt = _core_pdfwrite(SHORT, &ptype);
	_core_openseg->segsize = pdfnxt - _core_openseg->pdfptr;
    }
    _core_osexists = FALSE;
    _core_openseg = NULL;
    _core_fastflag = FALSE;
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    return (0);
}


_core_validvt(vwflag)
    int vwflag;
{				/* check viewing parameters to ensure valid
				 * transform */
    /* and compose with world transform */
    double determ;
    double dummy[4][2];
    viewsurf *surfp;
    int i;

    if (vwflag) {
	if (_core_make_mat())
	    return (2);
	_core_vtchang = FALSE;
    } else {
	_core_pop(&_core_compxfrm32k[0][0]);
	_core_push(&_core_vwxform1[0][0]);
    }
    _core_push(&_core_modxform[0][0]);
    _core_matcon();
    _core_pop(&_core_compxfrm1[0][0]);
    _core_push(&_core_vwxform32k[0][0]);
    _core_push(&_core_modxform[0][0]);
    _core_matcon();
    _core_copytop(&_core_compxfrm32k[0][0]);
    _core_moveword((short *)_core_compxfrm32k, (short *)_core_invwxform, 32);
    _core_matinv(_core_invwxform, 4, dummy, 0, &determ);
    if (determ == 0.0)
	_core_compxfrm_invert = FALSE;
    else
	_core_compxfrm_invert = TRUE;
    if (_core_xformvs)
	_core_update_hphardwr(_core_xformvs);
    if (_core_fastflag)
	for (i = 0; i < _core_openseg->vsurfnum; i++) {
	    surfp = _core_openseg->vsurfptr[i];
	    if (&surfp->vsurf != _core_xformvs)
		_core_update_hphardwr(&surfp->vsurf);
	}
    return (0);
}


_core_update_segbndbox(segptr, point)
    segstruc *segptr;
    ipt_type *point;
{
    if (point->x < segptr->bndbox_min.x)
	segptr->bndbox_min.x = point->x;
    if (point->y < segptr->bndbox_min.y)
	segptr->bndbox_min.y = point->y;
    if (point->z < segptr->bndbox_min.z)
	segptr->bndbox_min.z = point->z;

    if (point->x > segptr->bndbox_max.x)
	segptr->bndbox_max.x = point->x;
    if (point->y > segptr->bndbox_max.y)
	segptr->bndbox_max.y = point->y;
    if (point->z > segptr->bndbox_max.z)
	segptr->bndbox_max.z = point->z;
}


_core_update_hphardwr(vwsurf)
    register struct vwsurf *vwsurf;
{
    ddargtype ddstruct;

    _core_critflag++;
    ddstruct.instance = vwsurf->instance;
    ddstruct.opcode = SETVIEWPORT;
    ddstruct.ptr1 = (char *) (&_core_vwstate.viewport);
    (*vwsurf->dd) (&ddstruct);
    ddstruct.opcode = WINDOWCLIP;
    ddstruct.int1 = _core_wclipplanes;
    (*vwsurf->dd) (&ddstruct);
    ddstruct.opcode = SETVWXFORM32K;
    ddstruct.ptr1 = (char *) &_core_TStack[_core_TSp][0];
    (*vwsurf->dd) (&ddstruct);
    ddstruct.opcode = SETVWXFORM1;
    ddstruct.ptr1 = (char *) _core_compxfrm1;
    (*vwsurf->dd) (&ddstruct);
    if (vwsurf == _core_xformvs)
	_core_ddxformset = TRUE;
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
}


_core_set_fastwidth()
{
    int i;
    float f;
    viewsurf *surfp;
    ddargtype ddstruct;

    /*
     * Only call this routine if _core_fastflag is TRUE, which implies a
     * temporary segment is open 
     */
    _core_fastwidth = TRUE;
    for (i = 0; i < _core_openseg->vsurfnum; i++) {
	surfp = _core_openseg->vsurfptr[i];
	ddstruct.instance = surfp->vsurf.instance;
	ddstruct.opcode = SETWIDTH;
	f = (float) ((_core_ndcspace[0] < _core_ndcspace[1]) ?
		     _core_ndcspace[0] : _core_ndcspace[1]);
	ddstruct.int1 = _core_current.linwidth * f / 100.;
	(*surfp->vsurf.dd) (&ddstruct);
    }
}

_core_set_fastpoly()
{
    int i;
    viewsurf *surfp;
    ddargtype ddstruct;

    /*
     * Only call this routine if _core_fastflag is TRUE, which implies a
     * temporary segment is open 
     */
    _core_fastpoly = TRUE;
    for (i = 0; i < _core_openseg->vsurfnum; i++) {
	surfp = _core_openseg->vsurfptr[i];
	if (surfp->hiddenon) {
	    ddstruct.instance = surfp->vsurf.instance;
	    ddstruct.opcode = CHKHWZBUF;
	    (*surfp->vsurf.dd) (&ddstruct);
	    if (ddstruct.logical == FALSE)
	        _core_fastpoly = FALSE; /* No HW Z buffer */
	}
    }
}
