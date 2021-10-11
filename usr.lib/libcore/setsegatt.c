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
static char sccsid[] = "@(#)setsegatt.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_visibility                                     */
/*                                                                          */
/****************************************************************************/

set_segment_visibility(segname,visbilty) int segname; int visbilty;
{
    char *funcname;
    int errnum,index;
    int previous;
    segstruc *segptr;
    ddargtype ddstruct;
    viewsurf *vwsrf;

    funcname = "set_segment_visibility";
    if (visbilty != TRUE && visbilty != FALSE) {
        errnum = 27;
        _core_errhand(funcname,errnum);
        return(2);
        }
    for (segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
    	    _core_critflag++;
	    if (visbilty != segptr->segats.visbilty) {
	        previous = segptr->redraw;
	        segptr->redraw = TRUE;
	        for (index = 0; index < segptr->vsurfnum; index++) {
		    vwsrf = segptr->vsurfptr[index];
		    if (vwsrf->erasure && !visbilty && !previous) {
		        if (vwsrf->hphardwr) {
			    ddstruct.opcode = SEGDRAW;
			    ddstruct.ptr1 = (char*)segptr;
			    ddstruct.instance = vwsrf->vsurf.instance;
			    ddstruct.int1 = TRUE;
			    ddstruct.int2 = index;
			    (*vwsrf->vsurf.dd)( &ddstruct);
		        } else
			    _core_segdraw(segptr,index,TRUE);
		    } else {
			vwsrf->nwframnd = TRUE; 
			}
		    }
	        segptr->segats.visbilty = visbilty;
	        if (!_core_batchupd) {_core_repaint(FALSE); }
	        }
       	    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	    return(0);
	    }
        if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname,errnum); return(1);
	    }
        }
    errnum = 29;
    _core_errhand(funcname,errnum);
    return(1);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_detectability                                  */
/*                                                                          */
/****************************************************************************/
set_segment_detectability(segname,detectbl) int segname; int detectbl;
{
    char *funcname;
    int errnum;
    segstruc *segptr;

    funcname = "set_segment_detectability";
    if (detectbl < 0 ) {
        errnum = 27; _core_errhand(funcname,errnum);
        return(27);
        }
    for (segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	    segptr->segats.detectbl = detectbl;
	    return(0);
	    }
        if (segptr->type == EMPTY) {
 	    errnum = 29;
	    _core_errhand(funcname,errnum);
	    return(1);
	    }
        }
    errnum = 29;
    _core_errhand(funcname,errnum);
    return(1);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_highlighting                                   */
/*                                                                          */
/****************************************************************************/
set_segment_highlighting(segname,highlght) int segname; int highlght;
{
    char *funcname;
    int errnum,index;
    int previous;
    ddargtype ddstruct;
    segstruc *segptr;
    viewsurf *vwsrf;

    funcname = "set_segment_highlighting";
    if (highlght != TRUE && highlght != FALSE) {
        errnum = 27;
        _core_errhand(funcname,errnum);
        return(2);
        }
    for (segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
    	    _core_critflag++;
	    if ((highlght == TRUE || segptr->segats.highlght == TRUE)) {
	        previous = segptr->redraw;
	        segptr->redraw = TRUE;
	        ddstruct.logical = segname;
	        for (index = 0; index < segptr->vsurfnum; index++) {
		    vwsrf = segptr->vsurfptr[index];
		    if (vwsrf->erasure && !previous){
			    /* raster */
			    if (vwsrf->hphardwr) {
				ddstruct.opcode = SEGDRAW;
				ddstruct.ptr1 = (char*)segptr;
				ddstruct.instance = vwsrf->vsurf.instance;
				ddstruct.int1 = TRUE;
				ddstruct.int2 = index;
				(*vwsrf->vsurf.dd)( &ddstruct);
		    	    } else
			        _core_segdraw(segptr,index,TRUE); /* erase */
						  /* and redraw later  */
		            vwsrf->nwframnd = TRUE;
		    	    /* repaint will undo above */
		    } else if (vwsrf->nwframdv){
			    /* storage tube */
			    if (vwsrf->hphardwr) {
				ddstruct.opcode = SEGDRAW;
				ddstruct.ptr1 = (char*)segptr;
				ddstruct.instance = vwsrf->vsurf.instance;
				ddstruct.int1 = FALSE;
				ddstruct.int2 = index;
				(*vwsrf->vsurf.dd)( &ddstruct);
		    	    } else
			        _core_segdraw(segptr,index,FALSE);
		            /* redraw this segment only, highlight by flash */
		    }
		}
	        segptr->segats.highlght = highlght;
	        if (!_core_batchupd)
			_core_repaint(FALSE);
	        }
       	    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	    return(0);
	    }
	if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname,errnum); return(1);
	    }
        }
    errnum = 29;
    _core_errhand(funcname,errnum);
    return(1);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_image_transformation_2                         */
