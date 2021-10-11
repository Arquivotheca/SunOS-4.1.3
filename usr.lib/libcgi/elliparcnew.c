#ifndef lint
static char	sccsid[] = "@(#)elliparcnew.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI elliptical arc functions
 */

/*
elliptical_arc_close
cgipw_elliptical_arc_close
_cgi_elliptical_arc_close
_cgi_get_ellip_angle
elliptical_arc
cgipw_elliptical_arc
_cgi_elliptical_arc
_cgi_draw_elliptical_arc
_cgi_partial_textured_ellipse
_cgi_partial_ang_textured_ellip_pts
_cgi_fill_partial_ellipse
_cgi_partial_ellip_fill_pts
_cgi_fill_partial_pie_ellipse
_cgi_partial_pie_ellip_fill_pts
_cgi_fill_partial_pattern_ellipse
_cgi_fill_partial_pattern_pie_ellipse
_cgi_partial_pie_ellip_fill_ret_pts
*/
/*
bugs:
error check 65
float/opt
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
#include <math.h>
#include "cgipriv_arcs.h"

short           _cgi_texture[8];
static int      pattern2e[6][9] =
{
    {
	180, 90, 0, 0, 0, 0, 0, 0, 0
    },				/* solid  */
    {
	2, 2, 2, 2, 2, 2, 2, 2, 16
    },				/* dotted */
    {
	6, 2, 6, 2, 6, 2, 6, 2, 32
    },				/* dashed */
    {
	8, 2, 2, 2, 8, 2, 2, 2, 24
    },				/* dash dot */
    {
	8, 2, 2, 2, 2, 2, 2, 2, 18
    },				/* dash dot dotted */
    {
	10, 10, 10, 10, 10, 10, 10, 10, 80
    }
};				/* long dashed */

#define pw_pencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)	\
(*(dpw)->pw_ops->pro_stencil) ((dpw)->pw_opshandle, \
  x - (dpw)->pw_opsx, y - (dpw)->pw_opsy, \
  w, h, op, stpr, stx, sty, spr, sy, sx)

static int      g_xrad, g_yrad, g_flag;
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: elliptical_arc_close					    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
int             elliptical_arc_close(c1, sx, sy, ex, ey, majx, miny, close)
Ccoor          *c1;		/* center */
int             sx, sy;		/* starting point of arc */
int             ex, ey;		/* ending point of arc */
int             majx, miny;	/* enpoints of major and minor axes */
Cclosetype      close;
{
    int             ivw;
    Cerror          err;	/* error */
    Ccoor           cb;
    Ccoor           spt, ept, c2, c3;	/* center and contact point */

    err = _cgi_err_check_4();
    if (!err)
    {
	cb = *c1;
	spt.x = sx;
	spt.y = sy;
	ept.x = ex;
	ept.y = ey;
	c2.x = majx;
	c2.y = 0;
	c3.x = 0;
	c3.y = miny;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_elliptical_arc_close(_cgi_vws,
					    c1, &spt, &ept, &c2, &c3, close);
	    cb = *c1;
	}
    }
    return (_cgi_errhand(err));
}



