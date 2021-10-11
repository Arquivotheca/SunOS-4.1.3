#ifndef lint
static char	sccsid[] = "@(#)circtext2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI curve functions
 */

/*
_cgi_angle_textured_circle
_cgi_ang_textured_circ_pts
*/

/*
bugs:
textured circle
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */
int             _cgi_pix_mode;	/* pixrect equivalent */
short           _cgi_texture[8];

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_textured_circle 					    */
/*                                                                          */
/*		draws thick, textured circle with coors in screen space	    */
/****************************************************************************/
_cgi_angle_textured_circle(x0, y0, rad, width, val, style)
int             x0, y0, rad;	/* center and radius */
int             width;		/* thickness */
int             val;		/* color */
Clintype        style;		/* style */
{
    int             x, y;
    int             x1, y1;
    register short  d, d1;
    int             i, k, ptc;
    int             rad1;
    int             convf, oconvf, angsum, angsum1;
    int             styl;
    static int      pattern2[6][9] =
    {
     {
      9000, 46, 0, 0, 0, 0, 0, 0, 0
      },			/* solid  */
     {
      2, 2, 2, 2, 2, 2, 2, 2, 16
      },			/* dotted */
     {
      6, 2, 6, 2, 6, 2, 6, 2, 32
      },			/* dashed */
     {
      8, 2, 2, 2, 8, 2, 2, 2, 24
      },			/* dash dot */
     {
      8, 2, 2, 2, 2, 2, 2, 2, 18
      },			/* dash dot dotted */
     {
      10, 10, 10, 10, 10, 10, 10, 10, 80
      }
    };				/* long dashed */


    /* count number of points in an interior octant */
/*     if (width > 1) { */
    oconvf = _cgi_oct_circ_radius(rad);
    /* count number of points in an exterior octant */
    rad1 = rad - width + 1;
    convf = _cgi_oct_circ_radius(rad1);
    for (k = 0; k < 8; k++)
	_cgi_texture[k] = 1;
    x = 0;			/* x,y and d for inner circle */
    y = rad1;
    d = 3 - 2 * rad1;
    x1 = 0;
    y1 = rad;
    d1 = 3 - 2 * rad;
    i = 0;
/*     _cgi_texture[0] = 1; */
    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
    angsum1 = angsum = (pattern2[(int) style][0] << 10) / 2;
    if (style == SOLID)
    {
	while (x1 < y1)
	{
	    if (d < 0)		/* d = d + 4 * x + 6; */
		d += 4 * x + 6;
	    else
	    {
/* 	    d = d + 4 * (x - y) + 10; */
		d += 4 * (x - y) + 10;
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
/* 	    y = y - 1; */
		y--;
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
	    }
/* 	x += 1; */
	    x++;
/* 	ptc++; */
	    angsum += convf;
	    while (angsum1 <= angsum)
	    {
		if (d1 < 0)
		{
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		    d1 += 4 * x1 + 6;
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		}
		else
		{
		    d1 += 4 * (x1 - y1) + 10;
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
/* 		y1 = y1 - 1; */
		    y1--;
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		}
		x1++;
/* 	    x1 += 1; */
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		angsum1 += oconvf;
	    }
	}
    }				/* end (style == SOLID) */
    else
    {				/* (style != SOLID) */
	styl = (int) style;
	ptc = pattern2[(int) style][0] / 2;
	while (x1 < y1)
	{
	    if (d < 0)
		d = d + 4 * x + 6;
	    else
	    {
		d = d + 4 * (x - y) + 10;
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		y = y - 1;
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
	    }
	    x += 1;
	    ptc++;
	    angsum += convf;
	    while (angsum1 <= angsum)
	    {
		if (d1 < 0)
		{
/* 		_cgi_ang_textured_circ_pts (x0, y0, x, y, x1, y1, val); */
		    d1 = d1 + 4 * x1 + 6;
/* 		_cgi_ang_textured_circ_pts (x0, y0, x, y, x1, y1, val); */
		}
		else
		{
		    d1 = d1 + 4 * (x1 - y1) + 10;
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		    y1 = y1 - 1;
		    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		}
		x1 += 1;
		_cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
		angsum1 += oconvf;
	    }
	    _cgi_text_inc(&ptc, &styl, &i, &angsum, &angsum1);
	}			/* end while (x1 < y1) */
	if (x1 = y1)
	{
	    _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, val);
	}
    }				/* end (style != SOLID) */
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ang_textured_circ_pts 				    */
/*                                                                          */
/*		draws symetry points of textured circle			    */
/*		should only be one if test for optimiztion but ok for later */
/****************************************************************************/

int             _cgi_ang_textured_circ_pts(x0, y0, x, y, x1, y1, color)
int             x0, y0, x, y, x1, y1;
int             color;
{				/* draw symmetry points of circle */
    short           a1, a2, a3, a4, b1, b2, b3, b4;
    short           ra1, ra2, ra3, ra4, rb1, rb2, rb3, rb4;

    a1 = x0 + x;
    a2 = x0 + y;
    a3 = x0 - x;
    a4 = x0 - y;
    b1 = y0 + x;
    b2 = y0 + y;
    b3 = y0 - x;
    b4 = y0 - y;
    ra1 = x0 + x1;
    ra2 = x0 + y1;
    ra3 = x0 - x1;
    ra4 = x0 - y1;
    rb1 = y0 + x1;
    rb2 = y0 + y1;
    rb3 = y0 - x1;
    rb4 = y0 - y1;
    if (_cgi_texture[0] == 1)
	pw_vector(_cgi_vws->sunview.pw, a4, b1, ra4, rb1, _cgi_pix_mode, color);
    if (_cgi_texture[1] == 1)
	pw_vector(_cgi_vws->sunview.pw, a2, b1, ra2, rb1, _cgi_pix_mode, color);
    if (_cgi_texture[2] == 1)
	pw_vector(_cgi_vws->sunview.pw, a3, b2, ra3, rb2, _cgi_pix_mode, color);
    if (_cgi_texture[3] == 1)
	pw_vector(_cgi_vws->sunview.pw, a1, b2, ra1, rb2, _cgi_pix_mode, color);
    if (_cgi_texture[4] == 1)
	pw_vector(_cgi_vws->sunview.pw, a4, b3, ra4, rb3, _cgi_pix_mode, color);
    if (_cgi_texture[5] == 1)
	pw_vector(_cgi_vws->sunview.pw, a2, b3, ra2, rb3, _cgi_pix_mode, color);
    if (_cgi_texture[6] == 1)
	pw_vector(_cgi_vws->sunview.pw, a3, b4, ra3, rb4, _cgi_pix_mode, color);
    if (_cgi_texture[7] == 1)
	pw_vector(_cgi_vws->sunview.pw, a1, b4, ra1, rb4, _cgi_pix_mode, color);
}
