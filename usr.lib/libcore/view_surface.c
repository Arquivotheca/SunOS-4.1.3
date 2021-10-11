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
static char sccsid[] = "@(#)view_surface.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <sunwindow/window_hs.h>

#define	NULL	0
extern struct vwsurf _core_nullvs;

mpr_static(blank_mpr, 0, 0, 0, 0);
static struct cursor blank_cursor = {0, 0, PIX_SRC^PIX_DST, &blank_mpr};


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: initialize_view_surface                                    */
/*                                                                          */
/*     PURPOSE: OBTAIN ACCESS TO THE LOGICAL VIEW SURFACE 'surfname' AND    */
/*              TO INITIALIZE THAT SURFACE.                                 */
/*                                                                          */
/****************************************************************************/

initialize_view_surface(surfname, type) struct vwsurf *surfname; int type;
{
   char *funcname;
   int errnum, i, j;
   int clear;
   int *ptr1, *ptr2;
   viewsurf *surfp, *_core_VSnsrch();
   ddargtype ddstruct;
   struct inputmask im;
   int dsgn;

   clear = TRUE;
/* if (type > 1) { clear = FALSE; type -= 2; } */
   funcname = "initialize_view_surface";
   if(_core_sysinit == FALSE) {
      errnum = 4;
      _core_errhand(funcname,errnum); return(3);
      }
   /*
    * SEARCH SURFACE STRUCTURE FOR SPECIFIED SURFACE AND CHECK
    * TO SEE IF IT HAS BEEN INITIALIZED. IF NOT, INITIALIZE IT.
    */
   if (!surfname->dd) {
      _core_errhand(funcname,83); return(1);
      }
   if ( !(surfp = _core_VSnsrch(surfname)) ) {
      if (surfname->instance) {
          _core_errhand(funcname,83);	/* vwsurf struct must have */
          return(1);			/* instance = 0 to get new surf */
	  }
      for (i=0; i<MAXVSURF; i++) 
	  if (_core_surface[i].vinit == FALSE) {
	      surfp = &_core_surface[i];
	      j = sizeof(struct vwsurf) / 4;
	      ptr1 = (int *) &surfp->vsurf;
	      ptr2 = (int *) surfname;
	      while (--j >= 0)
		   *ptr1++ = *ptr2++;
	      break;
	      }
      if (!surfp)
	  {
	  _core_errhand(funcname, 85);
	  return(1);
	  }
      }
   else {
      _core_errhand(funcname,2); return(1);
      }
   _core_critflag++;
   ddstruct.opcode = INITIAL;		/* initialize the view surface */
   ddstruct.instance = 0;	/* zero instance for test in pixwindd's */
   ddstruct.int1 = _core_winsys;
   ddstruct.int2 = i;			/* give dd vwsurfindex */
   ddstruct.ptr1 = (char*)surfp;
   (*surfname->dd)(&ddstruct);
   if ( !ddstruct.logical) {
	surfp->vsurf = _core_nullvs;
	_core_errhand(funcname, 3);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle)();
	return(1);
	}

   j = sizeof(struct vwsurf) / 4;
   ptr1 = (int *) &surfp->vsurf;
   ptr2 = (int *) surfname;
   while (--j >= 0)
	*ptr2++ = *ptr1++;
   surfp->vsurf.ptr = 0;
   ddstruct.instance = surfp->vsurf.instance;
   ddstruct.opcode = SETNDC;		/* INFORM DEVICE OF NDC BOUNDS */
   ddstruct.float1 = _core_ndc.width;
   ddstruct.float2 = _core_ndc.height;
   ddstruct.float3 = _core_ndc.depth;
   (*surfname->dd)(&ddstruct);
   surfp->vinit = TRUE;			/* MARK SURFACE INITIALIZED. */

   if (!_core_xformvs && surfp->hphardwr) {  /* Announce transform capability */
	_core_xformvs = &surfp->vsurf;
	_core_update_hphardwr(_core_xformvs);
   }

   if (surfp->hphardwr)
	{
	ddstruct.opcode = OUTPUTCLIP;
	ddstruct.int1 = _core_outpclip;
	(*surfname->dd)(&ddstruct);
	}

					/* set default cursor and input mask */
   win_setcursor(surfp->vsurf.windowfd, &blank_cursor);
   if (_core_winsys)
	{
	input_imnull(&im);
	win_setinputcodebit(&im, MS_LEFT);
	win_setinputcodebit(&im, MS_MIDDLE);
	win_setinputcodebit(&im, MS_RIGHT);
	if (!(_core_keybord[0].subkey.enable & 1))
		im.im_flags |= IM_ASCII;
	win_setinputmask(surfp->vsurf.windowfd, &im, (struct inputmask *)0, _core_shellwinnum);
	}
   else
	{
	win_getinputmask(surfp->vsurf.windowfd, &im, &dsgn);
	input_imnull(&im);
	win_setinputmask(surfp->vsurf.windowfd, &im, (struct inputmask *)0, dsgn);
	}
	

   if (type) {				/* if hidden surface view surface */
       if ( surfp->hihardwr) {
	   ddstruct.opcode = INITZB;	/* init z buffer */
   	   (*surfname->dd)(&ddstruct);
           if ( ddstruct.logical) surfp->hiddenon = TRUE;
	   }
	else _core_errhand( funcname, 84);
	}
   if (clear /* && surfp->windptr->rawdev */) {
	ddstruct.opcode = CLEAR;		/* clear the view surface */
   	(*surfname->dd)(&ddstruct);
	}
   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: select_view_surface                                        */