/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_elliptical_arc_close				    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
int             cgipw_elliptical_arc_close(desc, c1, sx, sy, ex, ey, majx, miny, close)
Ccgiwin        *desc;
Ccoor          *c1;		/* center */
int             sx, sy;		/* starting point of arc */
int             ex, ey;		/* ending point of arc */
int             majx, miny;	/* enpoints of major and minor axes */
Cclosetype      close;
{
    Ccoor           spt, ept, c2, c3;	/* center and contact point */

    SETUP_CGIWIN(desc);
    spt.x = sx;
    spt.y = sy;
    ept.x = ex;
    ept.y = ey;
    c2.x = majx;
    c2.y = 0;
    c3.x = 0;
    c3.y = miny;
    return (_cgi_elliptical_arc_close(desc->vws,
				    c1, &spt, &ept, &c2, &c3, close));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_elliptical_arc_close				    */
/*                                                                          */
/*		Draws an ellipse centered at c1 with major and minor axes   */
/*              with endpoints at c2 and c3. Spt and ept are the start and  */
/*              ending points of the arc.                                   */
/****************************************************************************/
int             _cgi_elliptical_arc_close(vws, c1, spt, ept, c2, c3, close)
View_surface   *vws;
Ccoor          *c1;		/* center point */
Ccoor          *spt, *ept;	/* start and end points of the arc */
Ccoor          *c2, *c3;	/* endpoints of major and minor axes */
Cclosetype      close;
{
    register View_surface *vwsP = vws;
    register Outatt *attP = vws->att;
    Cerror          err = NO_ERROR;
    static Clinatt  templin;
    int             itemp;
    Ccoor	    pts[3];

    pw_lock(vwsP->sunview.pw, &vwsP->sunview.lock_rect);
    _cgi_get_ellip_angle(c1, c2, c3);
    /*
     * WDS: does _cgi_devscale (c1->x, c1->y, x1, y1), which is redundant with *
     * that in _cgi_fill_partial_ellipse &  _cgi_fill_partial_pattern_ellipse
     */
/*     err += _cgi_check_ang (c1, c2, rad); */
/*     err += _cgi_check_ang (c1, c3, rad); */
    if (err == NO_ERROR)
    {
	if (attP->fill.visible == ON)
	{
	    templin = attP->line;
	    attP->line.style = attP->fill.pstyle;
	    attP->line.width = attP->fill.pwidth;
	    itemp = vwsP->conv.line_width;
	    vwsP->conv.line_width = vwsP->conv.perimeter_width;
	    attP->line.color = attP->fill.pcolor;
	}
	switch (close)
	{
	case (CHORD):
	    {
		switch (attP->fill.style)
		{
		case SOLIDI:
		    _cgi_fill_partial_ellipse(vwsP, c1, spt, ept, c2, c3);
		    break;
		case HOLLOW:
		    break;
		case PATTERN:
		    _cgi_fill_partial_pattern_ellipse(vwsP, c1,
						      spt, ept, c2, c3,
						      attP->fill.pattern_index,
						      1);
		    break;
		case HATCH:
		    _cgi_fill_partial_pattern_ellipse(vwsP, c1,
						      spt, ept, c2, c3,
						      attP->fill.hatch_index,
						      0);
		    break;
		}
		if (attP->fill.visible == ON)
		{
		    pts[0] = *spt;
		    pts[1] = *ept;
		    err = _cgi_polyline(vws, 2, pts, POLY_DONTCLOSE, XFORM);
		}
		break;
	    }
	case (PIE):
	    {
		switch (attP->fill.style)
		{
		case SOLIDI:
		    _cgi_fill_partial_pie_ellipse(vwsP, c1,
						  spt, ept, c2, c3);
		    break;
		case HOLLOW:
		    break;
		case PATTERN:
		    _cgi_fill_partial_pattern_pie_ellipse(vwsP, c1,
							  spt, ept, c2, c3,
						     attP->fill.pattern_index,
							  1);
		    break;
		case HATCH:
		    _cgi_fill_partial_pattern_pie_ellipse(vwsP, c1,
							  spt, ept, c2, c3,
						       attP->fill.hatch_index,
							  0);
		    break;
		}
		if (attP->fill.visible == ON)
		{
		    pts[0] = *spt;
                    pts[1] = *c1;
		    pts[2] = *ept;
                    err = _cgi_polyline(vws, 3, pts, POLY_DONTCLOSE, XFORM);
		}
		break;
	    }
	};
	if (err == 0  &&  attP->fill.visible == ON)
	{
	    err = _cgi_draw_elliptical_arc(vwsP, c1, spt, ept, c2, c3);
	    attP->line = templin;
	    vwsP->conv.line_width = itemp;
	}
    }
    pw_unlock(vwsP->sunview.pw);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_get_ellip_angle					    */
/*                                                                          */
/****************************************************************************/
_cgi_get_ellip_angle(c1, c2, c3)
Ccoor           *c1, *c2, *c3;
{
    short           x0, y0, x1, y1, x2, y2;
    short           llx, lly, llx2, lly2;
    short           r1, r2;
    _cgi_devscale(c1->x, c1->y, x0, y0);
    _cgi_devscale(c2->x, c2->y, x1, y1);
    _cgi_devscale(c3->x, c3->y, x2, y2);
    llx = x1 - x0;		/* distances from center */
    lly = y1 - y0;
    llx2 = x2 - x0;
    lly2 = y2 - y0;
    if (!llx || !llx2)
    {				/* ellipse is aligned with x and y axes */
	/* better error check */
	if (llx == 0)
	{
	    r1 = abs(lly);
	    g_flag = 0;
	}
	else
	{
	    r1 = abs(llx);
	    g_flag = 1;
	}
	if (llx2 == 0)
	    r2 = abs(lly2);
	else
	    r2 = abs(llx2);
	g_xrad = r1;
	g_yrad = r2;
    }
    /* WDS: should assure that this doesn't happen or return error? */
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: elliptical_arc		 				    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
Cerror          elliptical_arc(c1, sx, sy, ex, ey, majx, miny)
Ccoor          *c1;		/* center */
Cint            sx, sy;		/* starting point of arc */
Cint            ex, ey;		/* ending point of arc */
Cint            majx, miny;	/* enpoints of major and minor axes */
{
    int             ivw, err;	/* error */
    Ccoor           cb;
    Ccoor           spt, ept, c2, c3;	/* center and contact point */

    err = _cgi_err_check_4();
    if (!err)
    {
	cb = *c1;
	spt.x = sx;
	spt.y = sy;
	ept.x = ex;
	ept.y = ey;
	c2.x = majx;
	c2.y = 0;
	c3.x = 0;
	c3.y = miny;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_elliptical_arc(_cgi_vws, c1, &spt, &ept, &c2, &c3);
	    *c1 = cb;
	}
    }
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_elliptical_arc	 				    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
Cerror          cgipw_elliptical_arc(desc, c1, sx, sy, ex, ey, majx, miny)
Ccgiwin        *desc;
Ccoor          *c1;		/* center */
Cint            sx, sy;		/* starting point of arc */
Cint            ex, ey;		/* ending point of arc */
Cint            majx, miny;	/* enpoints of major and minor axes */
{
    Ccoor           spt, ept, c2, c3;	/* center and contact point */
    int             err;

    SETUP_CGIWIN(desc);
    spt.x = sx;
    spt.y = sy;
    ept.x = ex;
    ept.y = ey;
    c2.x = majx;
    c2.y = 0;
    c3.x = 0;
    c3.y = miny;
    err = _cgi_elliptical_arc(desc->vws, c1, &spt, &ept, &c2, &c3);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_elliptical_arc	 				    */
/*                                                                          */
/*		interface to _cgi_draw_elliptical_arc that locks	    */
/*              around the call.					    */
/****************************************************************************/
Cerror          _cgi_elliptical_arc(vws, c1, spt, ept, c2, c3)
View_surface   *vws;
Ccoor           *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
{
    Cerror          err;

    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
    err = _cgi_draw_elliptical_arc(vws, c1, spt, ept, c2, c3);
    pw_unlock(vws->sunview.pw);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_draw_elliptical_arc 				    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
Cerror          _cgi_draw_elliptical_arc(vws, c1, spt, ept, c2, c3)
View_surface   *vws;
Ccoor          *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
/* wds: style, color, and width should be an argument: perhaps in a lineatt */
{
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    struct cgi_arc_geom arc_geom;
    int             color, width;
    int             xrad, yrad;
    Clintype        style;

    _cgi_get_ellip_angle(c1, c2, c3);
    _cgi_devscale(c1->x, c1->y, x1, y1);	/* WDS: redundant scaling */
    _cgi_devscale(spt->x, spt->y, x2, y2);
    _cgi_devscale(ept->x, ept->y, x3, y3);
    style = vws->att->line.style;
    color = vws->att->line.color;
    width = vws->conv.line_width;
    if (width < 1)
	width = 1;		/* Minimum, one pixel */
    _cgi_devscalen(c2->x, xrad);
    _cgi_devscalen(c3->y, yrad);
/* 	srad += width / 2; */

    arc_geom.xc = x1, arc_geom.yc = y1;
    arc_geom.x2 = x2, arc_geom.y2 = y2;
    arc_geom.x3 = x3, arc_geom.y3 = y3;
    _cgi_setup_arc_geom(&arc_geom, xrad, yrad, width);
    _cgi_partial_textured_ellipse(&arc_geom, xrad, yrad, width, color, style);

    return (NO_ERROR);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_textured_ellipse 				    */
/*                                                                          */
/*		draws ellipse with coors in screen space		    */
/****************************************************************************/
_cgi_partial_textured_ellipse(arc_geom, xrad, yrad, width, val, style)
struct cgi_arc_geom *arc_geom;
int             xrad, yrad;
int             width;
int             val;
Clintype        style;
{
    register int    x, y;
    float           a, b, f, f1;
    short           r1, r2;
    short           x1, y1;
    short           inc;
/*     int     width; */
    float           convf, oconvf, angsum, angsum1;
    int             i, k, rada, yrada;
    float           a1, b1;

/*	val = PIX_SRC | PIX_DST; */
    x = 0;
/*     xrad--; */
/*     yrad--; */
    r1 = yrad;
    r2 = xrad;
    a = r2;
    a = r1 / a;
    a = a * a;
    b = 1.0;
    f = 0;
    inc = 0;
    for (y = r1; y >= 0; y--)
    {
	f -= 2 * b * y - b;
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    inc++;
	    x++;
	}
	inc++;
    }
    rada = r1 - width + 1;
    yrada = r2 - width + 1;
    oconvf = 90.0 / inc;	/* outside length */
    x = 0;
    y = rada;
    a1 = yrada;
    a1 = rada / a1;
    a1 = a1 * a1;
    b1 = 1.0;
    f1 = 0;
    inc = 0;
    for (y = rada; y >= 0; y--)
    {
	f1 -= 2 * b1 * y - b1;
	while (f1 < 0)
	{
	    f1 += 2 * a1 * x + a1;
	    inc++;
	    x++;
	}
	inc++;
    }
    convf = 90.0 / inc;		/* outside length */
    x = 0;
    x1 = 0;
    y = r1;
    y1 = rada;
    f1 = 0;
    f = 0;
    for (k = 0; k < 8; k++)
	_cgi_texture[k] = 1;
    i = 0;
    angsum = pattern2e[(int) style][0] / 2;
    angsum1 = 0.0;
    if (_cgi_texture[1] == 1)
	_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
    angsum1 = pattern2e[(int) style][0] / 2;
    for (y = r1; y >= 0; y--)
    {
	f -= 2 * b * y - b;
	if (_cgi_texture[1] == 1)
	    _cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    inc++;
	    if (_cgi_texture[1] == 1)
		_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	    x++;
	    angsum += oconvf;
	    if (_cgi_texture[1] == 1)
		_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	}
	angsum += oconvf;
	while (angsum1 <= angsum)
	{
	    f1 -= 2 * b1 * y1 - b1;
	    if (_cgi_texture[1] == 1)
		_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
/* 	    while ((f1 < 0) && (angsum1 <= angsum)) {  */
	    while ((f1 < 0))
	    {			/* extra test neccessary to prevent texture
				 * dependent errors */
		f1 += 2 * a1 * x1 + a1;
		if (_cgi_texture[1] == 1)
		    _cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
		x1++;
		angsum1 += convf;
		if (_cgi_texture[1] == 1)
		    _cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	    }
/* 	    if ((angsum1 <= angsum) ) { */
	    if (_cgi_texture[1] == 1)
		_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	    y1--;
	    if (_cgi_texture[1] == 1)
		_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, val);
	    angsum1 += convf;
/* 	    } */
/*          } */
	}
/* 	angsum1 += oconvf; */
	if (angsum >= pattern2e[(int) style][i])
	{
	    i++;
	    if ((i % 2) == 0)
		for (k = 0; k < 8; k++)
		    _cgi_texture[k] = 1;
	    else
		for (k = 0; k < 8; k++)
		    _cgi_texture[k] = 0;
	    angsum1 -= angsum;
	    angsum = 0;
	    i &= 7;
	}
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_ang_textured_ellip_pts			    */
/*                                                                          */
/*              removed x0,y0; only calls give it arc_geom.xc & yc	    */
/****************************************************************************/
_cgi_partial_ang_textured_ellip_pts(x, y, x1, y1, arc_geom, color)
short           x, y, color;
struct cgi_arc_geom *arc_geom;
{				/* draw symmetry points of ellipse */
    short           a1, a3, b2, b4;
    short           ra1, ra3, rb2, rb4;
    a1 = arc_geom->xc + x;
    ra1 = arc_geom->xc + x1;
    a3 = arc_geom->xc - x;
    ra3 = arc_geom->xc - x1;
    b2 = arc_geom->yc + y;
    rb2 = arc_geom->yc + y1;
    b4 = arc_geom->yc - y;
    rb4 = arc_geom->yc - y1;
	_cgi_pcirc_vector(a3, b2, ra3, rb2, color, arc_geom);
	_cgi_pcirc_vector(a1, b2, ra1, rb2, color, arc_geom);
	_cgi_pcirc_vector(a3, b4, ra3, rb4, color, arc_geom);
	_cgi_pcirc_vector(a1, b4, ra1, rb4, color, arc_geom);
}

/* Now using _cgi_pcirc_vector (in file circarc2.c) */


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_ellipse	 			    */
/*                                                                          */
/*		draws non-closed _cgi_fill_partial_ellipse		    */
/****************************************************************************/
_cgi_fill_partial_ellipse(vws, c1, spt, ept, c2, c3)
View_surface   *vws;
Ccoor           *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
{
    int             x, y, color;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    struct cgi_arc_geom arc_geom;
    float           m, b;
    float           f, a, b1;
    int             rd, rd2;

    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(spt->x, spt->y, x2, y2);
    _cgi_devscale(ept->x, ept->y, x3, y3);
    _cgi_devscalen(c2->x, g_xrad);
    _cgi_devscalen(c3->y, g_yrad);
    if ((x3 - x2) != 0)
    {
	m = (y3 - y2);		/* don't worry, drawing scanlines which are
				 * horizontal anyway */
	m = m / (x3 - x2);
	b = y3 - m * x3;
    }
    else
    {
	m = 0;
	b = x3;
    }
    arc_geom.xc = x1, arc_geom.yc = y1;
    arc_geom.x2 = x2, arc_geom.y2 = y2;
    arc_geom.x3 = x3, arc_geom.y3 = y3;
    _cgi_setup_arc_geom(&arc_geom, g_xrad, g_yrad, 0);
    rd = g_yrad;
    rd2 = g_xrad;
    color = vws->att->fill.color;
    x = 0;
    a = rd2;
    a = rd / a;
    a = a * a;
    b1 = 1.0;
    y = rd;
    x = 0;
    f = 0;
    _cgi_partial_ellip_fill_pts(x1, y1, &arc_geom, color, x, y, m, b, 1);
    for (y = g_yrad; y >= 0; y--)
    {
	f -= 2 * b1 * y - b1;
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    x++;
	    _cgi_partial_ellip_fill_pts(x1, y1, &arc_geom, color, x, y, m, b, 1);
	}
	_cgi_partial_ellip_fill_pts(x1, y1, &arc_geom, color, x, y, m, b, 1);
    }
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_ellip_fill_pts 				    */
/*                                                                          */
/*		draws symmetry points of ellipse			    */
/****************************************************************************/

_cgi_partial_ellip_fill_pts(x0, y0, arc_geom, color, x, y, m, b, flag)
short           x0, y0, color, x, y;
struct cgi_arc_geom *arc_geom;
float           m, b;
short           flag;
{				/* draw symmetry points of ellipse */
    _cgi_vector_arc_clip(x0 - x, x0 + x, y0 + y, color, arc_geom, m, b, flag);
    _cgi_vector_arc_clip(x0 - x, x0 + x, y0 - y, color, arc_geom, m, b, flag);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pie_ellipse	 			    */
/*                                                                          */
/*		draws non-closed _cgi_fill_partial_pie_ellipse		    */
/****************************************************************************/

_cgi_fill_partial_pie_ellipse(vws, c1, spt, ept, c2, c3)
View_surface   *vws;
Ccoor           *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
{
    int             x, y, color;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    float           f, /* fx, fy, fxx, fyy, */ a, b2 /* , y1r */ ;
    int             rd, rd2;
    struct cgi_pie_clip_geom _cgi_pie;

    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(spt->x, spt->y, x2, y2);
    _cgi_devscale(ept->x, ept->y, x3, y3);
    _cgi_devscalen(c2->x, g_xrad);
    _cgi_devscalen(c3->y, g_yrad);
    _cgi_pie.xc = x1, _cgi_pie.yc = y1;
    _cgi_pie.x2 = x2, _cgi_pie.y2 = y2;
    _cgi_pie.x3 = x3, _cgi_pie.y3 = y3;
    _cgi_setup_pie_geom(&_cgi_pie, g_yrad);

    color = vws->att->fill.color;
    rd = g_yrad;
    rd2 = g_xrad;
    a = rd2;
    a = rd / a;
    a = a * a;
    b2 = 1.0;

    y = rd;
    x = 0;
    f = 0;
    _cgi_partial_pie_ellip_fill_pts(x1, y1, &_cgi_pie, color, x, y);
    for (y = g_yrad; y >= 0; y--)
    {
	f -= 2 * b2 * y - b2;
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    x++;
	    _cgi_partial_pie_ellip_fill_pts(x1, y1, &_cgi_pie, color, x, y);
	}
	_cgi_partial_pie_ellip_fill_pts(x1, y1, &_cgi_pie, color, x, y);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_pie_ellip_fill_pts			    */
/*                                                                          */
/*		draws non-closed _cgi_partial_pie_ellip_pts		    */
/****************************************************************************/
_cgi_partial_pie_ellip_fill_pts(x0, y0, pie_ptr, color, x, y)
short           x0, y0, color, x, y;
struct cgi_pie_clip_geom *pie_ptr;
{				/* draw symmetry points of ellipse */
    short           a1, a3, b2, b4;
    a1 = x0 + x;
    a3 = x0 - x;
    b2 = y0 + y;
    b4 = y0 - y;
    _cgi_vector_pcirc_pie_clip(a3, a1, b2, color, pie_ptr, 1);
    _cgi_vector_pcirc_pie_clip(a3, a1, b4, color, pie_ptr, 1);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pattern_ellipse	 		    */
/*                                                                          */
/*		draws non-closed _cgi_fill_partial_pattern_ellipse	    */
/****************************************************************************/
_cgi_fill_partial_pattern_ellipse(vws, c1, spt, ept, c2, c3, index, flag)
View_surface   *vws;
Ccoor           *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
int             index, flag;
{
    int             x, y, color;
    int             x1, y1;
    struct cgi_arc_geom arc_geom;
    float           m, b;
    Pixrect        *patrect, *outrect;
    float           f, a, b1;
    int             rd, rd2;
    int             sizex, sizey;

    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(spt->x, spt->y, arc_geom.x2, arc_geom.y2);
    _cgi_devscale(ept->x, ept->y, arc_geom.x3, arc_geom.y3);
    _cgi_devscalen(c2->x, g_xrad);
    _cgi_devscalen(c3->y, g_yrad);
    /* translate screen coords to coords of retained pixrect */
#define X_TRANSLATION           (x1 - g_xrad)
#define Y_TRANSLATION           (y1 - g_yrad)

    /* WDS : move below & remove TRANSLATIONs? */
    if ((arc_geom.x3 - arc_geom.x2) != 0)
    {
	m = (arc_geom.y3 - arc_geom.y2);
	m = m / (arc_geom.x3 - arc_geom.x2);
	b = (arc_geom.y3 - Y_TRANSLATION)
	    - m * (arc_geom.x3 - X_TRANSLATION);
    }
    else
    {
	m = 0;
	b = (arc_geom.x3 - X_TRANSLATION);
    }

    arc_geom.xc = x1 - X_TRANSLATION, arc_geom.yc = y1 - Y_TRANSLATION;
    arc_geom.x2 -= X_TRANSLATION, arc_geom.y2 -= Y_TRANSLATION;
    arc_geom.x3 -= X_TRANSLATION, arc_geom.y3 -= Y_TRANSLATION;
    _cgi_setup_arc_geom(&arc_geom, g_xrad, g_yrad, 0);

    /* set up stencil */
    sizex = 1 + g_xrad + g_xrad;
    sizey = 1 + g_yrad + g_yrad;
    _cgi_circrect = mem_create(sizex, sizey, 1);
    outrect = mem_create(sizex, sizey, vws->sunview.depth);
    patrect = _cgi_pattern(index, flag);
    (void) pr_replrop(outrect, 0, 0, sizex, sizey, PIX_SRC,
		      patrect, X_TRANSLATION, Y_TRANSLATION);

    rd = g_yrad;
    rd2 = g_xrad;
    color = vws->att->fill.color;
    x = 0;
    a = rd2;
    a = rd / a;
    a = a * a;
    b1 = 1.0;
    y = rd;
    x = 0;
    f = 0;
    _cgi_partial_ellip_fill_pts(g_xrad, g_yrad, &arc_geom, color, x, y, m, b, 0);
    for (y = g_yrad; y >= 0; y--)
    {
	f -= 2 * b1 * y - b1;
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    x++;
	    _cgi_partial_ellip_fill_pts(g_xrad, g_yrad, &arc_geom,
					color, x, y, m, b, 0);
	}
	_cgi_partial_ellip_fill_pts(g_xrad, g_yrad, &arc_geom, color, x, y, m, b, 0);
    }
    /* pw_unlock (vws->sunview.pw); wds removed 850503 */
    pw_pencil(vws->sunview.pw, X_TRANSLATION, Y_TRANSLATION, sizex, sizey,
	      _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
    (void) pr_destroy(patrect);
    (void) pr_destroy(outrect);
    (void) pr_destroy(_cgi_circrect);

#undef X_TRANSLATION           (x1 - g_xrad)
#undef Y_TRANSLATION           (y1 - g_yrad)
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pattern_pie_ellipse 			    */
/*                                                                          */
/*		draws non-closed _cgi_fill_partial_pattern_pie_ellipse	    */
/****************************************************************************/
_cgi_fill_partial_pattern_pie_ellipse(vws, c1, spt, ept, c2, c3, index, flag)
View_surface   *vws;
Ccoor           *c1, *spt, *ept, *c2, *c3;	/* center and contact point */
int             index, flag;
{
    int             x, y, color;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    int             x1t, y1t;
    int             x2t, y2t;
    int             x3t, y3t;
    Pixrect        *patrect, *outrect;
    float           f, /* fx, fy, fxx, fyy, */ a, b2 /* , y1r */ ;
    int             rd, rd2;
    int             sizex, sizey;
    struct cgi_pie_clip_geom _cgi_pie;

    _cgi_devscale(c1->x, c1->y, x1t, y1t);
    _cgi_devscale(spt->x, spt->y, x2t, y2t);
    _cgi_devscale(ept->x, ept->y, x3t, y3t);
    _cgi_devscalen(c2->x, g_xrad);
    _cgi_devscalen(c3->y, g_yrad);

    /* translate to pixrect coors and build pie clipping structure */
    x1 = g_xrad + 1, y1 = g_yrad + 1;
#define	X_OFFSET	(x1-x1t)/* xt(screen)+X_OFFSET = x(pixrect) */
#define	Y_OFFSET	(y1-y1t)/* yt(screen)+Y_OFFSET = y(pixrect) */
    x2 = x2t + X_OFFSET, y2 = y2t + Y_OFFSET;
    x3 = x3t + X_OFFSET, y3 = y3t + Y_OFFSET;
    _cgi_pie.xc = x1, _cgi_pie.yc = y1;
    _cgi_pie.x2 = x2, _cgi_pie.y2 = y2;
    _cgi_pie.x3 = x3, _cgi_pie.y3 = y3;
    _cgi_setup_pie_geom(&_cgi_pie, g_yrad);

    color = vws->att->fill.color;
    sizex = 1 + g_xrad + g_xrad;
    sizey = 1 + g_yrad + g_yrad;
    _cgi_circrect = mem_create(sizex, sizey, 1);
    outrect = mem_create(sizex, sizey, vws->sunview.depth);
    patrect = _cgi_pattern(index, flag);
    (void) pr_replrop(outrect, 0, 0, sizex, sizey, PIX_SRC,
		      patrect, -X_OFFSET, -Y_OFFSET);

    rd = g_yrad;
    rd2 = g_xrad;
    a = rd2;
    a = rd / a;
    a = a * a;
    b2 = 1.0;

    y = rd;
    x = 0;
    f = 0;
    _cgi_partial_pie_ellip_fill_ret_pts(g_xrad, g_yrad, &_cgi_pie, color, x, y);
    for (y = g_yrad; y >= 0; y--)
    {
	f -= 2 * b2 * y - b2;
	while (f < 0)
	{
	    f += 2 * a * x + a;
	    x++;
	    _cgi_partial_pie_ellip_fill_ret_pts(g_xrad, g_yrad, &_cgi_pie,
						color, x, y);
	}
	_cgi_partial_pie_ellip_fill_ret_pts(g_xrad, g_yrad, &_cgi_pie, color, x, y);
    }
/*  pw_unlock (vws->sunview.pw); wds removed 850503 */
    pw_pencil(vws->sunview.pw, -X_OFFSET, -Y_OFFSET, sizex, sizey,
	      _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
    (void) pr_destroy(patrect);
    (void) pr_destroy(outrect);
/*     (void) pr_destroy (endrect); */
    (void) pr_destroy(_cgi_circrect);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_pie_ellip_fill_ret_pts			    */
/*                                                                          */
/****************************************************************************/


_cgi_partial_pie_ellip_fill_ret_pts(x0, y0, pie_ptr, color, x, y)
short           x0, y0, color, x, y;
struct cgi_pie_clip_geom *pie_ptr;
{				/* draw symmetry points of ellipse */
    short           a1, a3, b2, b4;
    a1 = x0 + x;
    a3 = x0 - x;
    b2 = y0 + y;
    b4 = y0 - y;
    _cgi_vector_pcirc_pie_clip(a3, a1, b2, color, pie_ptr, 0);
    _cgi_vector_pcirc_pie_clip(a3, a1, b4, color, pie_ptr, 0);
}
