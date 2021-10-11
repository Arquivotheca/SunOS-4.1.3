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
static char sccsid[] = "@(#)inqprimattpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int inquire_charjust();
int inquire_charpath_2();
int inquire_charpath_3();
int inquire_charprecision();
int inquire_charsize();
int inquire_charspace();
int inquire_charup_2();
int inquire_charup_3();
int inquire_color_indices();
int inquire_fill_index();
int inquire_font();
int inquire_line_index();
int inquire_linestyle();
int inquire_linewidth();
int inquire_marker_symbol();
int inquire_pen();
int inquire_pick_id();
int inquire_polygon_edge_style();
int inquire_polygon_interior_style();
int inquire_primitive_attributes();
int inquire_rasterop();
int inquire_text_index();

int inqcharjust(chjust)
int *chjust;
	{
	return(inquire_charjust(chjust));
	}

int inqcharpath2(dx, dy)
double *dx, *dy;
	{
	float tdx, tdy;
	int f;
	f=inquire_charpath_2(&tdx, &tdy);
	*dx = tdx;
	*dy = tdy;
	return(f);
	}

int inqcharpath3(dx, dy, dz)
double *dx, *dy, *dz;
	{
	float tdx, tdy, tdz;
	int f;
	f=inquire_charpath_3(&tdx, &tdy, &tdz);
	*dx = tdx;
	*dy = tdy;
	*dz = tdz;
	return(f);
	}

int inqcharprecision(chqualty)
int *chqualty;
	{
	return(inquire_charprecision(chqualty));
	}

int inqcharsize(dx, dy)
double *dx, *dy;
	{
	float tdx, tdy;
	int f;
	f=inquire_charsize(&tdx, &tdy);
	*dx = tdx;
	*dy = tdy;
	return(f);
	}

int inqcharspace(space)
double *space;
	{
	float tspace;
	int f;
	f=inquire_charspace(&tspace);
	*space = tspace;
	return(f);
	}

int inqcharup2(dx, dy)
double *dx, *dy;
	{
	float tdx, tdy;
	int f;
	f=inquire_charup_2(&tdx, &tdy);
	*dx = tdx;
	*dy = tdy;
	return(f);
	}

int inqcharup3(dx, dy, dz)
double *dx, *dy, *dz;
	{
	float tdx, tdy, tdz;
	int f;
	f=inquire_charup_3(&tdx, &tdy, &tdz);
	*dx = tdx;
	*dy = tdy;
	*dz = tdz;
	return(f);
	}

#define DEVNAMESIZE 20

