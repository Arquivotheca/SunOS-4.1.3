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
static char sccsid[] = "@(#)rasterprim.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: put_raster                                                 */
/*                                                                          */
/*     PURPOSE: The rectangular raster is written with the upper left pixel */
/*	    at the CP.  The raster extends height pixels downward and width */
/*	    pixels to the right and therefore is device dependent.  The     */
/*	    raster is clipped to the current viewport.			    */
/*									    */
/****************************************************************************/
put_raster( srast) rast_type *srast;		/* pointer to source raster */
{
   char *funcname;
   viewsurf *surfp;
   int (*surfdd)();
   pt_type p1;
   ipt_type ip1, ip2, ip3;
   int size;
   short *ptr, ptype, n;
   ddargtype ddstruct;

   funcname = "put_raster";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }
   if (_core_openseg->type > XLATE2) {
      _core_errhand(funcname,28);	/* rasters may only be translated */
      }
   if ( srast->depth != 1 && srast->depth != 8) {
      _core_errhand(funcname,86);		/* 1 or 8 bit pixels only */
      return(1);
      }

   _core_critflag++;
   if (_core_xformvs)
	{
	_core_tranpt(&_core_cp.x, &p1.x);
	_core_pt3cnvrt(&p1.x, &_core_ndccp.x);
	}
   ip1 = _core_ndccp;
   _core_vwpscale( &ip1);			/* scale,trans cp to vwport */

   if (_core_openseg->type >= RETAIN) {
	 ptype = PDFPICKID;
	 (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.pickid);
	 n = 4;
	 (void)_core_pdfwrite(SHORT,&n);
			/* pick extent covers upper left of bitmap for now */
	 ip3 = ip1;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);	/* pixel extent in NDC space */
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.y = ip1.y + (srast->height<<5);
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.x = ip1.x + (srast->width<<5);
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
	 ip3.y = ip1.y;
	 (void)_core_pdfwrite(FLOAT*3, (short *)&ip3);
	 _core_update_segbndbox(_core_openseg, &ip3);
      if (_core_ropflag) {			/* rasterop changed */
	 ptype = PDFROP; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_fillcflag) {			/* color changed */
	 ptype = PDFFCOLOR; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT,(short *)&_core_current.fillindx);
	 }
      if (_core_cpchang) {			/* if cp changed */
	 ptype = PDFMOVE; (void)_core_pdfwrite(SHORT,&ptype);
	 (void)_core_pdfwrite(FLOAT*3,(short *)&ip1);
	 }
      ptype = PDFBITMAP; (void)_core_pdfwrite(SHORT,&ptype);
      (void)_core_pdfwrite(FLOAT,(short *)&srast->width);
      (void)_core_pdfwrite(FLOAT,(short *)&srast->height);
      (void)_core_pdfwrite(FLOAT,(short *)&srast->depth);
      if (srast->depth == 1)
		size = ((srast->width+15)>>4)*srast->height*2;
      else if (srast->depth == 8)
		size = ((srast->width + 1) & ~1)*srast->height;
      else size =  srast->width*srast->height*3;
      if (size & 1) size += 1;
      ptr = srast->bits;
      (void)_core_pdfwrite( size, ptr);
   }

   /* perform an image transformation on the point before sending to DD */

   if( !_core_openseg->idenflag) {
      if(_core_openseg->type == XLATE2) {	/* PERFORM TRANSLATION ONLY. */
	 ip1.x += _core_openseg->imxform[3][0];
	 ip1.y += _core_openseg->imxform[3][1];
      }
   }
   ip2.x = _core_vwstate.viewport.xmin;	/* build NDC clip bounds for rast */
   ip2.y = _core_vwstate.viewport.ymin;
   ip3.x = _core_vwstate.viewport.xmax;
   ip3.y = _core_vwstate.viewport.ymax;

	/*-------------- Output to Devices -------------------*/

   for (n = 0; n < _core_openseg->vsurfnum; n++) {
	 surfp = _core_openseg->vsurfptr[n];
	 surfdd = surfp->vsurf.dd;
	 ddstruct.instance = surfp->vsurf.instance;
	 if (_core_fillcflag || _core_ropflag || _core_rasatt){
	       ddstruct.opcode = SETFCOL;	/* color changed */
	       ddstruct.int1 = _core_current.fillindx;
    	       ddstruct.int2 = (_core_xorflag)?XORROP:_core_current.rasterop;
    	       ddstruct.int3 = FALSE;
	       (*surfdd)(&ddstruct);
	    }
	 if(_core_cpchang){  			/* current point changed */
	    ddstruct.opcode = MOVE;
	    ddstruct.int1 = ip1.x;
	    ddstruct.int2 = ip1.y;
	    (*surfdd)(&ddstruct);
	    }
	 if (_core_openseg->segats.visbilty) {
	    ddstruct.opcode = RASPUT;
	    ddstruct.logical = TRUE;
	    ddstruct.ptr1 = (char*)srast;
	    ddstruct.ptr2 = (char*)(&ip2);	/* lower left viewport */
	    ddstruct.ptr3 = (char*)(&ip3);	/* upper right viewport */
	    (*surfdd)(&ddstruct);
	    }
     }

   _core_fillcflag = FALSE;
   _core_ropflag = FALSE;
   _core_rasatt = FALSE;
   _core_cpchang = FALSE;

   if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   return(0);
   }

