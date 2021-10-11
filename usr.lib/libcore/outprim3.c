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
static char sccsid[] = "@(#)outprim3.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <math.h>

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_current_position_3                                 */
/*                                                                          */
/*     PURPOSE: INQUIRE CURRENT DRAWING POSITION                            */
/*                                                                          */
/****************************************************************************/

inquire_current_position_3(x,y,z) float *x,*y,*z;
{
   *x = _core_cp.x;
   *y = _core_cp.y;
   *z = _core_cp.z;
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_abs_3                                                 */
/*                                                                          */
/*     PURPOSE: DESCRIBE A LINE OF AN OBJECT IN WORLD COORDINATES.THIS LINE */
/*              RUNS FROM CP TO THE POSITION SPECIFIED. CP IS UPDATED.      */
/*                                                                          */
/****************************************************************************/

line_abs_3(x,y,z) float x,y,z;
{
   viewsurf *surfp;
   int (*surfdd)();
   pt_type p1;
   ipt_type ip1,ip2,ip3,ip4,dummy;
   char *funcname;
   short ptype, n, pt2winclip, earlyexit;
   ddargtype ddstruct;
   float f;

   funcname = "line_abs_3";
   if (! _core_osexists) {
      _core_errhand(funcname, 21);
      return(1);
      }
   _core_critflag++;
    if (_core_fastflag && (_core_current.linestyl == SOLID) && _core_fastwidth)
	{
	float xarray[1], yarray[1], zarray[1];

	for (n=0; n < _core_openseg->vsurfnum; n++) {
		surfp = _core_openseg->vsurfptr[n];
		surfdd = surfp->vsurf.dd;
		ddstruct.instance = surfp->vsurf.instance;
		if (_core_linecflag || _core_ropflag || _core_linatt) {
			ddstruct.opcode = SETLCOL;	/* color changed */
			ddstruct.int1 = _core_current.lineindx;
			ddstruct.int2 =
				(_core_xorflag)?XORROP:_core_current.rasterop;
			ddstruct.int3 = FALSE;
			(*surfdd)(&ddstruct);
			}
		if (_core_lsflag || _core_linatt) { /* line style changed */
			ddstruct.opcode = SETSTYL;
			ddstruct.int1 = _core_current.linestyl;
			(*surfdd)(&ddstruct);
			}
		if (_core_lwflag || _core_linatt) { /* line width changed */
			ddstruct.opcode = SETWIDTH;
			f = (float)((_core_ndcspace[0]<_core_ndcspace[1])?
				_core_ndcspace[0] : _core_ndcspace[1]);
			ddstruct.int1 = _core_current.linwidth * f / 100;
			(*surfdd)(&ddstruct);
			}
                ddstruct.opcode = WLDVECSTOSCREEN;
                ddstruct.logical = THREED;
                ddstruct.int1 = 1;      /* # of vectors */
                ddstruct.int2 = (int) &_core_cp;
                xarray[0] = x; yarray[0] = y; zarray[0] = z;
                ddstruct.ptr1 = (char *) xarray;
                ddstruct.ptr2 = (char *) yarray;
                ddstruct.ptr3 = (char *) zarray;
                (*surfdd)(&ddstruct);
                }
        _core_cp.x = x;  _core_cp.y = y; _core_cp.z = z;
        _core_linecflag = FALSE;
        _core_ropflag = FALSE;
        _core_lsflag = FALSE;
        _core_lwflag = FALSE;
        _core_linatt = FALSE;
        _core_cpchang = TRUE;
        if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
                (*_core_sighandle)();
	return (0);
	}
   else if (_core_xformvs)
        {
        short resultflags[1];
        float xarray[1], yarray[1], zarray[1];
 
        ddstruct.opcode = WLDVECSTONDC;
        ddstruct.instance = _core_xformvs->instance;
        ddstruct.logical = THREED;
        ddstruct.int1 = 1;              /* # of vectors */
        ddstruct.int2 = (int) &_core_cp;
        ddstruct.int3 = (int) resultflags;
        xarray[0] = x; yarray[0] = y; zarray[0] = z;
        ddstruct.ptr1 = (char *) xarray;
        ddstruct.ptr2 = (char *) yarray;
        ddstruct.ptr3 = (char *) zarray;
        *((ipt_type **) &ddstruct.float1) = &ip1;
        *((ipt_type **) &ddstruct.float2) = &ip2;
        (*_core_xformvs->dd)(&ddstruct);
        _core_cp.x = x;  _core_cp.y = y; _core_cp.z = z;
        if (resultflags[0] < 0)
                {               /* vector not visible */
                _core_cpchang = TRUE;
                if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)                        (*_core_sighandle)();
                return(0);
                }
	_core_cpchang |= (resultflags[0] & 1); /* bit 0 indicates pt1 clipped */
	pt2winclip = (resultflags[0] & 2);     /* bit 1 indicates pt2 clipped */
	}
    else {
	ip1 = _core_ndccp;
	_core_cp.x = x; _core_cp.y = y; _core_cp.z = z;
	_core_tranpt( &_core_cp.x, &p1.x);	/* view xform wld to ndc */
	_core_pt3cnvrt( &p1.x, &ip2.x);
	dummy = ip2;
	pt2winclip = 0;
	if(_core_wclipplanes) {
		if (!_core_clipvec(&ip1,&ip2)) {	/* if not visible */
	 	_core_ndccp = dummy;
	 	_core_cpchang = TRUE;
	 	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
			(*_core_sighandle)();
	 	return(0);
	 	}
        _core_cpchang |= ((ip1.x != _core_ndccp.x) || (ip1.y != _core_ndccp.y));
         pt2winclip = ((ip2.x != dummy.x) || (ip2.y != dummy.y) ||
	     (ip2.z != dummy.z));
	}
        _core_vwpscale( &ip1);                  /* viewport scale to NDC */
        _core_vwpscale( &ip2);
	}
   if (_core_openseg->type >= RETAIN) {
      ptype = PDFPICKID;
      _core_pdfwrite(SHORT,&ptype);
      _core_pdfwrite(FLOAT,(short *)&_core_current.pickid);
      n = 0;
      _core_pdfwrite(SHORT,&n);

      if (_core_ropflag) {			/* color changed */
	 ptype = PDFROP; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_linecflag) {			/* color changed */
	 ptype = PDFLCOLOR; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.lineindx);
	 }
      if (_core_lsflag) {			/* line style changed */
	 ptype = PDFLINESTYLE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.linestyl);
	 }
      if (_core_lwflag) {			/* line width changed */
	 ptype = PDFLINEWIDTH; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.linwidth);
	 }
      if (_core_cpchang) {
			/* if the cp has been changed since last put in pdf */
	 ptype = PDFMOVE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT*3,(short *)&ip1);
         _core_update_segbndbox(_core_openseg, &ip1);
	 }
      ptype = PDFLINE; _core_pdfwrite(SHORT,&ptype);
      _core_pdfwrite(FLOAT*3,(short *)&ip2);
      _core_update_segbndbox(_core_openseg, &ip2);
      }
