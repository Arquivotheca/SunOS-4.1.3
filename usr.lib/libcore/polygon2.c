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
static char sccsid[] = "@(#)polygon2.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <sunwindow/window_hs.h>
#include <stdio.h>

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polygon_rel_2                                              */
/*                                                                          */
/*     PURPOSE:  Fill an n sided region with the current color.  Clip to    */
/*         the current window, scale to the viewport, and output to DD.	    */
/*                                                                          */
/****************************************************************************/

polygon_rel_2( xlist, ylist, n)
register float *xlist, *ylist;
short n;
{
   char *funcname;
   short i;
   register float x, y, *xptr, *yptr;
   float xlistabs[MAXPOLY], ylistabs[MAXPOLY];

   funcname = "polygon_rel_2";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   if ( n > MAXPOLY) {
      _core_errhand(funcname,81);
      return(81);
      }
   if (n < 3) {
      _core_errhand(funcname,115);
      return(115);
      }

   xptr = xlistabs;
   yptr = ylistabs;
   x = _core_cp.x;
   y = _core_cp.y;
   for (i=0; i<n; i++) {
	x += *xlist++;
	*xptr++ = x;
	y += *ylist++;
	*yptr++ = y;
	}
   (void)polygon_abs_2(xlistabs, ylistabs, n);
   return (0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polygon_abs_2                                              */
/*                                                                          */
/*     PURPOSE:  Fill an n sided region with the current color.  Clip to    */
/*         the current window, scale to the viewport, and output to DD.	    */
/*                                                                          */
/****************************************************************************/

polygon_abs_2( xlist, ylist, n)
float *xlist, *ylist;
int n;
{
   char *funcname;
   short i;
   pt_type p1, p2;

   funcname = "polygon_abs_2";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   if ( n > MAXPOLY) {
      _core_errhand(funcname,81);
      return(81);
      }
   if (n < 3) {
      _core_errhand(funcname,115);
      return(115);
      }

   if (_core_fastflag)
	{
	viewsurf *surfp;
	int (*surfdd)();
	ddargtype ddstruct;

	_core_critflag++;
	_core_cp.x = *xlist;	_core_cp.y = *ylist;
	_core_cpchang = TRUE;
	for (i = 0; i < _core_openseg->vsurfnum; i++) {
		surfp = _core_openseg->vsurfptr[i];
		surfdd = surfp->vsurf.dd;
		ddstruct.instance = surfp->vsurf.instance;
		if (_core_fillcflag || _core_ropflag || _core_polatt)	{
			ddstruct.opcode = SETFCOL;	/* color changed */
			ddstruct.int1 = _core_current.fillindx;
			ddstruct.int2 =
			    (_core_xorflag)?XORROP:_core_current.rasterop;
			ddstruct.int3 = FALSE;
			(*surfdd)(&ddstruct);
			}
		if (_core_pisflag || _core_polatt ) {
					/* polygon interior changed */
			ddstruct.opcode = SETPISTYL;
			ddstruct.int1 = _core_current.polyintstyl;
			(*surfdd)(&ddstruct);
			}
		ddstruct.opcode = WLDPLYGNTOSCREEN;
		ddstruct.logical = TWOD;
		ddstruct.int1 = n;
		ddstruct.int2 = (int) &_core_cp;
		ddstruct.ptr1 = (char *) xlist;
		ddstruct.ptr2 = (char *) ylist;
		(*surfdd)(&ddstruct);
		}
	_core_fillcflag = FALSE;
	_core_ropflag = FALSE;
	_core_pisflag = FALSE;
	_core_pesflag = FALSE;
	_core_polatt = FALSE;
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle)();
	return(0);
	}
   if (_core_xformvs)
	{
	ddargtype ddstruct;

	_core_critflag++;
	_core_cp.x = *xlist;	_core_cp.y = *ylist;
	_core_cpchang = TRUE;
	ddstruct.opcode = WLDPLYGNTONDC;
	ddstruct.instance = _core_xformvs->instance;
	ddstruct.logical = TWOD;
	ddstruct.int1 = n;
	ddstruct.int2 = (int) &_core_cp;
	/* M -- max # of vertices in output list */
	ddstruct.int3 = 2*n;  
	ddstruct.ptr1 = (char *) xlist;
	ddstruct.ptr2 = (char *) ylist;
	*((vtxtype **) &ddstruct.float1) = _core_vtxlist;  /* output list */
	(*_core_xformvs->dd)(&ddstruct);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	if (ddstruct.int3 > 0)
		{
		(void)pgnall2( 0, ddstruct.int3);
		return(0);
		}
	if (ddstruct.int3 == 0)
		return(0);	/* polygon not visible or clipped polygon has
				   more than M vertices */
	}
   /* get here if !_core_fastflag || !_core_xformvs || too many vertices for
      _core_xformvs (ddstruct.int3 return value < 0 above */
   /*------------ View transform and clip boundary list --------*/
   p1 = _core_cp;
   move_abs_2( *xlist, *ylist);	/* first point is current point */
   _core_vtxcount = 0;
   for (i=0; i<n; i++) {
   	p1.x = *xlist++;
   	p1.y = *ylist++;
   	_core_tranpt2( &p1.x, &p2.x);	/* transform to NDC  */
   					/* convert to integer points */
   	_core_pt2cnvrt( &p2.x, &_core_vtxlist[_core_vtxcount++].x);
   	}
   if (_core_wndwclip) {
   	for (i=0; i<n; i++)		/* clip to view window  */
   		_core_clpvtx2( &_core_vtxlist[i]);
   	_core_clpend2();
   	for (i=n; i<_core_vtxcount; i++)  /* scale to viewport */
   		_core_vwpscale( (ipt_type *)&_core_vtxlist[i]);
	if (_core_vtxcount > n)
   		(void)pgnall2( n, _core_vtxcount-n);
   	return(0);
   	}
   for (i=0; i<n; i++)			/* scale to viewport */
   	_core_vwpscale( (ipt_type *)&_core_vtxlist[i]);
   (void)pgnall2( 0, n);
   return (0);
   }

/************************************************************************/
static pgnall2( indx, nvtx)  short indx, nvtx;
{
   ipt_type ip3;
   short ptype, i;
   viewsurf *surfp;
   int (*surfdd)();
   ddargtype ddstruct;


   /*---------------- Write polygon stuff out to PDF --------------*/

   _core_critflag++;
   if (_core_openseg->type >= RETAIN) {
	 ptype = PDFPICKID;
	 _core_pdfwrite(SHORT, &ptype);
	 _core_pdfwrite(FLOAT, (short *)&_core_current.pickid);
	 _core_pdfwrite(SHORT, &nvtx);		/* generate pick extents */
	 for (i=indx; i<nvtx+indx; i++) {
	     _core_pdfwrite( FLOAT*3, (short *)&_core_vtxlist[i]);
	     _core_update_segbndbox(_core_openseg, (ipt_type *)&_core_vtxlist[i]);
	     }
      if (_core_ropflag) {			/* color changed */
	 ptype = PDFROP; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_fillcflag) {			/* color changed */
	 ptype = PDFFCOLOR; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.fillindx);
	 }
      if (_core_pisflag) {			/* color changed */
	 ptype = PDFPISTYLE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.polyintstyl);
	 }
      if (_core_pesflag) {			/* color changed */
	 ptype = PDFPESTYLE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.polyedgstyl);
	 }
      ptype = PDFPOL2;			/* put clipped polygon in PDF */
      _core_pdfwrite(SHORT,&ptype);
      _core_pdfwrite(SHORT,&nvtx);
      for (i=indx; i<nvtx+indx; i++) {
	  _core_pdfwrite( FLOAT*4, (short *)&_core_vtxlist[i]);
	  }
      }