/*----------------------------------------------------------------------*/
/*  size_raster 							*/
/*	Compute height, width, and depth in pixel coordinates for a     */
/*	raster from a particular view surface whose extent in NDC	*/
/*	coordinates is lower left (xmin,ymin) and upper right is	*/
/*	(xmax,ymax).  The bits per pixel or 'depth' of the raster will  */
/*	be 1 for the sunbitmap, and 8 for the suncolor view surface.	*/
/*	The raster.bits field will be set to the NULL pointer.		*/
/*									*/
/*	size_raster( surfname, xmin, xmax, ymin, ymax, ras)		*/
/*	allocate_raster( raster);					*/
/*	get_raster( surf,xmin,xmax,ymin,ymax, xd, yd, raster);		*/
/*	...use the raster...						*/
/*	free_raster( raster);						*/
/*									*/
/*----------------------------------------------------------------------*/
size_raster( surfname, xmin, xmax, ymin, ymax, raster)
    struct vwsurf *surfname;
    float xmin, ymin, xmax, ymax;		/* region of NDC space to get */
    rast_type *raster;
{
    ddargtype ddstruct;
    ipt_type pll, pur;
    viewsurf *_core_VSnsrch();
    char *funcname;

    funcname = "size_raster";

    /* check surface name */
    if ( !_core_VSnsrch(surfname) ) {
	_core_errhand(funcname, 5); return(5);
    }

	    /* upper left coords */
    pll.x = xmin * (float)MAX_NDC_COORD;
    pll.y = ymin * (float)MAX_NDC_COORD;
    pll.z = 0;   pll.w = 1;
	    /* lower right coords */
    pur.x = xmax * (float)MAX_NDC_COORD;
    pur.y = ymax * (float)MAX_NDC_COORD;
    pur.z = 0;   pur.w = 1;

    ddstruct.instance = surfname->instance;
    ddstruct.opcode = RASGET;
    ddstruct.ptr1 = (char*)raster;
    ddstruct.ptr2 = (char*)(&pll);		/* lower left raster  */
    ddstruct.ptr3 = (char*)(&pur);		/* upper right raster */
    ddstruct.logical = TRUE;			/* upper right raster */
    (*surfname->dd)(&ddstruct);
    return(0);
}

/*----------------------------------------------------------------------*/
allocate_raster( rptr) rast_type *rptr;
{
    short *ptr;
    int count, i;

    if (rptr->depth == 1) {
        count = ((rptr->width+15)>>4)*rptr->height;
        ptr = rptr->bits = (short*)malloc((unsigned int)(count*2));
	if (ptr) {
	    for (i=0; i<count; i++) *ptr++ = 0xffff;
	    }
	}
    else if (rptr->depth == 8) {
	rptr->bits = (short*)malloc((unsigned int)(((rptr->width+1) & ~1) * rptr->height));
	}
    else rptr->bits = 0;
}
/*----------------------------------------------------------------------*/
free_raster( rptr) rast_type *rptr;
{
    if (rptr->bits) {
	free((char *)rptr->bits);
	rptr->bits = 0;
	}
}

/*----------------------------------------------------------------------*/
/*  get_raster 								*/
/*	Fill in the data for a sized and allocated raster.		*/
/*	Read a region of NDC space and put it in the raster starting 	*/
/*	at xd, yd in pixel coordinates from upper left.			*/
/*									*/
/*	size_raster( surfname, xmin, xmax, ymin, ymax, ras)		*/
/*	allocate_raster( raster);					*/
/*	get_raster( surf,xmin,xmax,ymin,ymax, xd, yd, raster);		*/
/*	...use the raster...						*/
/*	free_raster( raster);						*/
/*									*/
/*----------------------------------------------------------------------*/
get_raster( surfname, xmin, xmax, ymin, ymax, xd, yd, raster)
    struct vwsurf *surfname;
    float xmin, ymin, xmax, ymax;	/* region of NDC space to get */
    int xd, yd;
    rast_type *raster;
{
    char *funcname;
    ddargtype ddstruct;
    ipt_type pll, pur;
    viewsurf *_core_VSnsrch();

    funcname = "get_raster";

    /* check surface name */
    if ( !_core_VSnsrch(surfname) ) {
	_core_errhand(funcname, 5); return(5);
    }

    xmin *= (float)MAX_NDC_COORD;
    xmax *= (float)MAX_NDC_COORD;
    ymin *= (float)MAX_NDC_COORD;
    ymax *= (float)MAX_NDC_COORD;
	    /* upper left coords */
    pll.x = (xmin> 0) ? ((xmin < _core_ndcspace[0])?xmin:_core_ndcspace[0]) : 0;
    pll.y = (ymin> 0) ? ((ymin < _core_ndcspace[1])?ymin:_core_ndcspace[1]) : 0;
    pll.z = 0;   pll.w = 1;
	    /* lower right coords */
    pur.x = (xmax> 0) ? ((xmax < _core_ndcspace[0])?xmax:_core_ndcspace[0]) : 0;
    pur.y = (ymax> 0) ? ((ymax < _core_ndcspace[1])?ymax:_core_ndcspace[1]) : 0;
    pur.z = 0;   pur.w = 1;

    ddstruct.instance = surfname->instance;
    ddstruct.opcode = RASGET;
    ddstruct.ptr1 = (char*)raster;
    ddstruct.ptr2 = (char*)(&pll);		/* lower left raster  */
    ddstruct.ptr3 = (char*)(&pur);		/* upper right raster */
    ddstruct.logical = FALSE;			/* not sizing */
    ddstruct.int1 = xd;
    ddstruct.int2 = yd;
    if ((*surfname->dd)(&ddstruct)) {
	_core_errhand( funcname, 88);
	return(88);
	}
    return(0);
}