/*                                                                          */
/*     PURPOSE: SELECT THE LOGICAL VIEW SURFACE 'surfname' FOR ALL          */
/*              SUBSEQUENT GRAPHIC OUTPUT UNTIL THE SURFACE IS DESELECTED   */
/*              WITH A 'deselect_view_surface' FUNCTION CALL.               */
/*                                                                          */
/****************************************************************************/

select_view_surface(surfname) struct vwsurf *surfname;
{
   char *funcname; int errnum;
   viewsurf *surfp, *_core_VSnsrch();

   funcname = "select_view_surface";

   if(_core_osexists == TRUE) {  /*** IS A SEGMENT OPEN? ***/
      errnum = 8;
      _core_errhand(funcname,errnum);
      return(4);
      }
   /*
    * SEARCH SURFACE TABLE FOR MATCH TO 'sysnam' THAT CORRESPONDS
    * TO SPECIFIED 'surfname'.
    */

   if ( !(surfp = _core_VSnsrch(surfname)) ) {
      errnum = 5;
      _core_errhand(funcname,errnum); return(3);
      }
   if(surfp->selected) {        /*** SURFACE ALREADY BEEN SELECTED? **/
      errnum = 6;
      _core_errhand(funcname,errnum);
      return(2);
      }
   surfp->selected = TRUE;
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: deselect_view_surface                                      */
/*                                                                          */
/*     PURPOSE: DESELECTS THE LOGICAL VIEW SURFACE "surfname" FOR ALL       */
/*              SUBSEQUENT GRAPHIC OUTPUT UNTIL RESELECTED.                 */
/*                                                                          */
/****************************************************************************/

deselect_view_surface(surfname) struct vwsurf *surfname;
{
   char *funcname; int errnum;
   viewsurf *surfp, *_core_VSnsrch();

   funcname = "deselect_view_surface";

   if(_core_osexists == TRUE) {  /*** IS A SEGMENT OPEN? ***/
      errnum = 8;
      _core_errhand(funcname,errnum);
      return(2);
      }
   /*
    * SEARCH SURFACE TABLE FOR MATCH TO 'sysnam' THAT CORRESPONDS
    * TO SPECIFIED 'surfname'.
    */
   if( !(surfp = _core_VSnsrch(surfname)) ) {
      errnum = 5;
      _core_errhand(funcname,errnum); return(3);
      }
   if(!surfp->selected) {
      errnum = 9;
      _core_errhand(funcname,errnum);
      return(1);
      }
   surfp->selected = FALSE;
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: terminate_view_surface                                     */
/*                                                                          */
/*     PURPOSE: TERMINATE ACCESS TO THE VIEW SURFACE surfname.              */
/*                                                                          */
/****************************************************************************/

terminate_view_surface(surfname) struct vwsurf *surfname;
{
   char *funcname; int errnum,i;
   viewsurf *surfp, *_core_VSnsrch();
   segstruc *segptr;
   ddargtype ddstruct;
   device *devptr;

   funcname = "terminate_view_surface";

   /*
    * SEARCH SURFACE TABLE FOR ENTRY WITH SPECIFIED DRIVER.
    */

   if( !(surfp = _core_VSnsrch(surfname)) ) {
      errnum = 5;
      _core_errhand(funcname,errnum); return(3);
      }

   /*
    * Terminate all input device echoing on this view surface
    */

   for (i = 0; i < PICKS; i++)
	{
	devptr = &_core_pick[i].subpick;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(PICK, i + 1, (struct vwsurf *)NULL);
	}
   for (i = 0; i < LOCATORS; i++)
	{
	devptr = &_core_locator[i].subloc;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(LOCATOR, i + 1, (struct vwsurf *)NULL);
	}
   for (i = 0; i < KEYBORDS; i++)
	{
	devptr = &_core_keybord[i].subkey;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(KEYBOARD, i + 1, (struct vwsurf *)NULL);
	}
   for (i = 0; i < VALUATRS; i++)
	{
	devptr = &_core_valuatr[i].subval;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(VALUATOR, i + 1, (struct vwsurf *)NULL);
	}
   for (i = 0; i < BUTTONS; i++)
	{
	devptr = &_core_button[i].subbut;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(BUTTON, i + 1, (struct vwsurf *)NULL);
	}
   for (i = 0; i < STROKES; i++)
	{
	devptr = &_core_stroker[i].substroke;
	if ((devptr->enable & 1) && (devptr->echosurfp == surfp))
		set_echo_surface(STROKE, i + 1, (struct vwsurf *)NULL);
	}

   /*
    * SEARCH FOR SEGMENTS IN THE SEGMENT TABLE WHOSE IMAGES APPEAR ON
    * THE SPECIFIED SURFACE. MARK THOSE POINTERS TO THE VIEW SURFACE
    * AS BEING NULL, UNLESS THE SPECIFIED SURFACE IS THE ONLY ONE
    * ASSOCIATED WITH THE SEGMENT, IN WHICH CASE DELETE THE SEGMENT.
    */

   _core_critflag++;
   for(segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]
    && segptr->type != EMPTY; segptr++) {
      if(segptr->type >= NORETAIN) {
	 for(i = 0;i < segptr->vsurfnum;i++) {
	    if(segptr->vsurfptr[i] == surfp) {
	       if(segptr->vsurfnum == 1) {
		     if (segptr == _core_openseg) {
			close_retained_segment();/* close the open segment */
			}
		     if (segptr->type >= RETAIN)
			{
			segptr->type = DELETED;
			--_core_segnum;
			_core_PDFcompress(segptr);
			}
		     }
	       else  {
		     viewsurf **p1, **p2;
		     i = segptr->vsurfnum - i;
		     p1 = &segptr->vsurfptr[i];
		     p2 = p1 + 1;
		     while (i--)
			*p1++ = *p2++;
		     *p1 = NULL;
		     --(segptr->vsurfnum);
		     }
	       break;
	       }
	    }
	 }
      }
   ddstruct.instance = surfname->instance;
   ddstruct.opcode = CLEAR;
   (*surfname->dd)(&ddstruct);          /* CLEAR THE SURFACE AND  */

   if ( surfp->hihardwr && surfp->hiddenon) {
 	ddstruct.opcode = TERMZB;	/* Terminate z buffer */
	(*surfname->dd)(&ddstruct);
 	}

   surfp->vinit = FALSE;		/* set vinit FALSE here so if
					   termination causes a SIGCHILD,
					   _core_sigchild will not see this
					   surface as still being active */
   ddstruct.opcode = TERMINATE;
   (*surfname->dd)(&ddstruct);          /* TERMINATE ACCESS TO IT. */

   surfp->hphardwr = FALSE;		/* reset the view surf flags */
   surfp->lshardwr = FALSE;
   surfp->lwhardwr = FALSE;
   surfp->clhardwr = FALSE;
   surfp->txhardwr = FALSE;
   surfp->hihardwr = FALSE;
   surfp->erasure =  FALSE;
   surfp->nwframdv = FALSE;
   surfp->nwframnd = FALSE;
   surfp->hiddenon = FALSE;
   surfp->selected = FALSE;
   surfp->vsurf = _core_nullvs;

   if (_core_xformvs && (_core_xformvs == &surfp->vsurf)) {
					/* if this surface was xform server */
	_core_xformvs = 0;		/* terminate xform service */
	for (i=0; i<MAXVSURF; i++) {	/* find another xform server */
		if (_core_surface[i].vinit && _core_surface[i].hphardwr) {
		    _core_xformvs = &_core_surface[i].vsurf;
		    _core_update_hphardwr(_core_xformvs);
		    break;
		}
	}
	if (_core_xformvs == 0)
	    move_rel_3(0.0, 0.0, 0.0);		/* make sure _core_ndccp holds
						   correct value if no longer
						   have an xformvs
						*/
   }

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _core_VSnsrch                                                    */
/*                                                                          */
/*     PURPOSE: SEARCH THE SURFACE STRUCTURE FOR SPECIFIED SURFACE NAME.    */
/*                                                                          */
/****************************************************************************/

viewsurf *_core_VSnsrch(surfname) struct vwsurf *surfname;
{
   viewsurf *found;

   for (found = &_core_surface[0]; found < &_core_surface[MAXVSURF]; found++)
	if (found->vinit && found->vsurf.dd == surfname->dd &&
	    found->vsurf.instance == surfname->instance &&
	    found->vsurf.windowfd == surfname->windowfd)   /* double check */
   		return(found);		/* return ptr into view surface table */
   return((viewsurf *) 0);
   }