/*--------- Write polygons on all selected surfaces ------------*/

   for (i = 0; i < _core_openseg->vsurfnum; i++) {
	 surfp = _core_openseg->vsurfptr[i];
	 surfdd = surfp->vsurf.dd;
	 ddstruct.instance = surfp->vsurf.instance;
	 if (_core_fillcflag || _core_ropflag || _core_polatt)	{
	    ddstruct.opcode = SETFCOL;	/* color changed */
	    ddstruct.int1 = _core_current.fillindx;
    	    ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	    ddstruct.int3 = FALSE;
	    (*surfdd)(&ddstruct);
	    }
	 if (_core_pisflag || _core_polatt ) {	/* polygon interior changed */
	    ddstruct.opcode = SETPISTYL;
	    ddstruct.int1 = _core_current.polyintstyl;
	    (*surfdd)(&ddstruct);
	    }
	 if ( surfp->hphardwr ) {
	     if (_core_openseg->segats.visbilty) {
		 ddstruct.opcode = POLYGN2;	/* paint the polygon */
		 ddstruct.int1 = nvtx;
		 ddstruct.ptr1 = (char*)(&_core_vtxlist[indx]);
		 ddstruct.ptr2 = (char*)(&_core_ddvtxlist[indx]);
		 (*surfdd)(&ddstruct);
		 }
	     }
	 else {
	     /* image transform the polygon */
	     if(! _core_openseg->idenflag) {
	        if(_core_openseg->type == XLATE2) {
		    for (i=indx; i<nvtx+indx; i++) {	/* translate only */
		        _core_vtxlist[i].x += _core_openseg->imxform[3][0];
		        _core_vtxlist[i].y += _core_openseg->imxform[3][1];
		        }
		    }
	        else if(_core_openseg->type == XFORM2) {
		    for (i=indx; i<nvtx+indx; i++) {	/* full transform */
		        _core_moveword( (short *)&_core_vtxlist[i], (short *)&ip3, 8);
		        _core_imxfrm2( _core_openseg, &ip3, (ipt_type *)&_core_vtxlist[i]);
		        }
		    }
	        }
	     if (_core_outpclip) {
		  _core_vtxcount = nvtx + indx;
		  for (i=indx; i<nvtx+indx; i++) {	/* clip to viewport  */
		      _core_oclpvtx2(&_core_vtxlist[i],&_core_vwstate.viewport);
		      }
		  _core_oclpend2();
		  i = nvtx;
		  nvtx = _core_vtxcount - nvtx - indx;
		  indx = i + indx;
		  }
	     if (_core_openseg->segats.visbilty) {
		if (nvtx > 0) {
		 ddstruct.opcode = POLYGN2;	/* paint the polygon */
		 ddstruct.int1 = nvtx;
		 ddstruct.ptr1 = (char*)(&_core_vtxlist[indx]);
		 ddstruct.ptr2 = (char*)(&_core_ddvtxlist[indx]);
		 (*surfdd)(&ddstruct);
		 }
                }
	     }
      }

   _core_fillcflag = FALSE;
   _core_ropflag = FALSE;
   _core_pisflag = FALSE;
   _core_pesflag = FALSE;
   _core_polatt = FALSE;

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }
