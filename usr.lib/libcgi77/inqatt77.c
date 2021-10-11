#ifndef lint
static char	sccsid[] = "@(#)inqatt77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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
/*
 * Inquire attribute functions
 */

/*
inquire_line_attributes
cgipw_inquire_line_attributes
inquire_marker_attributes
cgipw_inquire_marker_attributes
inquire_fill_area_attributes
cgipw_inquire_fill_area_attributes
inquire_pattern_attributes
cgipw_inquire_pattern_attributes
inquire_perimeter_attributes
cgipw_inquire_perimeter_attributes
inquire_text_attributes
cgipw_inquire_text_attributes
inquire_aspect_source_flags
cgipw_inquire_aspect_source_flags
*/

#include "cgidefs.h"
#include "cf77.h"

Clinatt *inquire_line_attributes ();
Cmarkatt *inquire_marker_attributes ();
Cfillatt *inquire_fill_area_attributes ();
Cpatternatt *inquire_pattern_attributes ();
Ctextatt *inquire_text_attributes ();
Cflaglist *inquire_aspect_source_flags();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqlnatts			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfqlnatts_ (style,width,color,index)
int *style;
float *width;
int *color,*index;
{
    Clinatt   *att;	/* pointer to line attribute structure */

    att = inquire_line_attributes ();
    *style = (int) att->style;
    *width =  att->width;
    *color =  att->color;
    *index =  att->index;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqmkatts			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfqmkatts_ (type,size,color,index)
int *type;
float *size;
int *color,*index;
{
    Cmarkatt  *att;	/* pointer to marker attribute structure */

    att = inquire_marker_attributes ();
    *type = (int) att->type;
    *size =  att->size;
    *color =  att->color;
    *index =  att->index;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqflareaatts				    */
/*                                                                          */
/*                                                                          */
/*		perimeter visbility, fill color, hatch index, pattern index */
/****************************************************************************/
int cfqflareaatts_ (style,vis,color,hindex,pindex,bindex,pstyle,pwidth,pcolor)
int *style, *vis, *color;
int *hindex, *pindex, *bindex;
int *pstyle;
float *pwidth;
int *pcolor;
{
    Cfillatt  *att;	/* pointer to marker attribute structure */

    att = inquire_fill_area_attributes();
    *style = (int) att->style;
    *vis = (int) att->visible;
    *color = att->color;
    *hindex = att->hatch_index;
    *pindex = att->pattern_index;
    *bindex = att->index;
    *pstyle = (int)att->pstyle;
    *pwidth = att->pwidth;
    *pcolor = att->pcolor;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqpatatts			    */
/*                                                                          */
/*                                                                          */
/*		index, row and column count, color list, pattern ref. pt.   */
/*		and pattern size deltas.				    */
/****************************************************************************/
int cfqpatatts_(cindex,row,column,colorlis,x,y,dx,dy)
int *cindex,*row,*column,colorlis[],*x,*y,*dx,*dy;
{
    Cpatternatt  *att;	/* pointer to pattern area attribute structure */

    att = inquire_pattern_attributes ();
    *cindex = att->cur_index;
    *row =att->row;
    *column =att->column;
    RETURN_1ARRAY(colorlis, att->colorlist, (*row) * (*column));
    *x = att->point->x;
    *y = att->point->y;
    *dx =att->dx;
    *dy =att->dy;
    return (NO_ERROR);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqptextatts                                               */
/*                                                                          */
/****************************************************************************/
int cfqtextatts_(fontset,index,cfont,prec,efac,space,color,
	hgt,bx,by,ux,uy,path,halign, valign,hfac,cfac)
int *fontset,*index,*cfont,*prec;
float *efac,*space;
int *color,*hgt;
float *bx,*by,*ux,*uy;
int *path,*halign,*valign;
float *hfac,*cfac;
{
    Ctextatt *att;	/* pointer to text area attribute structure */

    att = inquire_text_attributes();
    *fontset = att->fontset;
    *index = att->index;
    *cfont = att->current_font;
    *prec = (int) att->precision;
    *efac = att->exp_factor;
    *space = att->space;
    *color = att->color;
    *hgt = att->height;
    *bx = att->basex;
    *by = att->basey;
    *ux = att->upx;
    *uy = att->upy;
    *path = (int)att->path;
    *halign =  (int) att->halign;
    *valign = (int) att->valign;
    *hfac = att->hcalind;
    *cfac = att->vcalind;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqasfs 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfqasfs_(n,num,vals)
int *n;
int num[];
int vals[];
{
    Cflaglist *att; /* pointer to flag list structure */

    att = inquire_aspect_source_flags ();
    *n = att->n;
    RETURN_1ARRAY(num, att->num, att->n);
    RETURN_1ARRAY(vals, att->value, att->n);
    return(NO_ERROR);
}
