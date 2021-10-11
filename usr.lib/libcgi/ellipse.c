#ifndef lint
static char	sccsid[] = "@(#)ellipse.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI ellipse functions
*/

/*
ellipse
cgipw_ellipse
_cgi_ellipse
_cgi_draw_ellipse
_cgi_inner_ellipse
_cgi_ellipse_pts
_cgi_pattern_ellipse
_cgi_ellipse_ret_pts
_cgi_textured_ellipse
cgi_ang_textured_ellip_pts
*/

/*
bugs:
rename maj and minor->x and y
float/registers
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
short           _cgi_texture[8];
static int      pattern2[6][9] =
{
    {
	18000, 90, 0, 0, 0, 0, 0, 0, 0
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

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: ellipse	 					    */
/*                                                                          */
/*		Draws an ellipse centered at c1 with major and minor axes   */
/*              with endpoints at c2 and c3.                                */
/****************************************************************************/

Cerror          ellipse(c1, majx, miny)
Ccoor          *c1;		/* center */
Cint            majx, miny;	/* endpoints of major and minor axes */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	if (majx < 0)
	    majx = -majx;
	if (miny < 0)
	    miny = -miny;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_ellipse(_cgi_vws, c1, majx, miny);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_ellipse	 					    */
/*                                                                          */
/*		Draws an ellipse centered at c1 with major and minor axes   */
/*              with endpoints at c2 and c3.                                */
/****************************************************************************/

Cerror          cgipw_ellipse(desc, c1, majx, miny)
Ccgiwin        *desc;
Ccoor          *c1;		/* center */
Cint            majx, miny;	/* endpoints of major and minor axes */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_ellipse(desc->vws, c1, majx, miny);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ellipse	 					    */
/*                                                                          */
/*		Draws an ellipse centered at c1 with major and minor axes   */
/*              with endpoints at c2 and c3.                                */
/****************************************************************************/