/*** now write the vector to the appropriate view surfaces***/

   earlyexit = FALSE;
   for (n = 0; n < _core_openseg->vsurfnum; n++) {
	 surfp = _core_openseg->vsurfptr[n];
	 surfdd = surfp->vsurf.dd;
	 ddstruct.instance = surfp->vsurf.instance;
	 if (_core_linecflag || _core_ropflag || _core_linatt)	{
	       ddstruct.opcode = SETLCOL;	/* color changed */
	       ddstruct.int1 = _core_current.lineindx;
    	       ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	       ddstruct.int3 = FALSE;
	       (*surfdd)(&ddstruct);
	    }
	 if (_core_lsflag || _core_linatt) {	/* line style changed */
	       ddstruct.opcode = SETSTYL;
	       ddstruct.int1 = _core_current.linestyl;
	       (*surfdd)(&ddstruct);
	    }
	 if (_core_lwflag || _core_linatt) {	/* line width changed */
	       ddstruct.opcode = SETWIDTH;
	       f = (float)((_core_ndcspace[0]<_core_ndcspace[1])?
	       _core_ndcspace[0] : _core_ndcspace[1]);
	       ddstruct.int1 = _core_current.linwidth * f / 100.;
	       (*surfdd)(&ddstruct);
	    }
	if (surfp->hphardwr) {
	    if (_core_cpchang)	{	/* cp changed since last sent to DD */
		ddstruct.opcode = MOVE;
		ddstruct.int1 = ip1.x;
		ddstruct.int2 = ip1.y;
		ddstruct.int3 = ip1.z;
		(*surfdd)(&ddstruct);
		}
	    if (_core_openseg->segats.visbilty) {
		ddstruct.opcode = LINE;
		ddstruct.int1 = ip2.x;
		ddstruct.int2 = ip2.y;
		ddstruct.int3 = ip2.z;
		(*surfdd)(&ddstruct);
		}
	    }
	else {
	    /* before sending to the device, use imxform on primitives */
	    if(! _core_openseg->idenflag) {
		ip3 = ip1;
		ip4 = ip2;
		_core_imxfrm3( _core_openseg, &ip3, &ip1);
		_core_imxfrm3( _core_openseg, &ip4, &ip2);
		}
	   if(_core_outpclip) {
	      if (!_core_oclpvec2(&ip1,&ip2,&_core_vwstate.viewport)) {
		 earlyexit = TRUE;		/* if not visible */
		 break;
		 }
	      }
	   if (_core_cpchang)	{	/* cp changed since last sent to DD */
		ddstruct.opcode = MOVE;
		ddstruct.int1 = ip1.x;
		ddstruct.int2 = ip1.y;
		ddstruct.int3 = ip1.z;
		(*surfdd)(&ddstruct);
		}
	   if (_core_openseg->segats.visbilty) {
		ddstruct.opcode = LINE;
		ddstruct.int1 = ip2.x;
		ddstruct.int2 = ip2.y;
		ddstruct.int3 = ip2.z;
		(*surfdd)(&ddstruct);
		}

	   }
      }

   if (!earlyexit)
        {
        _core_linecflag = FALSE;
        _core_ropflag = FALSE;
        _core_lsflag = FALSE;
        _core_lwflag = FALSE;
        _core_linatt = FALSE;
        }

   if(pt2winclip || earlyexit) 	/* if clipped, we need to change cp in PDF */
	 _core_cpchang = TRUE;
   else
        _core_cpchang = FALSE;
   if (!_core_xformvs)
        _core_ndccp = dummy;
   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_rel_3                                                 */
