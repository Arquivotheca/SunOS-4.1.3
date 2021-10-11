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
static char sccsid[] = "@(#)segatt3.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_segment_image_transformation_3                         */
/*                                                                          */
/****************************************************************************/
set_segment_image_transformation_3(segname,sx,sy,sz,rx,ry,rz,tx,ty,tz)
   int segname; float sx,sy,sz,rx,ry,rz,tx,ty,tz;
{
   char *funcname;
   int errnum,index;
   int previous;
   ddargtype ddstruct;
   segstruc *segptr;
   viewsurf *vwsrf;

   funcname = "set_segment_image_transformation_3";
   if(sx == 0 || sy == 0 || sz == 0) {
      _core_errhand(funcname,27);
      return(2);
      }
   tx *= (float) MAX_NDC_COORD;
   ty *= (float) MAX_NDC_COORD;
   tz *= (float) MAX_NDC_COORD;
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
        if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	    if (segptr->type < XFORM2) {
	        _core_errhand(funcname,30); return(3);
	        }
   	    _core_critflag++;
	    previous = segptr->redraw;
	    segptr->redraw = TRUE;
	    ddstruct.logical = segname;
	    for (index = 0; index < segptr->vsurfnum; index++) {
		   			/* vector without highlight */
		vwsrf = segptr->vsurfptr[index];
	    	if (vwsrf->erasure && !previous) {
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SEGDRAW;
			ddstruct.ptr1 = (char*)segptr;
	 		ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = TRUE;
			ddstruct.int2 = index;
			(*vwsrf->vsurf.dd)( &ddstruct);
		    } else
			_core_segdraw(segptr,index,TRUE);	/* raster */
		   	/* erase segment from indexed VS and redraw later */
			}
	    	vwsrf->nwframnd = TRUE;
	    	}
	    segptr->segats.rotate[0] = rx;
	    segptr->segats.rotate[1] = ry;
	    segptr->segats.rotate[2] = rz;
	    segptr->segats.scale[0] = sx;
	    segptr->segats.scale[1] = sy;
	    segptr->segats.scale[2] = sz;
	    segptr->segats.translat[0] = tx;
	    segptr->segats.translat[1] = ty;
	    segptr->segats.translat[2] = tz;
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
/*     FUNCTION: set_segment_image_translate_3                              */
/*                                                                          */
/****************************************************************************/
set_segment_image_translate_3(segname,dx,dy,dz)
   int segname; float dx,dy,dz;
{
   char *funcname;
   int errnum,index;
   int previous;
   ddargtype ddstruct;
   segstruc *segptr;
   viewsurf *vwsrf;

   funcname = "set_segment_image_translate_3";
   dx *= (float) MAX_NDC_COORD;
   dy *= (float) MAX_NDC_COORD;
   dz *= (float) MAX_NDC_COORD;
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	 if (segptr->type < XLATE2) {
	    _core_errhand(funcname,30); return(3);
	    }
         _core_critflag++;
	 previous = segptr->redraw;
	 segptr->redraw = TRUE;
	 ddstruct.logical = segname;
	 for (index = 0; index < segptr->vsurfnum; index++) {
					/* vector without highlight */
	    vwsrf = segptr->vsurfptr[index];
	    if (vwsrf->erasure && !previous) {
		if (vwsrf->hphardwr) {
		    ddstruct.opcode = SEGDRAW;
		    ddstruct.ptr1 = (char*)segptr;
		    ddstruct.instance = vwsrf->vsurf.instance;
		    ddstruct.int1 = TRUE;
		    ddstruct.int2 = index;
		    (*vwsrf->vsurf.dd)( &ddstruct);
		} else
		    _core_segdraw(segptr,index,TRUE);
		   /* erase segment from indexed VS and redraw later      */
		}
	    vwsrf->nwframnd = TRUE;
	    }
	 segptr->segats.scale[0] = 1.0;
	 segptr->segats.scale[1] = 1.0;
	 segptr->segats.scale[2] = 1.0;
	 segptr->segats.rotate[0] = 0.0;
	 segptr->segats.rotate[1] = 0.0;
	 segptr->segats.rotate[2] = 0.0;
	 segptr->segats.translat[0] = dx;
	 segptr->segats.translat[1] = dy;
	 segptr->segats.translat[2] = dz;
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
/*     FUNCTION: inquire_segment_image_transformation_3                     */
/*                                                                          */
/****************************************************************************/

inquire_segment_image_transformation_3(segname,sx,sy,sz,rx,ry,rz,tx,ty,tz)
   int segname; float *sx,*sy,*sz,*rx,*ry,*rz,*tx,*ty,*tz;
{
   char *funcname;
   segstruc *segptr;

   funcname = "inquire_segment_image_transformation_3";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	 if(segptr->type != XFORM2) {
	    _core_errhand(funcname,30);
	    return(2);
	    }
	 *rx = segptr->segats.rotate[0];
	 *ry = segptr->segats.rotate[1];
	 *rz = segptr->segats.rotate[2];
	 *sx = segptr->segats.scale[0];
	 *sy = segptr->segats.scale[1];
	 *sz = segptr->segats.scale[2];
	 *tx = segptr->segats.translat[0]/(float) MAX_NDC_COORD;
	 *ty = segptr->segats.translat[1]/(float) MAX_NDC_COORD;
	 *tz = segptr->segats.translat[2]/(float) MAX_NDC_COORD;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 _core_errhand(funcname,29);
	 return(1);
	 }
      }
   _core_errhand(funcname,29);
   return(1);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_segment_image_translate_3                          */
/*                                                                          */
/****************************************************************************/

inquire_segment_image_translate_3(segname,tx,ty,tz)
   int segname; float *tx,*ty,*tz;
{
   char *funcname;
   segstruc *segptr;

   funcname = "inquire_segment_image_translate_3";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
	 if(segptr->type == RETAIN) {
	    _core_errhand(funcname,30);
	    return(2);
	    }
	 *tx = segptr->segats.translat[0]/(float) MAX_NDC_COORD;
	 *ty = segptr->segats.translat[1]/(float) MAX_NDC_COORD;
	 *tz = segptr->segats.translat[2]/(float) MAX_NDC_COORD;
	 return(0);
	 }
      if (segptr->type == EMPTY) {
	 _core_errhand(funcname,29);
	 return(1);
	 }
      }
   _core_errhand(funcname,29);
   return(1);
   }


