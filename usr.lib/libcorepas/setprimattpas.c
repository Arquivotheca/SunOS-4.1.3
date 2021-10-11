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
static char sccsid[] = "@(#)setprimattpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"

int define_color_indices();
int set_charjust();
int set_charpath_2();
int set_charpath_3();
int set_charprecision();
int set_charsize();
int set_charspace();
int set_charup_2();
int set_charup_3();
int set_drag();
int set_fill_index();
int set_font();
int set_line_index();
int set_linestyle();
int set_linewidth();
int set_marker_symbol();
int set_pen();
int set_pick_id();
int set_polygon_edge_style();
int set_polygon_interior_style();
int set_primitive_attributes();
int set_rasterop();
int set_text_index();

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

int defcolorindices(surf, i1, i2, red, grn, blu)
    int i1, i2;
    vwsurf *surf;
    double red[], grn[], blu[];

{
    int i;

    /* map array indiced from [1..N] (Pascal convention) to [0..N-1] */
    /* 'C' convention */
    i1--;
    i2--;
    for (i = i1; i <= i2; ++i) {
    	xcoort[i] = red[i];
	ycoort[i] = grn[i];
	zcoort[i] = blu[i];
    }
    return(define_color_indices(surf, i1, i2, xcoort, ycoort,zcoort));
}

int setcharjust(chjust)
int chjust;
	{
	return(set_charjust(chjust));
	}

int setcharpath2(dx, dy)
double dx, dy;
	{
	return(set_charpath_2(dx, dy));
	}

int setcharpath3(dx, dy, dz)
double dx, dy, dz;
	{
	return(set_charpath_3(dx, dy, dz));
	}

int setcharprecision(chqualty)
int chqualty;
	{
	return(set_charprecision(chqualty));
	}

int setcharsize(chwidth, cheight)
double chwidth, cheight;
	{
	float tchwidth, tcheight;
	tchwidth = chwidth;	
	tcheight = cheight;
	return(set_charsize(tchwidth, tcheight));
	}

int setcharspace(space)
double space;
	{
	return(set_charspace(space));
	}

int setcharup2(dx, dy)
double dx, dy;
	{
	return(set_charup_2(dx, dy));
	}

int setcharup3(dx, dy, dz)
double dx, dy, dz;
	{
	return(set_charup_3(dx, dy, dz));
	}

int setdrag(drag)
int drag;
	{
	return(set_drag(drag));
	}

int setfillindex(color)
int color;
	{
	return(set_fill_index(color-1));
	}

int setfont(font)
int font;
	{
	return(set_font(font));
	}

int setlineindex(color)
int color;
	{
	/* Pascal indexes are [1..N] map to [0..N-1] */
	return(set_line_index(color-1));
	}

int setlinestyle(linestyl)
int linestyl;
	{
	return(set_linestyle(linestyl));
	}

int setlinewidth(linwidth)
double linwidth;
	{
	return(set_linewidth(linwidth));
	}

int setmarkersymbol(mark)
int mark;
	{
	return(set_marker_symbol(mark));
	}

int setpen(pen)
int pen;
	{
	return(set_pen(pen));
	}

int setpickid(pickid)
int pickid;
	{
	return(set_pick_id(pickid));
	}

int setpolyedgestyle(polyedgstyl)
int polyedgstyl;
	{
	return(set_polygon_edge_style(polyedgstyl));
	}

int setpolyintrstyle(polyintstyl)
int polyintstyl;
	{
	return(set_polygon_interior_style(polyintstyl));
	}

typedef struct { double x,y,z,w;} pt_type;
typedef struct { double width, height;} aspect;

typedef struct {		/* primitive attribute list structure */
   int lineindx;
   int fillindx;
   int textindx;
   int linestyl;
   int polyintstyl;
   int polyedgstyl;
   float linwidth;
   int pen;
   int font;
   aspect charsize;
   pt_type chrup, chrpath, chrspace;
   int chjust;
   int chqualty;
   int marker;
   int pickid;
   int rasterop;
   } primattr;

typedef struct { double p_x,p_y,p_z,p_w;} p_pt_type;
typedef struct { double p_width, p_height;} p_aspect;

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

int setprimattribs(defprim2)
p_primattr *defprim2;
	{
	primattr *defprim;
		defprim->lineindx = defprim2->p_lineindx - 1;
		defprim->fillindx = defprim2->p_fillindx - 1;
		defprim->textindx = defprim2->p_textindx - 1;
		defprim->linestyl = defprim2->p_linestyl;
		defprim->polyintstyl = defprim2->p_polyintstyl;
		defprim->polyedgstyl = defprim2->p_polyedgstyl;
		defprim->linwidth = defprim2->p_linwidth;
		defprim->pen = defprim2->p_pen;
		defprim->font = defprim2->p_font;
		defprim->charsize.width = defprim2->p_charsize.p_width;
		defprim->charsize.height = defprim2->p_charsize.p_height;
 		defprim->chrup.x = defprim2->p_chrup.p_x;
 		defprim->chrup.y = defprim2->p_chrup.p_y;
 		defprim->chrup.z = defprim2->p_chrup.p_z;
 		defprim->chrup.w = defprim2->p_chrup.p_w;
 		defprim->chrpath.x = defprim2->p_chrpath.p_x;
 		defprim->chrpath.y = defprim2->p_chrpath.p_y;
 		defprim->chrpath.z = defprim2->p_chrpath.p_z;
 		defprim->chrpath.w = defprim2->p_chrpath.p_w;
 		defprim->chrspace.x = defprim2->p_chrspace.p_x;
 		defprim->chrspace.y = defprim2->p_chrspace.p_y;
 		defprim->chrspace.z = defprim2->p_chrspace.p_z;
 		defprim->chrspace.w = defprim2->p_chrspace.p_w;
		defprim->chjust = defprim2->p_chjust;
		defprim->chqualty = defprim2->p_chqualty;
		defprim->marker = defprim2->p_marker;
		defprim->pickid = defprim2->p_pickid;
		defprim->rasterop = defprim2->p_rasterop;
		return(set_primitive_attributes(defprim));
	}

int setrasterop(flag)
int flag;
	{
	return(set_rasterop(flag));
	}

int settextindex(color)
int color;
	{
	return(set_text_index(color-1)); 
	}

