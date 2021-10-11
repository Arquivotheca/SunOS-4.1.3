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
static char sccsid[] = "@(#)inqprimatt.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/****************************************************************************/
inquire_color_indices(surf, i1, i2, red, grn, blu)
	struct vwsurf *surf;  int i1, i2;  float *red, *grn, *blu;
{
    char *funcname;
    ddargtype ddstruct;
    short i;
    unsigned char ured[256], green[256], blue[256];
    viewsurf *_core_VSnsrch();

    funcname = "inquire_color_indices";
    if (i1 < MINCOLORINDEX || i2 > MAXCOLORINDEX || i1 > i2) {
	_core_errhand(funcname,27);
	return(2);
    }
    if (_core_VSnsrch(surf)) {
	ddstruct.instance = surf->instance;
	ddstruct.opcode = GETTAB;
	ddstruct.int1 = i1;
	ddstruct.int2 = i2;
	ddstruct.ptr1 = (char*)ured;
	ddstruct.ptr2 = (char*)green;
	ddstruct.ptr3 = (char*)blue;
	(*surf->dd)(&ddstruct);
	for (i=0; i<i2-i1+1; i++) {
	    *red++ = ((float)(ured[i]) /  (float) 255.);
	    *grn++ = ((float)(green[i])/  (float) 255.);
	    *blu++ = ((float)(blue[i]) /  (float) 255.);
	    }
	return(0);
	}
    else {
	_core_errhand(funcname,5);
	return(1);
	}
}

/****************************************************************************/
inquire_line_index(color) int *color;
{
    *color = _core_current.lineindx;
    return(0);
   }

/****************************************************************************/
inquire_fill_index(color) int *color;
{
    *color = _core_current.fillindx;
    return(0);
   }

/****************************************************************************/
inquire_text_index(color) int *color;
{
    *color = _core_current.textindx;
    return(0);
   }

/****************************************************************************/
inquire_linestyle(linestyl) int *linestyl;
{
    *linestyl = _core_current.linestyl;
    return(0);
   }

/****************************************************************************/
inquire_polygon_interior_style(polyintstyl) int *polyintstyl;
{
    *polyintstyl = _core_current.polyintstyl;
    return(0);
   }

/****************************************************************************/
inquire_polygon_edge_style(polyedgstyl) int *polyedgstyl;
{
    *polyedgstyl = _core_current.polyedgstyl;
    return(0);
   }

/****************************************************************************/
inquire_linewidth(linwidth) float *linwidth;
{
    *linwidth = _core_current.linwidth;
    return(0);
   }

/****************************************************************************/
inquire_pen(pen) int *pen;
{
    *pen = _core_current.pen;
    return(0);
   }

/****************************************************************************/
inquire_font(font) int *font;
{
    *font = _core_current.font;
    return(0);
   }

/****************************************************************************/
inquire_charsize(chwidth,cheight) float *chwidth,*cheight;
{
    *chwidth = _core_current.charsize.width;
    *cheight = _core_current.charsize.height;
    return(0);
   }

/****************************************************************************/
inquire_charup_3( dx, dy, dz) float *dx,*dy,*dz;
{
    *dx = _core_current.chrup.x;
    *dy = _core_current.chrup.y;
    *dz = _core_current.chrup.z;
    return(0);
   }

/****************************************************************************/
inquire_charup_2( dx, dy) float *dx,*dy;
{
    *dx = _core_current.chrup.x;
    *dy = _core_current.chrup.y;
    return(0);
   }

/****************************************************************************/
inquire_charpath_3( dx, dy, dz) float *dx,*dy,*dz;
{
    *dx = _core_current.chrpath.x;
    *dy = _core_current.chrpath.y;
    *dz = _core_current.chrpath.z;
    return(0);
   }

/****************************************************************************/
inquire_charpath_2( dx, dy) float *dx,*dy;
{
    *dx = _core_current.chrpath.x;
    *dy = _core_current.chrpath.y;
    return(0);
   }

/****************************************************************************/
inquire_charspace(space) float *space;
{
    *space = _core_current.chrspace.x;
    return(0);
   }

/****************************************************************************/
inquire_charjust(chjust) int *chjust;
{
    *chjust = _core_current.chjust;
    return(0);
   }

/****************************************************************************/
inquire_charprecision(chqualty) int *chqualty;
{
    *chqualty = _core_current.chqualty;
    return(0);
   }

/****************************************************************************/
inquire_marker_symbol(mark) int *mark;
{
    *mark = _core_current.marker;
    return(0);
   }
   
/****************************************************************************/
inquire_pick_id(pickid) int *pickid;
{
    *pickid = _core_current.pickid;
    return(0);
   }

/****************************************************************************/
inquire_rasterop(rasterop) int *rasterop;
{
    *rasterop = _core_current.rasterop;
    return(0);
   }

/****************************************************************************/
inquire_primitive_attributes(defprim)
    primattr *defprim;
   {

   defprim->lineindx = _core_current.lineindx;
   defprim->fillindx = _core_current.fillindx;
   defprim->textindx = _core_current.textindx;
   defprim->linestyl = _core_current.linestyl;
   defprim->polyintstyl = _core_current.polyintstyl;
   defprim->polyedgstyl = _core_current.polyedgstyl;
   defprim->linwidth = _core_current.linwidth;
   defprim->pen = _core_current.pen;
   defprim->font = _core_current.font;
   defprim->charsize = _core_current.charsize;
   defprim->chrup = _core_current.chrup;
   defprim->chrpath = _core_current.chrpath;
   defprim->chrspace = _core_current.chrspace;
   defprim->chjust = _core_current.chjust;
   defprim->chqualty = _core_current.chqualty;
   defprim->marker = _core_current.marker;
   defprim->pickid = _core_current.pickid;
   defprim->rasterop = _core_current.rasterop;

   return(0);
   }
