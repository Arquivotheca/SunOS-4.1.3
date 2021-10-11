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
static char sccsid[] = "@(#)setprimatt.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

static primattr minimum =
	{MINCOLORINDEX,MINCOLORINDEX,MINCOLORINDEX,
	SOLID,PLAIN,SOLID,0.0,0,ROMAN, {0.0,0.0},
	{-INFINITY,-INFINITY,-INFINITY,1.0},{-INFINITY,-INFINITY,-INFINITY,1.0},
	{-INFINITY,-INFINITY,-INFINITY,1.0},
	0,STRING,MINASCII,0,NORMAL};
static primattr maximum =
	{MAXCOLORINDEX,MAXCOLORINDEX,MAXCOLORINDEX,
	DOTDASHED,SHADED,DOTDASHED,20.0,5,SYMBOLS, {INFINITY,INFINITY},
	{INFINITY,INFINITY,INFINITY,1.0},{INFINITY,INFINITY,INFINITY,1.0},
	{INFINITY,INFINITY,INFINITY,1.0},
	3,CHARACTER,MAXASCII,1000000000,ORROP};

/****************************************************************************/
set_drag( drag) int drag;
{
	_core_xorflag = drag;
	_core_ropflag = TRUE;
}
/****************************************************************************/
set_line_index(color) int color;
{
    char *funcname;

    funcname = "set_line_index";
    if (color < minimum.lineindx || color > maximum.lineindx) {
	_core_errhand(funcname,27); return(27);
    }
    _core_linecflag = TRUE;
    _core_current.lineindx = color;
    return(0);
   }

/****************************************************************************/
set_fill_index(color) int color;
{
    char *funcname;

    funcname = "set_fill_index";
    if (color < minimum.fillindx || color > maximum.fillindx) {
	_core_errhand(funcname,27); return(27);
    }
    _core_fillcflag = TRUE;
    _core_current.fillindx = color;
    return(0);
   }

/****************************************************************************/
set_text_index(color) int color;
{
    char *funcname;

    funcname = "set_text_index";
    if (color < minimum.textindx || color > maximum.textindx) {
	_core_errhand(funcname,27); return(27);
    }
    _core_textcflag = TRUE;
    _core_current.textindx = color;
    return(0);
   }

/****************************************************************************/
set_linestyle(linestyl) int linestyl;
{
    char *funcname;
    int errnum;

    funcname = "set_linestyle";
    if (linestyl < minimum.linestyl || linestyl > maximum.linestyl) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_lsflag = TRUE;
    _core_current.linestyl = linestyl;
    return(0);
   }

/****************************************************************************/
set_polygon_interior_style(polyintstyl) int polyintstyl;
{
    char *funcname;
    int errnum;

    funcname = "set_polygon_interior_style";
    if (polyintstyl<minimum.polyintstyl || polyintstyl>maximum.polyintstyl) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_pisflag = TRUE;
    _core_current.polyintstyl = polyintstyl;
    return(0);
   }

/****************************************************************************/
set_polygon_edge_style(polyedgstyl) int polyedgstyl;
{
    char *funcname;
    int errnum;

    funcname = "set_polygon_edge_style";
    if (polyedgstyl<minimum.polyedgstyl || polyedgstyl>maximum.polyedgstyl) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_pesflag = TRUE;
    _core_current.polyedgstyl = polyedgstyl;
    return(0);
   }

/****************************************************************************/
set_linewidth( linwidth) float linwidth;
{
    char *funcname;
    int errnum;

    funcname = "set_linewidth";
    if (linwidth < minimum.linwidth || linwidth > maximum.linwidth) {
	errnum = 27; _core_errhand(funcname,errnum);
	return(errnum);
       }
    _core_lwflag = TRUE;	/* linwidth is percent of NDC x extent */
    _core_current.linwidth = linwidth;
    if (_core_fastflag)
	_core_set_fastwidth();
    return(0);
   }

/****************************************************************************/
set_pen(pen) int pen;
{
    char *funcname;
    int errnum;

    funcname = "set_pen";
    if (pen < minimum.pen || pen > maximum.pen) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_penflag = TRUE;
    _core_current.pen = pen;
    return(0);
   }

/****************************************************************************/
set_font(font) int font;
{
    char *funcname;
    int errnum;

    funcname = "set_font";
    if (font < minimum.font || font > maximum.font) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_fntflag = TRUE;
    _core_current.font = font;
    return(0);
   }

