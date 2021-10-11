#ifndef lint
static char	sccsid[] = "@(#)circarcpat.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI circular arc functions
 */
/*
_cgi_fill_partial_pattern_circle
_cgi_arc_fill_ret_pts
_cgi_fill_partial_pattern_pie_circle
_cgi_partial_pie_circ_fill_pie_ret_pts
*/


#include "cgipriv.h"
#include <math.h>

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
double          pow();
Pixrect        *_cgi_circrect;
#include "cgipriv_arcs.h"

Pixrect        *_cgi_pattern();

#define pw_pencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)	\
(*(dpw)->pw_ops->pro_stencil) ((dpw)->pw_opshandle, 		\
    x - (dpw)->pw_opsx, y - (dpw)->pw_opsy,				\
    w, h, op, stpr, stx, sty, spr, sy, sx)


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pattern_circle 			    */
/*                                                                          */
/* **************************************************************************/
_cgi_fill_partial_pattern_circle(c1, c2, c3, rad, index, flag)
Ccoor          *c1, *c2, *c3;		/* wds 850221: Pointers to center and
					 * contact point */
int             rad;		/* radius */
int             index, flag;
{
    int             x, y, d /* , color */ ;
    int             x1, y1;
    /* geometric info for a circular or elliptical arc */
    struct cgi_arc_geom arc_info;
    int             srad;
    float           m, b;
    int             size;
    Pixrect        *patrect, *outrect;

    _cgi_devscalen(rad, srad);
    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(c2->x, c2->y, arc_info.x2, arc_info.y2);
    _cgi_devscale(c3->x, c3->y, arc_info.x3, arc_info.y3);
    size = 1 + srad + srad;

    /* translate screen coords to coords of retained pixrect */
#define X_TRANSLATION		(x1 - srad)
#define Y_TRANSLATION		(y1 - srad)

    _cgi_circrect = mem_create(size, size, 1);
    /*
     * WDS: _cgi_vws->sunview.depth may not be set to this view surfaces'
     * depth. 
     */
    outrect = mem_create(size, size, _cgi_vws->sunview.depth);
    patrect = _cgi_pattern(index, flag);
    (void) pr_replrop(outrect, 0, 0, size, size, PIX_SRC,
		      patrect, X_TRANSLATION, Y_TRANSLATION);

    if ((arc_info.x3 - arc_info.x2) != 0)
    {
	m = (arc_info.y3 - arc_info.y2);
	m = m / (arc_info.x3 - arc_info.x2);
	b = (arc_info.y3 - Y_TRANSLATION)
	    - m * (arc_info.x3 - X_TRANSLATION);
    }
    else
    {
	m = 0;
	b = (arc_info.x3 - X_TRANSLATION);
    }
    arc_info.xc = x1 - X_TRANSLATION, arc_info.yc = y1 - Y_TRANSLATION;
    arc_info.x2 -= X_TRANSLATION, arc_info.y2 -= Y_TRANSLATION;
    arc_info.x3 -= X_TRANSLATION, arc_info.y3 -= Y_TRANSLATION;
    _cgi_setup_arc_geom(&arc_info, srad, srad, 0);
    x = 0;
    y = srad;
    d = 3 - 2 * srad;
    while (x < y)
    {
	_cgi_arc_fill_ret_pts(&arc_info, srad, 1, x, y, m, b);
	if (d < 0)
	    d += 4 * x + 6;
	else
	{
	    d += 4 * (x - y) + 10;
	    y = y - 1;
	}
	x += 1;
    }
    if (x = y)
	_cgi_arc_fill_ret_pts(&arc_info, srad, 1, x, y, m, b);
    /* translate coords of retained pixrect back to screen coords */
    pw_pencil(_cgi_vws->sunview.pw, X_TRANSLATION, Y_TRANSLATION, size, size,
	      _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
    (void) pr_destroy(patrect);
    (void) pr_destroy(outrect);
/*     (void) pr_destroy (endrect); */
    (void) pr_destroy(_cgi_circrect);
#undef	X_TRANSLATION		(x1 - srad)
#undef	Y_TRANSLATION		(y1 - srad)
}



/****************************************************************************/
/*                                                                          */
/****************************************************************************/

_cgi_arc_fill_ret_pts(arc_geom, rad, color, x, y, m, b)
struct cgi_arc_geom *arc_geom;		/* pointer to geometric info for arc */
short           rad, color, x, y;
float           m, b;
{				/* draw symmetry points of circle */
    short           a1, a2, a3, a4;	/* WDS: xrads */

    a1 = rad + x;
    a2 = rad + y;
    a3 = rad - x;
    a4 = rad - y;

    /* From left to right along a single scanline (yrads) */
    _cgi_vector_arc_clip(a2, a4, rad + x, color, arc_geom, m, b, 0);
    _cgi_vector_arc_clip(a3, a1, rad + y, color, arc_geom, m, b, 0);
    _cgi_vector_arc_clip(a2, a4, rad - x, color, arc_geom, m, b, 0);
    _cgi_vector_arc_clip(a3, a1, rad - y, color, arc_geom, m, b, 0);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pattern_pie_circle 			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_fill_partial_pattern_pie_circle(c1, c2, c3, rad, index, flag)
Ccoor          *c1, *c2, *c3;		/* wds 850221: Pointers to center and
					 * contact point */
int             rad;		/* radius */
int             index, flag;
{
    register short  x, y, d, color;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    int             x1t, y1t;
    int             x2t, y2t;
    int             x3t, y3t;
    int             srad;
    int             size;
    Pixrect        *patrect, *outrect;
    struct cgi_pie_clip_geom _cgi_pie;

    _cgi_devscale(c1->x, c1->y, x1t, y1t);
    _cgi_devscale(c2->x, c2->y, x2t, y2t);
    _cgi_devscale(c3->x, c3->y, x3t, y3t);
    _cgi_devscalen(rad, srad);
    /* _cgi_pcirc_box (x1, y1, x2, y2, x3, y3, srad, &x5, &y5, &x6, &y6,0); */

    /* translate to pixrect coors and build pie clipping structure */
    x1 = srad + 1, y1 = srad + 1;
#define	X_OFFSET	(x1-x1t)/* xt(screen)+X_OFFSET = x(pixrect) */
#define	Y_OFFSET	(y1-y1t)/* yt(screen)+Y_OFFSET = y(pixrect) */
    x2 = x2t + X_OFFSET, y2 = y2t + Y_OFFSET;
    x3 = x3t + X_OFFSET, y3 = y3t + Y_OFFSET;
    _cgi_pie.xc = x1, _cgi_pie.yc = y1;
    _cgi_pie.x2 = x2, _cgi_pie.y2 = y2;
    _cgi_pie.x3 = x3, _cgi_pie.y3 = y3;

    /* removed all the prior-to-850425 crap -- now in "setup pie" function */

    size = 1 + srad + srad;
    _cgi_circrect = mem_create(size, size, 1);
    outrect = mem_create(size, size, _cgi_vws->sunview.depth);
    patrect = _cgi_pattern(index, flag);
    (void) pr_replrop(outrect, 0, 0, size, size, PIX_SRC,
		      patrect, -X_OFFSET, -Y_OFFSET);

    _cgi_setup_pie_geom(&_cgi_pie, srad);
    x = 0;
    y = srad;
    d = 3 - 2 * srad;
    color = _cgi_att->fill.color;
    while (x < y)
    {
	_cgi_partial_pie_circ_fill_pie_ret_pts(srad, srad, &_cgi_pie,
					       color, x, y);
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
	_cgi_partial_pie_circ_fill_pie_ret_pts(srad, srad, &_cgi_pie,
					       color, x, y);
    pw_unlock(_cgi_vws->sunview.pw);
    pw_pencil(_cgi_vws->sunview.pw, -X_OFFSET, -Y_OFFSET, size, size,
	      _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
    (void) pr_destroy(patrect);
    (void) pr_destroy(outrect);
    (void) pr_destroy(_cgi_circrect);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_pie_circ_fill_pie_ret_pts			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

/* WDS: this is it */
_cgi_partial_pie_circ_fill_pie_ret_pts(x0, y0, pie_ptr, color, x, y)
short           x0, y0, color, x, y;
struct cgi_pie_clip_geom *pie_ptr;
/* wds: it gets slopes directly from the pie structure */
{				/* draw symmetry points of circle */
    short           a1, a2, a3, a4, b1, b2, b3, b4;
    a1 = x0 + x;
    a2 = x0 + y;
    a3 = x0 - x;
    a4 = x0 - y;
    b1 = y0 + x;
    b2 = y0 + y;
    b3 = y0 - x;
    b4 = y0 - y;

    /* _cgi_vector_pcirc_pie_clip gets slopes directly from the pie structure */
    _cgi_vector_pcirc_pie_clip(a4, a2, b1, color, pie_ptr, 0);
    _cgi_vector_pcirc_pie_clip(a3, a1, b2, color, pie_ptr, 0);
    _cgi_vector_pcirc_pie_clip(a4, a2, b3, color, pie_ptr, 0);
    _cgi_vector_pcirc_pie_clip(a3, a1, b4, color, pie_ptr, 0);
}
