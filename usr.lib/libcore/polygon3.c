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
static char sccsid[] = "@(#)polygon3.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include "colorshader.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_zbuffer_cut 	                                    */
/*                                                                          */
/*     PURPOSE:  Specify the cutaway surface used for clipping away portions*/
/*		of hidden surface objects.  Objects closer to the viewer    */
/*		than this surface are clipped away.			    */
/*                                                                          */
/****************************************************************************/
set_zbuffer_cut( surfname, xarr, zarr, n)
	float xarr[], zarr[];  int n;
	struct vwsurf *surfname;
{
	char *funcname;
        viewsurf *surfp, *_core_VSnsrch();
	ddargtype ddstruct;


	funcname = "set_zbuffer_cut";

    /*
     *  search surface table for entry with specified driver 
     */
        if ( !(surfp = _core_VSnsrch(surfname)) ) {
	      _core_errhand(funcname,5); return(5);
        }
        if (!surfp->hiddenon) {
	      _core_errhand(funcname,84); return(84);
        }
	ddstruct.opcode = SETZBCUT;           /* initialize the view surface */
	ddstruct.ptr1 = (char *)xarr;
	ddstruct.ptr2 = (char *)zarr;
	ddstruct.int1 = n;
	ddstruct.instance = surfname->instance;
	(*surfname->dd)(&ddstruct);
	return( 0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_shading_parameters                                     */
/*                                                                          */
/*     PURPOSE:  Specify the parameters for rendering 3-D polygons 	    */
/*		including ambient light and diffuse and specular reflection */
/*                                                                          */
/****************************************************************************/
set_shading_parameters( amb, dif, spec, flood, bump, hue, style)
	float amb, dif, spec, flood, bump;
	int hue, style;
{
	shading_parameter_structure *s;
    
	s = &_core_shading_parameters;	/* set surface shading */
       	s->ambient = amb * 32768;	/* percent ambient */
       	s->diffuseamt = dif * 32768;	/* percent diffuse */
       	s->specularamt = spec * 32768;	/* percent specular */
       	s->floodamt = flood * 32768;
       	s->bump = (int)bump;		/* specular power */
       	s->transparent = 0;
       	s->hue = hue;			/* 0,1,2,3,4 = select LUT section */
					/* 1-255,1-63,64-127,128-191,192-255 */
       	s->frenel = 0;			/* unused */
       	s->shadestyle = style;		/* 0,1,2 = solid, Gouraud, Phong */
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_light_direction                                        */
/*                                                                          */
/*     PURPOSE:  Specify the direction of the light source for shading 3D   */
/*		polygons.  The direction is specified in normalized device  */
/*		coordinates						    */
/*                                                                          */
/****************************************************************************/
set_light_direction( dx, dy, dz) float dx, dy, dz;
{
	pt_type p1,p2;
	shading_parameter_structure *s;

	if (dx == 0 && dy == 0. && dz == 0.) {
		_core_errhand("set_light_direction", 39); return(1);
		}
	s = &_core_shading_parameters;		/* set surface shading */
	p1.x = dx;  p1.y = dy;  p1.z = dz;  p1.w = 1.0;
	p2 = p1;
	_core_unitvec( &p1);
	s->lx = p1.x * 32768;
	s->ly = p1.y * 32768;
	s->lz = p1.z * 32768;
	p2.z -= 1.0;				/* H = (E+L)/|E+L|  */
	_core_unitvec( &p2);
	s->ex = 0; s->ey = 0; s->ez = -32768;	/* eye vector */
	s->hx = p2.x * 32768;			/* hilite vector */
	s->hy = p2.y * 32768;
	s->hz = p2.z * 32768;
	return (0);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_vertex_indices                                         */
/*                                                                          */
/*     PURPOSE:  Specify the color indices at the vertices of a 3D, n sided */
/*		planar polygon.  This routine converts the normals to unit  */
/*		vectors and stores them in vtxlist as fixed pt integers     */
/*		with 15 bit fractions.					    */
/****************************************************************************/
set_vertex_indices( indxlist, n)
	int *indxlist, n;
{
   char *funcname;
   short i;

   funcname = "set_vertex_indices";
   if (! _core_osexists) {
      _core_errhand(funcname,21); return(1);
      }
   if ( n > MAXPOLY) { /* too many vertices */
      _core_errhand(funcname, 81);
      return (81);
      }

   if ( n < 3) { /* too few vertices */
      _core_errhand(funcname, 115); 
      return (115);
      }

   _core_vtxcount = 0;
   for (i=0; i<n; i++) {		/* 0..255 -> 0..32k */
	_core_vtxlist[_core_vtxcount++].dx = (*indxlist++) << 7;
	}
   return (0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_vertex_normals                                         */
/*                                                                          */
/*     PURPOSE:  Specify the surface normal vectors for a 3D n sided planar */
/*		polygon.  This routine converts the normals to unit vectors */
/*		and stores them in vtxlist as fixed pt integers with 15 bit */
/*		fractions.						    */
/*                                                                          */
/****************************************************************************/
set_vertex_normals( dxlist, dylist, dzlist, n)
	float *dxlist, *dylist, *dzlist; int n;
{
   char *funcname;
   short i;
   pt_type p1, p2;

   funcname = "set_vertex_normals";
   if (! _core_osexists) {
      _core_errhand(funcname,21); return(1);
      }

   if ( n > MAXPOLY) { /* too many vertices */
      _core_errhand(funcname,81); return(81);
      }

   if ( n < 3) { /* too few vertices */
      _core_errhand(funcname,115); return(115);
      }

   _core_vtxnmlcnt = n;

   /*------------ View transform normal vectors --------*/

   _core_vtxcount = 0;
   p1.w = 1.0;
   for (i=0; i<n; i++) {
	p1.x = *dxlist++;			/* Normal vectors have been  */
	p1.y = *dylist++;			/* transformed and normalized*/
	p1.z = *dzlist++;			/* by set_vertex_normals     */
	_core_sizept( &p1.x, &p2.x);			/* transform to NDC  */
	_core_unitvec( &p2);
	p2.x *= 32768.;				/* convert to fixed pt */
	p2.y *= 32768.;
	p2.z *= 32768.;
						/* convert to integer points */
	_core_pt3cnvrt( &p2.x, &_core_vtxlist[_core_vtxcount++].dx);
	}

	return (0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polygon_rel_3                                              */
/*                                                                          */
/*     PURPOSE:  Render an n sided 3D polygon using the shading parameters  */
/*		in the global structure '_core_shading_parameters'.  Clip to      */
/*         the current window, scale to the viewport, and output to DD.	    */
/*                                                                          */
/****************************************************************************/

polygon_rel_3( xlist, ylist, zlist, n) float *xlist, *ylist, *zlist;  int n;
{
   char *funcname;
   short i;
   register float x, y, z, *xptr, *yptr, *zptr;
   float xlistabs[MAXPOLY], ylistabs[MAXPOLY], zlistabs[MAXPOLY];

   funcname = "polygon_rel_3";
   if (! _core_osexists) {
      _core_errhand(funcname, 21);
      return(1);
      }

   if ( n > MAXPOLY) { /* too many vertices */
      _core_errhand(funcname, 81);
      return(81);
      }

   if ( n < 3) { /* too few vertices */
      _core_errhand(funcname, 115);
      return(115);
      }

   xptr = xlistabs;
   yptr = ylistabs;
   zptr = zlistabs;
   x = _core_cp.x;
   y = _core_cp.y;
   z = _core_cp.z;

   for (i=0; i<n; i++) {
	x += *xlist++;
	*xptr++ = x;
	y += *ylist++;
	*yptr++ = y;
	z += *zlist++;
	*zptr++ = z;
	}
   (void)polygon_abs_3(xlistabs, ylistabs, zlistabs, n);
   return (0);
   }

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polygon_abs_3                                              */
/*                                                                          */
/*     PURPOSE:  Render an n sided 3D polygon using the shading parameters  */
/*		in the global structure '_core_shading_parameters'.  Clip to      */
/*         the current window, scale to the viewport, and output to DD.	    */
/*                                                                          */
/****************************************************************************/

polygon_abs_3( xlist, ylist, zlist, n) 
	float *xlist, *ylist, *zlist;
	int n;
   {
   char *funcname;
   short i;
   pt_type p1, p2;

   funcname = "polygon_abs_3";
   if (! _core_osexists) {
      _core_errhand(funcname,21);
      return(1);
      }

   if ( n > MAXPOLY) {
      _core_errhand(funcname, 81);
      return(81);
      }

   if ( n < 3) {
      _core_errhand(funcname, 115);
      return(115);
      }

 
   if (_core_fastpoly && _core_vtxnmlcnt != n )
        {
        viewsurf *surfp;
        int (*surfdd)();
        ddargtype ddstruct;

        _core_critflag++;
        _core_cp.x = *xlist;    _core_cp.y = *ylist; _core_cp.z = *zlist;
        _core_cpchang = TRUE;
        for (i = 0; i < _core_openseg->vsurfnum; i++) {
                surfp = _core_openseg->vsurfptr[i];
                surfdd = surfp->vsurf.dd;
                ddstruct.instance = surfp->vsurf.instance;
                if (_core_fillcflag || _core_ropflag || _core_polatt)   {
                        ddstruct.opcode = SETFCOL;      /* color changed */
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
                ddstruct.logical = THREED;
                ddstruct.int1 = n;
                ddstruct.int2 = (int) &_core_cp;
                ddstruct.ptr1 = (char *) xlist;
                ddstruct.ptr2 = (char *) ylist;
                ddstruct.ptr3 = (char *) zlist;
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
   if (_core_xformvs && _core_vtxnmlcnt != n)
        {
        ddargtype ddstruct;

        _core_critflag++;
        _core_cp.x = *xlist;    _core_cp.y = *ylist; _core_cp.z = *zlist;
        _core_cpchang = TRUE;
        ddstruct.opcode = WLDPLYGNTONDC;
        ddstruct.instance = _core_xformvs->instance;
        ddstruct.logical = THREED;
        ddstruct.int1 = n;
       /* M -- max # of vertices in output list */
        ddstruct.int3 = 2*n;  
        ddstruct.ptr1 = (char *) xlist;
        ddstruct.ptr2 = (char *) ylist;
        ddstruct.ptr3 = (char *) zlist;
        *((vtxtype **) &ddstruct.float1) = &_core_vtxlist[n];  /* output list */
        (*_core_xformvs->dd)(&ddstruct);
        if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
                (*_core_sighandle)();
        if (ddstruct.int3 > 0)
                {
		_core_vtxcount = n + ddstruct.int3;
                pgnall3( n, ddstruct.int3);
                return (0);
                }
        if (ddstruct.int3 == 0)
                return(0);      /* polygon not visible or clipped polygon has
                                   more than M vertices */
        }

       /* 
        * get here if !_core_fastflag || !_core_xformvs || too many vertices for
        * _core_xformvs (ddstruct.int3 return value < 0 above  || phong
        * shading may be taking place 
        */

   /*------------ View transform and clip boundary list --------*/
   p1.w = 1.0;
   move_abs_3( *xlist, *ylist, *zlist);		/* first point becomes cp */
   _core_vtxcount = 0;
   for (i=0; i<n; i++) {
	p1.x = *xlist++;
	p1.y = *ylist++;
	p1.z = *zlist++;
	_core_tranpt( &p1.x, &p2.x);		/* transform to NDC  */
						/* convert to integer points */
	_core_pt3cnvrt( &p2.x, &_core_vtxlist[_core_vtxcount++].x);
	}					/* unit normal vectors already*/
   if (_core_wclipplanes) {			/* in vtxlist */
	for (i=0; i<n; i++) {
	    _core_clpvtx3( &_core_vtxlist[i]);	/* clip to view window  */
	    }					/* clips normals as well */
	_core_clpend3();
        for (i=n; i<_core_vtxcount; i++)  /* scale to viewport */
                _core_vwpscale( (ipt_type *)&_core_vtxlist[i]);
	if (_core_vtxcount > n)
		pgnall3( n, _core_vtxcount-n);
	return(0);
	}
   for (i=0; i<n; i++)                  /* scale to viewport */
        _core_vwpscale( (ipt_type *)&_core_vtxlist[i]);
   pgnall3( 0, n);
   return (0);
   }

/************************************************************************/
static pgnall3( indx, nvtx)  short indx, nvtx;
{
   ipt_type ip3;
   short ptype, i;
   viewsurf *surfp;
   int (*surfdd)();
   ddargtype ddstruct;

   /*---------------- Write vertex list stuff out to PDF --------------*/

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
      if (_core_ropflag) {			/* rasterop changed */
	 ptype = PDFROP; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.rasterop);
	 }
      if (_core_fillcflag) {			/* color changed */
	 ptype = PDFFCOLOR; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.fillindx);
	 }
      if (_core_pisflag) {			/* poly interior changed */
	 ptype = PDFPISTYLE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.polyintstyl);
	 }
      if (_core_pesflag) {			/* poly interior changed */
	 ptype = PDFPESTYLE; _core_pdfwrite(SHORT,&ptype);
	 _core_pdfwrite(FLOAT,(short *)&_core_current.polyedgstyl);
	 }
      ptype = PDFPOL3;			/* put clipped polygon in PDF */
      _core_pdfwrite(SHORT,&ptype);
      _core_pdfwrite(SHORT,&nvtx);
      for (i=indx; i<nvtx+indx; i++) {
	  _core_pdfwrite( FLOAT*8, (short *)&_core_vtxlist[i]);
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
	 if (  surfp->hphardwr ) {    /* Martin, this awaits GP poly*/
	     if (_core_openseg->segats.visbilty) {
		 ddstruct.opcode = POLYGN3;	/* paint the polygon */
		 ddstruct.int1 = nvtx;
		 ddstruct.int3 = surfp->hiddenon;
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
		        _core_vtxlist[i].z += _core_openseg->imxform[3][2];
		        }
		    }
	         else if(_core_openseg->type == XFORM2) {
		    for (i=indx; i<nvtx+indx; i++) {	/* full transform */
		        _core_moveword( (short *)&_core_vtxlist[i], (short *)&ip3, 8);
		        _core_imxfrm3( _core_openseg, (ipt_type *)&ip3, (ipt_type *)&_core_vtxlist[i]);
			/* imxform nrmals;  should convert to unit vec */
			if (_core_shading_parameters.shadestyle == WARNOCK) {
			    _core_moveword( (short *)&_core_vtxlist[i].dx, (short *)&ip3, 8);
			    _core_imszpt3(_core_openseg, (ipt_type *)&ip3,(ipt_type *)(&_core_vtxlist[i].dx));
			}

		    }
		    }
	     }
	     if (_core_outpclip) {
		 _core_vtxcount = nvtx + indx;
		 for (i=indx; i < nvtx + indx; i++) { /* clip to viewport  */
		     _core_oclpvtx2(&_core_vtxlist[i],&_core_vwstate.viewport);
		     }
		 _core_oclpend2();
		 i = nvtx;
		 nvtx = _core_vtxcount - nvtx - indx;
		 indx = i + indx;
		 }
	     if (_core_openseg->segats.visbilty) {
		if (nvtx > 0) {
		 ddstruct.opcode = POLYGN3;	/* paint the polygon */
		 ddstruct.int1 = nvtx;
		 ddstruct.int3 = surfp->hiddenon;
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

   if ( --_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle)();
   }