typedef struct {
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int inqcolorindices(surf, i1, i2, red, grn, blu)
int i1, i2;
vwsurf *surf;
double red[], grn[], blu[];
{
	int i, result;

	/* Pascal arrays are conventionally indexed from 1, so remap indices*/

	i1--;	/* Pascal arrays are conventionally ... */
	i2--;   /* indexed from 1..N, so remap indices to 0..N-1     */

	result = inquire_color_indices(surf, i1, i2, xcoort, ycoort, zcoort);
	for (i = i1; i <= i2; ++i) {
	    red[i] = xcoort[i];
	    grn[i] = ycoort[i];
	    blu[i] = zcoort[i];
	}
	return(result);
}

int inqfillindex(color)
int *color;
	{
	return(inquire_fill_index(color)+1);
	}

int inqfont(font)
int *font;
	{
	return(inquire_font(font));
	}

int inqlineindex(color)
int *color;
	{
	return(inquire_line_index(color)+1);
	}

int inqlinestyle(linestyl)
int *linestyl;
	{
	return(inquire_linestyle(linestyl));
	}

int inqlinewidth(linwidth)
double *linwidth;
	{
	float tlinwidth;
	int f;
	f=inquire_linewidth(&tlinwidth);
	*linwidth = tlinwidth;
	return(f);
	}

int inqmarkersymbol(mark)
int *mark;
	{
	return(inquire_marker_symbol(mark));
	}

int inqpen(pen)
int *pen;
	{
	return(inquire_pen(pen));
	}

int inqpickid(pickid)
int *pickid;
	{
	return(inquire_pick_id(pickid));
	}

int inqpolyedgestyle(polyedgstyl)
int *polyedgstyl;
	{
	return(inquire_polygon_edge_style(polyedgstyl));
	}

int inqpolyintrstyle(polyintstyl)
int *polyintstyl;
	{
	return(inquire_polygon_interior_style(polyintstyl));
	}

typedef struct { double p_x,p_y,p_z,p_w;} p_pt_type;
typedef struct { double p_width, p_height; }p_aspect;

typedef struct {		/* primitive attribute list structure */
   int p_lineindx;
   int p_fillindx;
   int p_textindx;
   int p_linestyl;
   int p_polyintstyl;
   int p_polyedgstyl;
   double p_linwidth;
   int p_pen;
   int p_font;
   p_aspect p_charsize;
   p_pt_type p_chrup, p_chrpath, p_chrspace;
   int p_chjust;
   int p_chqualty;
   int p_marker;
   int p_pickid;
   int p_rasterop;
   } p_primattr;

typedef struct { float  x, y, z, w;}  pt_type;
typedef struct { float  width,  height; } aspect;

typedef struct {		/* primitive attribute list structure */
   int  lineindx;
   int  fillindx;
   int  textindx;
   int  linestyl;
   int  polyintstyl;
   int  polyedgstyl;
   float  linwidth;
   int  pen;
   int  font;
    aspect  charsize;
    pt_type  chrup,  chrpath,  chrspace;
   int  chjust;
   int  chqualty;
   int  marker;
   int  pickid;
   int  rasterop;
   }  primattr;

int inqprimattribs(defprim2)
p_primattr *defprim2;
	{
	primattr defprim;
	int f;
		f=inquire_primitive_attributes(&defprim);
		defprim2->p_lineindx = defprim.lineindx + 1;
		defprim2->p_fillindx = defprim.fillindx + 1;
		defprim2->p_textindx = defprim.textindx + 1;
		defprim2->p_linestyl = defprim.linestyl;
		defprim2->p_polyintstyl = defprim.polyintstyl;
		defprim2->p_polyedgstyl = defprim.polyedgstyl;
		defprim2->p_linwidth = defprim.linwidth;
		defprim2->p_pen = defprim.pen;
		defprim2->p_font = defprim.font;
		defprim2->p_charsize.p_width =defprim.charsize.width;
		defprim2->p_charsize.p_height =defprim.charsize.height;
 		defprim2->p_chrup.p_x = defprim.chrup.x;
 		defprim2->p_chrup.p_y = defprim.chrup.y;
 		defprim2->p_chrup.p_z = defprim.chrup.z;
 		defprim2->p_chrup.p_w = defprim.chrup.w;
 		defprim2->p_chrpath.p_x = defprim.chrpath.x;
 		defprim2->p_chrpath.p_y = defprim.chrpath.y;
 		defprim2->p_chrpath.p_z = defprim.chrpath.z;
 		defprim2->p_chrpath.p_w = defprim.chrpath.w;
 		defprim2->p_chrspace.p_x = defprim.chrspace.x;
 		defprim2->p_chrspace.p_y = defprim.chrspace.y;
 		defprim2->p_chrspace.p_z = defprim.chrspace.z;
 		defprim2->p_chrspace.p_w = defprim.chrspace.w;
		defprim2->p_chjust = defprim.chjust;
		defprim2->p_chqualty = defprim.chqualty;
		defprim2->p_marker = defprim.marker;
		defprim2->p_pickid = defprim.pickid;
		defprim2->p_rasterop = defprim.rasterop;
		return(f);
	}

int inqrasterop(rasterop)
int *rasterop;
	{
	return(inquire_rasterop(rasterop));
	}

int inqtextindex(color)
int *color;
	{
	return(inquire_text_index(color)+1);
	}
