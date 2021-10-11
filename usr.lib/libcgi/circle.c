#ifndef lint
static char	sccsid[] = "@(#)circle.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI circle functions
 */

/*
circle
cgipw_circle
_cgi_circle
_cgi_inner_circle
_cgi_circ_pts
_cgi_pattern_circle
_cgi_circ_ret_pts
*/

/*
bugs:
textured circle
VP's algorithm
error numbers
*/

#include "cgipriv.h"		/* defines types used in this file  */
#ifndef window_hs_DEFINED
#include <sunwindow/window_hs.h>
#endif

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
Pixrect        *_cgi_circrect;
Pixrect        *_cgi_pattern();

/* Like pw_put, but uses RasterOp code */
#define PW_PUT_WITH_OP(pw,x,y,op,color)	pw_vector(pw, x,y, x,y, op, color)

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: circle	 					            */
/*                                                                          */
/*		draws a circle of radius rad centered at c1		    */
/****************************************************************************/

Cerror          circle(c1, rad)
Ccoor          *c1;		/* center */
Cint            rad;		/* radius */
{
    int             ivw;
    Cerror          err;	/* error */
    Ccoor           ca;

    err = _cgi_err_check_4();
    if (!err)
    {
	ca = *c1;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_circle(_cgi_vws, &ca, rad);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_circle 					            */
/*                                                                          */
/*		draws a circle of radius rad centered at c1		    */
/****************************************************************************/

Cerror          cgipw_circle(desc, c1, rad)
Ccgiwin        *desc;
Ccoor          *c1;		/* center */
Cint            rad;		/* radius */
{
    SETUP_CGIWIN(desc);
    return (_cgi_circle(desc->vws, c1, rad));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circle 					            */
/*                                                                          */
/*		draws a circle of radius rad centered at c1		    */
/****************************************************************************/

Cerror          _cgi_circle(vws, c1, rad)
View_surface   *vws;
Ccoor          *c1;		/* center */
Cint            rad;		/* radius */
{
    register Outatt *attP = vws->att;
    int             x1, y1, srad;
    int             color, width;
    Cerror          err = NO_ERROR;
    Clintype        style;
/*     Rect subrect; */

    if (rad < 0)		/* radius is always interpreted as positive as
				 * per standard */
	rad = -rad;
    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscalen(rad, srad);
/*		rad2 = (srad + srad)++;
		subrect.r_left = (short)(x1 - srad);
		subrect.r_top = (short)(y1 - srad);
		subrect.r_width = (short)(rad2);
		subrect.r_height = (short)(rad2);
     */
    style = attP->fill.pstyle;
    color = attP->fill.pcolor;
    width = vws->conv.perimeter_width;
    if (rad > 0)
    {
	pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
	if ((style == SOLID) && (attP->fill.style == SOLIDI) &&
	    (attP->fill.visible == ON) &&
	    (attP->fill.color == attP->fill.pcolor)
	    )			/* special case for solid circle */
	    _cgi_inner_circle(vws, x1, y1, srad + width,
			      attP->fill.color);
	else
	{			/* fill and draw perimeter  */
	    if (width > srad)
		width = srad;
	    else if (width <= 0)
		width = 1;
/* 	    srad -= width; */
	    switch (attP->fill.style)
	    {
	    case SOLIDI:
		{
		    _cgi_inner_circle(vws, x1, y1, srad,
				      attP->fill.color);
		    break;
		}
	    case HOLLOW:	/* will need to be changed for full
				 * transparency in next CGI */
		{
		    break;
		}
	    case PATTERN:
		{
		    _cgi_pattern_circle(vws, x1, y1, srad,
					attP->fill.color,
					attP->fill.pattern_index, 1);
		    break;
		}
	    case HATCH:
		{
		    _cgi_pattern_circle(vws, x1, y1, srad,
					attP->fill.color,
					attP->fill.hatch_index, 0);
		    break;
		}
	    }
	    srad += width;
	    if (attP->fill.visible == ON)
	    {
		if (srad)
		    _cgi_angle_textured_circle(x1, y1, srad,
					       width, color, style);
	    }
	}
	pw_unlock(vws->sunview.pw);
    }
    else			/* special case for point */
	PW_PUT_WITH_OP(vws->sunview.pw, x1, y1, _cgi_pix_mode, color);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_inner_circle 					    */
/*                                                                          */
/*		draws a solid circle with coors in screen space		    */
/*		Algorithm from Michie by way of Foley and Can Dam	    */
/****************************************************************************/
int             _cgi_inner_circle(vws, x0, y0, rad, val)
View_surface   *vws;
short           x0, y0, rad, val;
{
    register int    x, y, d;

    x = 0;
    d = -rad;
    pw_vector(vws->sunview.pw, x0 - rad, y0, x0 + rad, y0, _cgi_pix_mode, val);
    for (y = rad; y >= 0; y--)
    {
	while (d <= 0)
	{
	    d += 2 * x + 1;
	    x++;
	}
	d -= 2 * y - 1;
	_cgi_circ_pts(vws, x0, y0, x, y, val);
    }
}



/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circ_pts	 					    */
/*                                                                          */
/*		draws scan lines of solid circle using symetry		    */
/****************************************************************************/
_cgi_circ_pts(vws, x0, y0, x, y, color)
View_surface   *vws;
short           x0, y0, x, y, color;
{				/* draw symmetry points of circle */
    int             a1, a3, b2, b4;
    a1 = x0 + x;
    a3 = x0 - x;
    b2 = y0 + y;
    b4 = y0 - y;
    pw_vector(vws->sunview.pw, a3, b2, a1, b2, _cgi_pix_mode, color);
    pw_vector(vws->sunview.pw, a3, b4, a1, b4, _cgi_pix_mode, color);
}

#define pw_pencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)     \
    (*(dpw)->pw_ops->pro_stencil) ((dpw)->pw_opshandle,           \
    x - (dpw)->pw_opsx, y - (dpw)->pw_opsy,                         \
    w, h, op, stpr, stx, sty, spr, sy, sx)
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pattern_circle 					 */
/*                                                                       */
/* 		draws a pattern circle with coors in screen space	 */
/* ***********************************************************************/
int             _cgi_pattern_circle(vws, x0, y0, rad, val, index, flag)
View_surface   *vws;
short           x0, y0, rad, val, index, flag;
{
    int             x, y, d, size;
    Pixrect        *patrect, *outrect;

    size = 1 + rad + rad;
    _cgi_circrect = mem_create(size, size, 1);
    outrect = mem_create(size, size, vws->sunview.depth);
    patrect = _cgi_pattern(index, flag);
    (void) pr_replrop(outrect, 0, 0, size, size, PIX_SRC,
		      patrect, x0 - rad, y0 - rad);	/* ULC of outrect on
							 * screen */
    (void) pr_destroy(patrect);
    x = 0;
    y = rad;
    d = 3 - 2 * rad;
    while (x < y)
    {
	_cgi_circ_ret_pts(rad, rad, x, y, val);
	if (d < 0)
	    d = d + 4 * x + 6;
	else
	{
	    d = d + 4 * (x - y) + 10;
	    y = y - 1;
	}
	x += 1;
    }
    if (x = y)
	_cgi_circ_ret_pts(rad, rad, x, y, val);
/* 	pw_write(vws->sunview.pw,x0-rad,y0-rad,size,size,_cgi_pix_mode,_cgi_circrect,0,0);  */
    /*
     * Except for symmetry of always locking in the same place in the code, and
     * except for locking around the perimeter with the same lock, we wouldn't
     * have to lock, except around the stencil below
     */
    pw_pencil(vws->sunview.pw, x0 - rad, y0 - rad, size, size,
	      _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
    (void) pr_destroy(outrect);
    (void) pr_destroy(_cgi_circrect);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circ_ret_pts	 				    */
/*                                                                          */
/*		draws non-closed _cgi_circ_ret_pts		            */
/****************************************************************************/

int             _cgi_circ_ret_pts(x0, y0, x, y, color)
short           x0, y0, x, y, color;
{				/* draw symmetry points of circle */
    int             a1, a2, a3, a4, b1, b2, b3, b4;
    int             bop;

    bop = PIX_OPCOLOR(255) | PIX_SRC;
    a1 = x0 + x;
    a2 = x0 + y;
    a3 = x0 - x;
    a4 = x0 - y;
    b1 = y0 + x;
    b2 = y0 + y;
    b3 = y0 - x;
    b4 = y0 - y;
    (void) pr_vector(_cgi_circrect, a4, b1, a2, b1, bop, color);
    (void) pr_vector(_cgi_circrect, a3, b2, a1, b2, bop, color);
    (void) pr_vector(_cgi_circrect, a4, b3, a2, b3, bop, color);
    (void) pr_vector(_cgi_circrect, a3, b4, a1, b4, bop, color);
}