Cerror          _cgi_ellipse(vws, c1, majx, miny)
View_surface   *vws;
Ccoor          *c1;		/* center */
Cint            majx, miny;	/* endpoints of major and minor axes */
{
    Ccoor           c2, c3;
    int             err;

    /* check negative majx & miny, because user can call cgipw_ellipse */
    c2.x = c1->x + ((majx < 0) ? (-majx) : majx);
    c2.y = c1->y;
    c3.x = c1->x;
    c3.y = c1->y + ((miny < 0) ? (-miny) : miny);
    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
    err = _cgi_draw_ellipse(vws, *c1, c2, c3);
    pw_unlock(vws->sunview.pw);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_draw_ellipse	 				    */
/*                                                                          */
/*		Draws an ellipse centered at c1 with major and minor axes   */
/*              with endpoints at c2 and c3.                                */
/****************************************************************************/
int             _cgi_draw_ellipse(vws, c1, c2, c3)
View_surface   *vws;
Ccoor           c1, c2, c3;	/* c1 is center point; c2 and c3 are endpoints
				 * of major and minor axes */
{
    register Outatt *attP = vws->att;
    short           x1, y1;
    short           x2, y2;
    short           x3, y3;
    Clintype        style;

    _cgi_devscale(c1.x, c1.y, x1, y1);
    /* convert to screen coordinates */
    _cgi_devscale(c2.x, c2.y, x2, y2);
    _cgi_devscale(c3.x, c3.y, x3, y3);
    /* just draw a elliptical blob */
    style = attP->fill.pstyle;
    if ((style == SOLID) && (attP->fill.style == SOLIDI) &&
	(attP->fill.visible == ON) &&
	(attP->fill.color == attP->fill.pcolor)
	)
    {
	x2 += vws->conv.perimeter_width;
	y3 += vws->conv.perimeter_width;
	(void) _cgi_inner_ellipse(vws, x1, y1, x2, y2, x3, y3);
    }
    else
    {				/* fill and then draw perimeter  */
	switch (attP->fill.style)
	{			/* select fill style */
	case SOLIDI:
	    {
		(void) _cgi_inner_ellipse(vws, x1, y1, x2, y2, x3, y3);
		break;
	    }
	case HOLLOW:
	    {
		break;
	    }
	case PATTERN:
	    {
		(void) _cgi_pattern_ellipse(vws, x1, y1, x2, y2, x3, y3,
					    attP->fill.pattern_index, 1);
		break;
	    }
	case HATCH:
	    {
		(void) _cgi_pattern_ellipse(vws, x1, y1, x2, y2, x3, y3,
					    attP->fill.hatch_index, 0);
		break;
	    }
	}
	x2 += vws->conv.perimeter_width;
	y3 += vws->conv.perimeter_width;
	if (attP->fill.visible == ON)
	    (void) _cgi_textured_ellipse(vws, x1, y1, x2, y2, x3, y3);
    }
    return (NO_ERROR);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_inner_ellipse 					    */
/*                                                                          */
/*		draws ellipse with coors in screen space		    */
/****************************************************************************/
/* Warning: returned value (err) is never used. */
Cerror          _cgi_inner_ellipse(vws, x0, y0, x1, y1, x2, y2)
View_surface   *vws;
short           x0, y0, x1, y1, x2, y2;
{
    int             x, y, rad, rad2;
    short           llx, lly, llx2, lly2;
    float           a, b, f;
    short           r1, r2;
    short           val;
    int             err = NO_ERROR;

/*	val = PIX_SRC | PIX_DST; */
    val = vws->att->fill.color;	/* color */
    llx = x1 - x0;		/* distances from center */
    lly = y1 - y0;
    llx2 = x2 - x0;
    lly2 = y2 - y0;
    if (!llx || !llx2)
    {				/* ellipse is aligned with x and y axes */
	/* better error check */
	if (llx == 0)
	    r1 = abs(lly);
	else
	    r1 = abs(llx);
	if (llx2 == 0)
	    r2 = abs(lly2);
	else
	    r2 = abs(llx2);
	rad = r1;
	rad2 = r2;
	x = 0;
	y = rad;
	a = r1;
	a = r2 / a;
	a = a * a;
	b = 1.0;
	f = 0;
	f = -rad2;
	pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
	for (y = rad2; y >= 0; y--)
	{
	    while (f <= 0)
	    {
		f += 2 * a * x + a;
		x++;
	    }
	    f -= 2 * b * y - b;
	    _cgi_ellipse_pts(vws, x0, y0, x, y, val);
	}
	pw_unlock(vws->sunview.pw);
    }
    else
    {
	err = EARCPNEL;		/* new error */
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ellipse_pts 					    */
/*                                                                          */
/****************************************************************************/
_cgi_ellipse_pts(vws, x0, y0, x, y, color)
View_surface   *vws;
short           x0, y0, x, y, color;
{				/* draw symmetry points of ellipse */
    short           a1, a2, b1, b2;
    a1 = x0 + x;
    a2 = x0 - x;
    b1 = y0 + y;
    b2 = y0 - y;
    pw_vector(vws->sunview.pw, a2, b1, a1, b1, _cgi_pix_mode, color);
    pw_vector(vws->sunview.pw, a2, b2, a1, b2, _cgi_pix_mode, color);
}


/* This is a fix for some old pw bug.  We should use pw_stencil & test it */
#define pw_pencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)	\
	(*(dpw)->pw_ops->pro_stencil) ((dpw)->pw_opshandle,	\
	x - (dpw)->pw_opsx, y - (dpw)->pw_opsy, w, h, op,		\
	stpr, stx, sty, spr, sy, sx)

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pattern_ellipse 					    */
/*                                                                          */
/* 		draws ellipse with coors in screen space		    */
/****************************************************************************/
/* Warning: return value (err) is always ignored. */
Cerror          _cgi_pattern_ellipse(vws, x0, y0, x1, y1, x2, y2, index, flag)
View_surface   *vws;
short           x0, y0, x1, y1, x2, y2, index, flag;
{
    int             x, y, size, size2;
    Pixrect        *patrect, *outrect;
    int             rad, rad2;
    float           a, b, f;
    short           r1, r2;
    short           llx, lly, llx2, lly2;
    int             err;

    llx = x1 - x0;		/* distances from center */
    lly = y1 - y0;
    llx2 = x2 - x0;
    lly2 = y2 - y0;
    if (!llx || !llx2)
    {				/* ellipse is aligned with x and y axes */
	/* better error check */
	if (llx == 0)
	    r1 = abs(lly);
	else
	    r1 = abs(llx);
	if (llx2 == 0)
	    r2 = abs(lly2);
	else
	    r2 = abs(llx2);
	rad = r1;
	rad2 = r2;
	x = 0;
	size = 1 + 2 * rad;
	size2 = 1 + 2 * rad2;

	outrect = mem_create(size, size2, vws->sunview.depth);
	patrect = _cgi_pattern(index, flag);
	(void) pr_replrop(outrect, 0, 0, size, size2, PIX_SRC,
			  patrect, x0 - r1, y0 - r2);
	(void) pr_destroy(patrect);
	_cgi_circrect = mem_create(size, size2, 1);

	a = r1;
	a = r2 / a;
	a = a * a;
	b = 1.0;
	f = 0;
	for (y = rad2; y >= 0; y--)
	{
	    f -= 2 * b * y - b;
	    while (f < 0)
	    {
		f += 2 * a * x + a;
		x++;
	    }
	    _cgi_ellipse_ret_pts(r1, r2, x, y);
	}
	pw_pencil(vws->sunview.pw, x0 - r1, y0 - r2, size, size2,
		  _cgi_pix_mode, _cgi_circrect, 0, 0, outrect, 0, 0);
	(void) pr_destroy(outrect);
	(void) pr_destroy(_cgi_circrect);
    }
    else
    {
	err = EARCPNEL;		/* should be new error */
    }
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ellipse_ret_pts 					    */
/*                                                                          */
/****************************************************************************/
_cgi_ellipse_ret_pts(x0, y0, x, y)
short           x0, y0, x, y;
{				/* draw symmetry points of ellipse */
    short           a1, a3, b2, b4;
    int             bop;

    bop = PIX_OPCOLOR(255) | PIX_SRC;
    a1 = x0 + x;
    a3 = x0 - x;
    b2 = y0 + y;
    b4 = y0 - y;
    (void) pr_vector(_cgi_circrect, a3, b2, a1, b2, bop, 1);
    (void) pr_vector(_cgi_circrect, a3, b4, a1, b4, bop, 1);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_textured_ellipse 					    */
/*                                                                          */
/*		draws textured scan-converted ellipse			    */
/****************************************************************************/
/* Warning: return value (err) is always ignored. */
Cerror          _cgi_textured_ellipse(vws, x0, y0, x1, y1, x2, y2)
View_surface   *vws;
short           x0, y0, x1, y1, x2, y2;
{
    int             x, y, rad, rad2;
    short           llx, lly, llx2, lly2;
    float           a, b, f, f1;
    short           r1, r2;
    short           inc, val;
    int             width;
    float           convf, oconvf, angsum, angsum1;
    int             i, k, ptc, rada, rad2a;
    float           a1, b1;
    Clintype        style;
    int             err = NO_ERROR;

/*	val = PIX_SRC | PIX_DST; */
    val = vws->att->fill.pcolor;/* wds 850830: perimeter color */
    style = vws->att->fill.pstyle;	/* color */
    llx = x1 - x0;		/* distances from center */
    lly = y1 - y0;
    llx2 = x2 - x0;
    lly2 = y2 - y0;
    if (!llx || !llx2)
    {				/* ellipse is aligned with x and y axes */
	/* better error check */
	if (llx == 0)
	    r1 = abs(lly);
	else
	    r1 = abs(llx);
	if (llx2 == 0)
	    r2 = abs(lly2);
	else
	    r2 = abs(llx2);
	width = vws->conv.perimeter_width;
	/* width */
	rad = r2;
	rad2 = r1;
	x = 0;
/* 	rad--; */
/* 	rad2--; */
	y = rad;
	a = r1;
	a = r2 / a;
	a = a * a;
	b = 1.0;
	f = 0;
	inc = 0;
	for (y = rad; y >= 0; y--)
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
	rada = rad - width + 1;
	rad2a = rad2 - width + 1;
	oconvf = 90.0 / inc;	/* outside length */
	x = 0;
	y = rada;
	a1 = rad2a;
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
	convf = 90.0 / inc;	/* outside length */
	x = 0;
	x1 = 0;
	y = rad;
	y1 = rada;
	f1 = 0;
	f = 0;
	for (k = 0; k < 8; k++)
	    _cgi_texture[k] = 1;
	i = 0;
	angsum = pattern2[(int) style][0] / 2;
	angsum1 = 0.0;
	ptc = 0;		/* number of points in an inner ellipse pattern
				 * element */
	_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
	angsum1 = pattern2[(int) style][0] / 2;
	ptc = pattern2[(int) style][0] / 2;
	if (width > 1)
	{
	    for (y = rad; y >= 0; y--)
	    {
		f -= 2 * b * y - b;
		_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
		ptc++;
		while (f < 0)
		{
		    ptc++;
		    f += 2 * a * x + a;
		    inc++;
		    _cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
		    x++;
		    angsum += oconvf;
		    _cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
		}
		angsum += oconvf;
		while (angsum1 <= angsum)
		{
		    f1 -= 2 * b1 * y1 - b1;
		    _cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
		    while ((f1 < 0))
		    {		/* extra test neccessary to prevent textrue
				 * dependent errors */
			f1 += 2 * a1 * x1 + a1;
			_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
			x1++;
			angsum1 += convf;
			_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, val);
		    }
		    y1--;
		    angsum1 += convf;
		}
		if (ptc >= pattern2[(int) style][i])
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
		    ptc = 0;
		    i &= 7;
		}
	    }
	}
	else
	{
	    for (y = rad; y >= 0; y--)
	    {
		f -= 2 * b * y - b;
		_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x, y, val);
		ptc++;
		while (f < 0)
		{
		    ptc++;
		    if (ptc >= pattern2[(int) style][i])
		    {
			i++;
			if ((i % 2) == 0)
			    for (k = 0; k < 8; k++)
				_cgi_texture[k] = 1;
			else
			    for (k = 0; k < 8; k++)
				_cgi_texture[k] = 0;
			ptc = 0;
			i &= 7;
		    }
		    f += 2 * a * x + a;
		    inc++;
		    _cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x, y, val);
		    x++;
		    _cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x, y, val);
		}
		if (ptc >= pattern2[(int) style][i])
		{
		    i++;
		    if ((i % 2) == 0)
			for (k = 0; k < 8; k++)
			    _cgi_texture[k] = 1;
		    else
			for (k = 0; k < 8; k++)
			    _cgi_texture[k] = 0;
		    ptc = 0;
		    i &= 7;
		}
	    }
	}
    }
    else
    {
/*	    _cgi_tilt_ellipse(x0,y0,x1,y1,x2,y2,val) */
	err = 999;		/* new error */
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgi_ang_textured_ellip_pts				    */
/*                                                                          */
/*		draws non-closed polyline				    */
/****************************************************************************/

_cgi_ang_textured_ellip_pts(vws, x0, y0, x, y, x1, y1, color)
View_surface   *vws;
short           x0, y0, x, y, x1, y1, color;
{				/* draw symmetry points of ellipse */
    short           a1, a3, b2, b4;
    short           ra1, ra3, rb2, rb4;
    if (_cgi_texture[0] == 1)
    {
	a1 = x0 + x;
	a3 = x0 - x;
	b2 = y0 + y;
	b4 = y0 - y;
	ra1 = x0 + x1;
	ra3 = x0 - x1;
	rb2 = y0 + y1;
	rb4 = y0 - y1;
	pw_vector(vws->sunview.pw, a3, b2, ra3, rb2, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, a1, b2, ra1, rb2, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, a1, b4, ra1, rb4, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, a3, b4, ra3, rb4, _cgi_pix_mode, color);
    }
}
