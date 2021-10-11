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
static char sccsid[] = "@(#)repaint.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
_core_repaint(explicit)
int explicit;
{
    int vsindex,segindex,flag;
    ddargtype ddstruct;
    viewsurf *vwsrf;

   _core_critflag++;

    for (vsindex = 0; vsindex < MAXVSURF; vsindex++) {
	 if (_core_surface[vsindex].nwframnd
	 && (_core_surface[vsindex].nwframdv || explicit)) {
	     ddstruct.instance = _core_surface[vsindex].vsurf.instance;
	     ddstruct.opcode = CLEAR;
	     (*(_core_surface[vsindex].vsurf.dd))(&ddstruct);
	 }
    }
    for (segindex = 0; segindex < SEGNUM; segindex++) {
	 if (_core_segment[segindex].type == EMPTY)
		break;
	 flag = FALSE;			/* no view surfaces need repaint */
	 if (_core_segment[segindex].type >= RETAIN)
	      for (vsindex = 0; vsindex<_core_segment[segindex].vsurfnum;
								vsindex++) {
	     	  if (_core_segment[segindex].vsurfptr[vsindex]->nwframnd
		      && (_core_segment[segindex].redraw ||
		         _core_segment[segindex].vsurfptr[vsindex]->nwframdv ||
			 explicit)) {
		      flag = TRUE;
	 	  }
	     }
	 if (flag) {
	      for (vsindex=0; vsindex<_core_segment[segindex].vsurfnum;
								vsindex++) {
		  vwsrf = _core_segment[segindex].vsurfptr[vsindex];
		  if (vwsrf->nwframnd
		      && (_core_segment[segindex].segats.visbilty)
		      && (_core_segment[segindex].redraw
		      || vwsrf->nwframdv || explicit)) {
		      if (vwsrf->hphardwr) {
			  ddstruct.opcode = SEGDRAW;
			  ddstruct.ptr1 = (char*)(&_core_segment[segindex]);
			  ddstruct.instance = vwsrf->vsurf.instance;
			  ddstruct.int1 = FALSE;
			  ddstruct.int2 = vsindex;
			  (*vwsrf->vsurf.dd)( &ddstruct);
		      } else
			  _core_segdraw(&_core_segment[segindex],vsindex,FALSE);
		  }
	      }
	 }
	 _core_segment[segindex].redraw = FALSE;
	}
    for (vsindex = 0; vsindex < MAXVSURF; vsindex++) {
	 _core_surface[vsindex].nwframnd = FALSE;
	}

    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	 (*_core_sighandle)();

    return(0);
}