/****************************************************************************/
set_charsize(chwidth,cheight) float chwidth,cheight;
   {
    char *funcname;
    int errnum;

    funcname = "set_charsize";
    if (chwidth < minimum.charsize.width || cheight < minimum.charsize.height){
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_upflag = TRUE;
    _core_pathflag = TRUE;
    _core_current.charsize.width = chwidth;
    _core_current.charsize.height = cheight;
    return(0);
   }

/****************************************************************************/
set_charup_3(dx,dy,dz) float dx,dy,dz;
{
    _core_upflag = TRUE;
    _core_current.chrup.x = dx;
    _core_current.chrup.y = dy;
    _core_current.chrup.z = dz;
    _core_current.chrup.w = 1.0;

    _core_unitvec( &_core_current.chrup);
    return(0);
   }

/****************************************************************************/
set_charup_2(dx,dy) float dx,dy;
{
    _core_upflag = TRUE;
    _core_current.chrup.x = dx;
    _core_current.chrup.y = dy;
    _core_current.chrup.z = 0.0;
    _core_current.chrup.w = 1.0;

    _core_unitvec( &_core_current.chrup);
    return(0);
   }

/****************************************************************************/
set_charpath_2(dx,dy) float dx,dy;
{
    _core_pathflag = TRUE;
    _core_spaceflag = TRUE;
    _core_current.chrpath.x = dx;
    _core_current.chrpath.y = dy;
    _core_current.chrpath.z = 0.0;
    _core_current.chrpath.w = 1.0;
    _core_unitvec( &_core_current.chrpath);
    return(0);
}

/****************************************************************************/
set_charpath_3(dx,dy,dz) float dx,dy,dz;
{
    _core_pathflag = TRUE;
    _core_spaceflag = TRUE;
    _core_current.chrpath.x = dx;
    _core_current.chrpath.y = dy;
    _core_current.chrpath.z = dz;
    _core_current.chrpath.w = 1.0;
    _core_unitvec( &_core_current.chrpath);
    return(0);
}

/****************************************************************************/
set_charspace(space) float space;
{
    _core_spaceflag = TRUE;
    _core_current.chrspace.x = space;
    return(0);
   }

/****************************************************************************/
set_charjust(chjust)
   int chjust;
{
    char *funcname;
    int errnum;

    funcname = "set_charjust";
    if (chjust < minimum.chjust || chjust > maximum.chjust) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_current.chjust = chjust;
    _core_justflag = TRUE;
    return(0);
   }

/****************************************************************************/
set_charprecision(chqualty)
   int chqualty;
{
    char *funcname;
    int errnum;

    funcname = "set_charprecision";
    if (chqualty < minimum.chqualty || chqualty > maximum.chqualty) {
       errnum = 27;
       _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_current.chqualty = chqualty;
    _core_qualflag = TRUE;
    return(0);
   }

/****************************************************************************/
set_marker_symbol(mark) int mark;
{
    _core_markflag = TRUE;
    _core_current.marker = mark;
    return(0);
   }

/****************************************************************************/
set_pick_id(pickid) int pickid;
{
    char *funcname;
    int errnum;

    funcname = "set_pick_id";
    if (pickid < minimum.pickid) {
       errnum = 27; _core_errhand(funcname,errnum);
       return(errnum);
       }
    _core_current.pickid = pickid;
    return(0);
   }

/****************************************************************************/
set_rasterop(flag) int flag;	/* 0 for normal erase-redraw, 1 for xoring, */
{				/* 2 for oring			*/
    _core_ropflag = TRUE;
    _core_linecflag = TRUE;
    _core_fillcflag = TRUE;
    _core_textcflag = TRUE;
    _core_current.rasterop = flag;
    return(0);
   }

/****************************************************************************/
define_color_indices(surf, i1, i2, red, grn, blu)
	struct vwsurf *surf;  int i1, i2;  float *red, *grn, *blu;
{
    char *funcname; int i;
    viewsurf *surfp, *_core_VSnsrch();
    ddargtype ddstruct;
    unsigned char ured[256], green[256], blue[256];

    funcname = "define_color_indices";
    if (i1 < minimum.lineindx || i2 > maximum.lineindx || i1 > i2) {
	_core_errhand(funcname,27);
	return(27);
    }
    for (i=0; i<i2-i1+1; i++) {
	ured[i] =  (unsigned char)(*red++ * 255.);
	green[i] = (unsigned char)(*grn++ * 255.);
	blue[i] =  (unsigned char)(*blu++ * 255.);
    }

    if (surfp = _core_VSnsrch(surf)) {
	ddstruct.instance = surfp->vsurf.instance;
	ddstruct.opcode = SETTAB;
	ddstruct.int1 = i1;
	ddstruct.int2 = i2;
	ddstruct.ptr1 = (char*)ured;
	ddstruct.ptr2 = (char*)green;
	ddstruct.ptr3 = (char*)blue;
	(*surfp->vsurf.dd)(&ddstruct);
	}
    else {
	_core_errhand(funcname,5); return(5); }
    return(0);
   }

/****************************************************************************/
set_primitive_attributes(defprim) primattr *defprim;
{
   char *funcname;

   funcname = "set_primitive_attributes";
   /*
    * CHECK FOR VALIDITY OF PRIMITIVE ATTRIBUTE PARAMETERS PASSED.
    */
   if(indefck(defprim->lineindx,minimum.lineindx,maximum.lineindx,funcname)
	== 3) return(3);
   if(indefck(defprim->fillindx,minimum.fillindx,maximum.fillindx,funcname)
	== 3) return(3);
   if(indefck(defprim->textindx,minimum.textindx,maximum.textindx,funcname)
	== 3) return(3);
   if(indefck(defprim->linestyl,minimum.linestyl,maximum.linestyl,
   	funcname) == 3) return(3);
   if(indefck(defprim->polyintstyl,minimum.polyintstyl,maximum.polyintstyl,
   	funcname) == 3) return(3);
   if(indefck(defprim->polyedgstyl,minimum.polyedgstyl,maximum.polyedgstyl,
   	funcname) == 3) return(3);
   if(fldefck(defprim->linwidth,minimum.linwidth,maximum.linwidth,
   	funcname) == 3) return(3);
   if(indefck(defprim->pen,minimum.pen,maximum.pen,funcname) == 3) return(3);
   if(indefck(defprim->font,minimum.font,maximum.font,funcname) == 3) return(3);
   if(fldefck(defprim->charsize.width,minimum.charsize.width,
   	maximum.charsize.width,funcname) == 3) return(3);
   if(fldefck(defprim->charsize.height,minimum.charsize.height,
   	maximum.charsize.height,funcname) == 3) return(3);

   if(fldefck(defprim->chrspace.x,minimum.chrspace.x,maximum.chrspace.x,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrup.x,minimum.chrup.x,maximum.chrup.x,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrup.y,minimum.chrup.y,maximum.chrup.y,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrup.z,minimum.chrup.z,maximum.chrup.z,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.x,minimum.chrpath.x,maximum.chrpath.x,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.y,minimum.chrpath.y,maximum.chrpath.y,
   	funcname) == 3) return(3);
   if(fldefck(defprim->chrpath.z,minimum.chrpath.z,maximum.chrpath.z,
   	funcname) == 3) return(3);

   if(indefck(defprim->chjust,minimum.chjust,maximum.chjust,
   	funcname) == 3) return(3);
   if(indefck(defprim->chqualty,minimum.chqualty,maximum.chqualty,
   	funcname) == 3) return(3);
   if(indefck(defprim->marker,minimum.marker,maximum.marker,
   	funcname) == 3) return(3);
   if(indefck(defprim->pickid,minimum.pickid,maximum.pickid,
   	funcname) == 3) return(3);
   if(indefck(defprim->rasterop,minimum.rasterop,maximum.rasterop,
   	funcname) == 3) return(3);

   /**************************************************/
   /*** DEFAULT PRIMITIVE ATTRIBUTE TRANSFERS      ***/
   /**************************************************/

   _core_current.lineindx = defprim->lineindx;
   _core_current.fillindx = defprim->fillindx;
   _core_current.textindx = defprim->textindx;
   _core_current.linestyl = defprim->linestyl;
   _core_current.polyintstyl = defprim->polyintstyl;
   _core_current.polyedgstyl = defprim->polyedgstyl;
   _core_current.linwidth = defprim->linwidth;
   _core_current.pen = defprim->pen;
   _core_current.font = defprim->font;
   _core_current.charsize = defprim->charsize;
   _core_current.chrup = defprim->chrup;
   _core_current.chrpath = defprim->chrpath;
   _core_current.chrspace = defprim->chrspace;
   _core_current.chjust = defprim->chjust;
   _core_current.chqualty = defprim->chqualty;
   _core_current.marker = defprim->marker;
   _core_current.pickid = defprim->pickid;
   _core_current.rasterop = defprim->rasterop;

    _core_linecflag = TRUE;
    _core_fillcflag = TRUE;
    _core_textcflag = TRUE;
    _core_lsflag = TRUE;
    _core_pisflag = TRUE;
    _core_pesflag = TRUE;
    _core_lwflag = TRUE;
    _core_penflag = TRUE;
    _core_fntflag = TRUE;
    _core_upflag = TRUE;
    _core_pathflag = TRUE;
    _core_justflag = TRUE;
    _core_qualflag = TRUE;
    _core_markflag = TRUE;
    _core_ropflag = TRUE;

   return(0);
   }

/****************************************************************************/
/*     FUNCTION: fldefck                                                    */
/*     PURPOSE: CHECK THE FLOATING POINT DEFAULT PARAMETER TO SEE IF IT     */
/*              FALLS WITHIN IT'S OWN RANGE: MINIMUM < VALUE > MAXIMUM.     */
/****************************************************************************/
static fldefck(value,minval,maxval,funcname) float value,minval,maxval;
   char *funcname;
{
   int errnum;

   if((value < minval) || (value > maxval)) {
      errnum = 13;
      _core_errhand(funcname,errnum);
      return(3);
      }
   return(0);
}

/****************************************************************************/
/*     FUNCTION: indefck                                                    */
/*     PURPOSE: CHECK THE INTEGER DEFAULT PARAMETER TO SEE IF IT            */
/*              FALLS WITHIN IT'S OWN RANGE: MINIMUM < VALUE > MAXIMUM.     */
/****************************************************************************/
static int indefck(value,minval,maxval,funcname)
   int value,minval,maxval;
   char *funcname;
{
   int errnum;

   if((value < minval) || (value > maxval)) {
      errnum = 13;
      _core_errhand(funcname,errnum);
      return(3);
      }
   return(0);
   }
