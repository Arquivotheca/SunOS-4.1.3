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
static char sccsid[] = "@(#)reopenseg.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
_core_reopensegment()
   {
   int index;
   ddargtype ddstruct;
   register viewsurf *surfp;
   					/* delete the segment from devices */
   for (index = 0; index < _core_openseg->vsurfnum; index++)
      {
      surfp = _core_openseg->vsurfptr[index];
      if (surfp->dehardwr)
	 {
	 ddstruct.instance = surfp->vsurf.instance;
	 ddstruct.opcode = DELETE;
	 ddstruct.logical = _core_openseg->segname;
	 (*surfp->vsurf.dd)(&ddstruct);
	 }
					/* reopen the segment by calling DD's */
      if (surfp->segopclo)
	 {
	 ddstruct.instance = surfp->vsurf.instance;
	 ddstruct.opcode = OPENSEG;
	 ddstruct.logical = _core_openseg->segname;
	 (*surfp->vsurf.dd)(&ddstruct);
	 				/* replace what was there, leave open */
	 _core_segdraw(_core_openseg,index,FALSE);
	 }
      }
   return(0);
   }