/*                                                                          */
/*     PURPOSE: DESCRIBE A LINE OF AN OBJECT IN WORLD COORDINATES.THIS LINE */
/*              RUNS FROM CP TO THE POSITION SPECIFIED. CP IS UPDATED.      */
/*                                                                          */
/****************************************************************************/

line_rel_3(dx,dy,dz) float dx,dy,dz;
{
   if (! _core_osexists) {
      _core_errhand("line_rel_3",21);
      return(1);
      }
   return( line_abs_3(_core_cp.x + dx, _core_cp.y + dy, _core_cp.z + dz)    );
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: move_abs_3                                                 */
/*                                                                          */
/*     PURPOSE: THE CP IS SET TO (x,y,z) WHERE (x,y,z) IS A POSITION        */
/*              IN THE WORLD COORDINATE SYSTEM.                             */
/*                                                                          */
/****************************************************************************/

move_abs_3(x,y,z) float x,y,z;
{
   pt_type point;

   _core_cp.x = x;
   _core_cp.y = y;
   _core_cp.z = z;
   if (!_core_xformvs)
        {
        _core_tranpt( &_core_cp.x, &point.x);
        _core_pt3cnvrt( &point.x, &_core_ndccp.x);
        }
   _core_cpchang = TRUE;
   return(0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: move_rel_3                                                 */
/*                                                                          */
/*     PURPOSE: THE CP IS SET TO (x,y,z) WHERE (x,y,z) IS A POSITION        */
/*              IN THE WORLD COORDINATE SYSTEM RELATIVE TO PREVIOUS (x,y,z).*/
/*                                                                          */
/****************************************************************************/

move_rel_3(dx,dy,dz) float dx,dy,dz;
{
   pt_type point;
   float fdx, fdy, fdz;

   fdx = dx;            /* use fdx,fdy,fdy to convert dbl precision args to */
   fdy = dy;            /* single precision, so adds are done in single */
   fdz = dz;            /* (assuming compiled with -fsingle) */

   _core_cp.x += fdx;
   _core_cp.y += fdy;
   _core_cp.z += fdz;
   if (!_core_xformvs)
        {
        _core_tranpt( &_core_cp.x, &point.x);
        _core_pt3cnvrt( &point.x, &_core_ndccp.x);
        }
   _core_cpchang = TRUE;
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polyline_abs_3                                             */
/*                                                                          */
/*     PURPOSE: DESCRIBE A CONNECTED SEQUENCE OF LINES IN AN OBJECT IN      */
/*              WORLD COORDINATES.                                          */
/*                                                                          */
/****************************************************************************/

polyline_abs_3(xcoord,ycoord,zcoord,n) float xcoord[],ycoord[],zcoord[]; int n;
{
   char *funcname;
   register int index;

   funcname = "polyline_abs_3";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   if (n <= 0) {
      _core_errhand(funcname,22);
      return(2);
      }

   (void)move_abs_3(xcoord[0],ycoord[0],zcoord[0]);
   for (index = 1; index < n; index++) {
      (void) line_abs_3(xcoord[index], ycoord[index],zcoord[index]);
      }
   return(0);
   }


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polyline_rel_3                                             */
/*                                                                          */
/*     PURPOSE: DESCRIBE A CONNECTED SEQUENCE OF LINES IN AN OBJECT IN      */
/*              WORLD COORDINATES.                                          */
/*                                                                          */
/****************************************************************************/

polyline_rel_3(xcoord,ycoord,zcoord,n) float xcoord[],ycoord[],zcoord[]; int n;
{
   char *funcname;
   register int index;

   funcname = "polyline_rel_3";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   if (n <= 0) {
      _core_errhand(funcname,22);
      return(2);
      }
   for (index = 0; index < n; index++) {
       (void)line_rel_3(xcoord[index],ycoord[index],zcoord[index]);
       }
   return(0);
   }