/*                                                                          */
/****************************************************************************/
set_segment_image_transformation_2(segname,sx,sy,a,tx,ty)
   int segname; float sx,sy,a,tx,ty;
{
    char *funcname;
    int errnum,index;
    int previous;
    ddargtype ddstruct;
    segstruc *segptr;
    viewsurf *vwsrf;

    funcname = "set_segment_image_transformation_2";
    if(sx == 0.0 || sy == 0.0) {
        _core_errhand(funcname,27);
        return(27);
        }
    tx *= (float) MAX_NDC_COORD;
    ty *= (float) MAX_NDC_COORD;
    for (segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	    if (segptr->type < XFORM2) {
	        errnum = 30; _core_errhand(funcname,errnum); return(3);
	        }
    	    _core_critflag++;
	    previous = segptr->redraw;
	    segptr->redraw = TRUE;
	    ddstruct.logical = segname;
	    for (index = 0; index < segptr->vsurfnum; index++) {
		    			/* vector without highlight */
		vwsrf = segptr->vsurfptr[index];
		if (vwsrf->erasure && !previous){ /* raster */
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SEGDRAW;
			ddstruct.ptr1 = (char*)segptr;
	 		ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = TRUE;
			ddstruct.int2 = index;
			(*vwsrf->vsurf.dd)( &ddstruct);
		    } else
			_core_segdraw(segptr,index,TRUE);
			/* erase seg from index VS and redraw later */
		    }		/* new frame later ***/
		vwsrf->nwframnd = TRUE;
		}
	    segptr->segats.rotate[0] = 0.0;
	    segptr->segats.rotate[1] = 0.0;
	    segptr->segats.scale[2] = 1.0;
	    segptr->segats.translat[2] = 0.0;
	    segptr->segats.rotate[2] = a;
	    segptr->segats.scale[0] = sx;
	    segptr->segats.scale[1] = sy;
	    segptr->segats.translat[0] = tx;
	    segptr->segats.translat[1] = ty;
            _core_setmatrix(segptr);

	    /* If segment is still open, send new image xform to DD */
	    if (_core_openseg && segptr->segname == _core_openseg->segname) {
	        for (index = 0; index < segptr->vsurfnum; index++) {
		    vwsrf = segptr->vsurfptr[index];
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SETIMGXFORM;
			ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = segptr->idenflag; /* imxform to DD */
			ddstruct.ptr1 = (char*)(segptr->imxform);
			(*vwsrf->vsurf.dd)( &ddstruct);
		    }
	        }
	    }
	    if (!_core_batchupd)
		    _core_repaint(FALSE);
            if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	    return(0);
	    }
        if (segptr->type == EMPTY) {
	    errnum = 29; _core_errhand(funcname,errnum);
	    return(errnum);
	    }
        }
    errnum = 29; _core_errhand(funcname,errnum);
    return(errnum);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_image_translate_2                              */
/*                                                                          */
/****************************************************************************/
set_segment_image_translate_2(segname,tx,ty) int segname; float tx,ty;
{
    char *funcname;
    int errnum,index;
    int previous;
    ddargtype ddstruct;
    segstruc *segptr;
    viewsurf *vwsrf;

    funcname = "set_segment_image_translate_2";
    tx *= (float) MAX_NDC_COORD;
    ty *= (float) MAX_NDC_COORD;
    for (segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	    if (segptr->type < XLATE2) {
	        errnum = 30; _core_errhand(funcname,errnum); return(3);
	        }
    	    _core_critflag++;
	    previous = segptr->redraw;
	    segptr->redraw = TRUE;
	    ddstruct.logical = segname;
	    for (index = 0; index < segptr->vsurfnum; index++) {
		vwsrf = segptr->vsurfptr[index];
		if (vwsrf->erasure && !previous){
			/* raster */
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SEGDRAW;
			ddstruct.ptr1 = (char*)segptr;
			ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = TRUE;
			ddstruct.int2 = index;
			(*vwsrf->vsurf.dd)( &ddstruct);
		    } else
		        _core_segdraw(segptr,index,TRUE);
			/* erase seg from indxd VS and redraw later */
		    }			/* new frame later */
	        vwsrf->nwframnd = TRUE;
		}
	    segptr->segats.scale[0] = 1.0;
	    segptr->segats.scale[1] = 1.0;
	    segptr->segats.scale[2] = 1.0;
	    segptr->segats.rotate[0] = 0.0;
	    segptr->segats.rotate[1] = 0.0;
	    segptr->segats.rotate[2] = 0.0;
	    segptr->segats.translat[2] = 0.0;
	    segptr->segats.translat[0] = tx;
	    segptr->segats.translat[1] = ty;
            _core_setmatrix(segptr);

	    /* If segment is still open, send new image xform to DD */
	    if (_core_openseg && segptr->segname == _core_openseg->segname) {
	        for (index = 0; index < segptr->vsurfnum; index++) {
		    vwsrf = segptr->vsurfptr[index];
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SETIMGXFORM;
			ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = segptr->idenflag; /* imxform to DD */
			ddstruct.ptr1 = (char*)(segptr->imxform);
			(*vwsrf->vsurf.dd)( &ddstruct);
		    }
	        }
	    }
	    if (!_core_batchupd)
		    _core_repaint(FALSE);
            if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	    return(0);
	    }
        if (segptr->type == EMPTY) {
	    errnum = 29;
	    _core_errhand(funcname,errnum);
	    return(1);
	    }
        }
    errnum = 29;
    _core_errhand(funcname,errnum);
    return(1);
}
