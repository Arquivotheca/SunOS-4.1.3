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
static char sccsid[] = "@(#)segments.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#define NULL 0

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: delete_retained_segment                                    */
/*                                                                          */
/*     PURPOSE: THE RETAINED SEGMENT 'segname' IS DELETED.                  */
/*                                                                          */
/****************************************************************************/

delete_retained_segment(segname) int segname;
{
   char *funcname;
   register int index;
   register segstruc *segptr;
   ddargtype ddstruct;
   viewsurf *vwsrf;

   funcname = "delete_retained_segment";
   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
      if((segptr->type >= RETAIN) && (segname == segptr->segname)) {
         _core_critflag++;
	 if (segptr == _core_openseg) {
	    close_retained_segment();		/* close the open segment */
	    }
	 if(segptr->segats.visbilty) {
	    for (index = 0; index < segptr->vsurfnum; index++) {
		vwsrf = segptr->vsurfptr[index];
	        if (vwsrf->erasure) {
		    if (vwsrf->hphardwr) {
			ddstruct.opcode = SEGDRAW;
			ddstruct.ptr1 = (char*)segptr;
	 		ddstruct.instance = vwsrf->vsurf.instance;
			ddstruct.int1 = TRUE;
			ddstruct.int2 = index;
			(*vwsrf->vsurf.dd)( &ddstruct);
		    } else
			_core_segdraw(segptr,index,TRUE);	/* redraw */
		} else {
			if (_core_batchupd) {		/* new frame later */
				(segptr->vsurfptr[index])->nwframnd = TRUE;
			} else  {
				_core_repaint(FALSE);	/* new frame */
			}
		}
	    }
	 }
	 segptr->type = DELETED;
	 --_core_segnum;

	 _core_PDFcompress(segptr);			/* compress the PDF */
         if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	 return(0);
	 }

      if (segptr->type == EMPTY) {
	 _core_errhand(funcname,14); return(1);
	 }
      }
   _core_errhand(funcname,14); return(1);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: delete_all_retained_segments                               */
/*                                                                          */
/*     PURPOSE: ALL RETAINED AND NON-RETAINED SEGMENTS ARE DELETED.         */
/*                                                                          */
/****************************************************************************/

delete_all_retained_segments()
{
   register int n;
   register viewsurf *surfp;
   register segstruc *segptr;
   ddargtype ddstruct;

   _core_critflag++;
   for(segptr = &_core_segment[0];segptr < &_core_segment[SEGNUM]
   && segptr->type != EMPTY;segptr++)
	if (segptr->type >= RETAIN)
		{
		if(segptr == _core_openseg)
			close_retained_segment();
		segptr->type = DELETED;
		if (segptr->segats.visbilty)
			for (n = 0; n < segptr->vsurfnum; n++)
				segptr->vsurfptr[n]->nwframnd = TRUE;
		}
   for(surfp = &_core_surface[0];surfp < &_core_surface[MAXVSURF];surfp++)
      if(surfp->nwframnd)
	 if (!_core_batchupd)
		{
		ddstruct.instance = surfp->vsurf.instance;
		ddstruct.opcode = CLEAR;
		(*surfp->vsurf.dd)(&ddstruct);
		surfp->nwframnd = FALSE;
		}
   _core_PDFcompress((segstruc *) 0); /* RESET PSEUDO DISPLAY FILE POINTER */
   _core_segnum = 0;		    /* RESET SEGMENT NUMBER COUNTER */
   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: rename_retained_segment                                    */
/*                                                                          */
/*     PURPOSE: EXISTING RETAINED SEGMENT NAMED BY THE PARAMETER 'segname'  */
/*              IS HENCEFORTH KNOWN BY THE NAME SPECIFIED BY THE 'newname'  */
/*              PARAMETER.                                                  */
/*                                                                          */
/****************************************************************************/

rename_retained_segment(segname,newname) int segname, newname;
   {
   char *funcname;
   int errnum;
   register segstruc *segptr;
   segstruc *savptr;

   funcname = "rename_retained_segment";
   savptr = NULL;

   for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++)
      if(segptr->type >= RETAIN)
		{
		if (newname == segptr->segname)
			{
	 		errnum = 36;
	 		_core_errhand(funcname,errnum);
	 		return(1);
	 		}
		if (segname == segptr->segname)
			savptr = segptr;
		}
      else if (segptr->type == EMPTY)
		break;
   if (savptr != NULL)
	{
	savptr->segname = newname;
	return(0);
	}
   errnum = 37;
   _core_errhand(funcname,errnum);
   return(2);
   }

