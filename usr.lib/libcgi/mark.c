#ifndef lint
static char	sccsid[] = "@(#)mark.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI marker functions
 */

/*
marker_type
cgipw_marker_type
_cgi_marker_type
marker_size
cgipw_marker_size
_cgi_check_marker_size
_cgi_set_conv_marker_size
marker_color
cgipw_marker_color
_cgi_marker_color
marker_size_specification_mode
cgipw_marker_size_specification_mode
polymarker
cgipw_polymarker
_cgi_polymarker
_cgi_marker
_cgi_hollow2_circle
_cgi_hollow2_circ_pts
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */

/* Like pw_put, but uses RasterOp code */
#define PW_PUT_WITH_OP(pw,x,y,op,color)	pw_vector(pw, x,y, x,y, op, color)

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_type                                           	    */
/*                                                                          */
/*		Sets marker type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          marker_type(ttyp)
Cmartype        ttyp;		/* style of marker */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_marker_type(_cgi_att, ttyp);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_marker_type                                     	    */
/*                                                                          */
/*                                                                          */
/*		Sets marker type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          cgipw_marker_type(desc, ttyp)
Ccgiwin        *desc;
Cmartype        ttyp;		/* style of marker */

{
    SETUP_CGIWIN(desc);
    return (_cgi_marker_type(desc->vws->att, ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_marker_type                                     	    */
/*                                                                          */
/*                                                                          */
/*		Sets marker type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          _cgi_marker_type(att, ttyp)
Outatt         *att;
Cmartype        ttyp;		/* style of marker */

{
    int             err = NO_ERROR;

    if (att->asfs[3] == INDIVIDUAL)
    {
	att->marker.type = (Cmartype) ttyp;
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_size                                           	    */
/*                                                                          */
/*		Sets marker size 					    */
/****************************************************************************/

Cerror          marker_size(size)
Cfloat          size;		/* marker size */
{
    int             ivw;
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_marker_size(_cgi_att, size);
    if (!err)
    {
	_cgi_att->marker.size = size;
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_marker_size(_cgi_vws);
    }

    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_marker_size                                     	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_marker_size(desc, size)
Ccgiwin        *desc;
Cfloat          size;		/* marker size */
{
    int             err = NO_ERROR;

    SETUP_CGIWIN(desc);
    if (!err)
	err = _cgi_check_marker_size(desc->vws->att, size);
    if (!err)
    {
	desc->vws->att->marker.size = size;
	_cgi_set_conv_marker_size(desc->vws);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_marker_size					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_check_marker_size(att, size)
Outatt         *att;
Cfloat          size;		/* marker size */
{
    if (att->asfs[4] != INDIVIDUAL)
	return (EBTBUNDL);
    else if (size < 0)
	return (EBADSIZE);	/* size must be positive */
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_set_conv_marker_size                                       */
/*                                                                          */
/*   Created 851009 (wds) to organize setting a marker.width value.	    */
/****************************************************************************/

Cerror          _cgi_set_conv_marker_size(vws)
View_surface   *vws;
{
    register Outatt *attP = vws->att;
    float           dist;	/* x range of VDC space */
    float           wtemp;	/* temporary result */

    if (attP->mark_spec_mode == SCALED)
    {
	/* wds 850617: viewport coords are "VDC" for pixwin surfaces */
	if (attP->vdc.use_pw_size)
	    dist = vws->vport.r_width;
	else
	    dist = abs(attP->vdc.rect.r_width);
	_cgi_devscalen((int) (dist * (attP->marker.size / 100.) + 0.5), wtemp);
	if (wtemp < MIN_MKR_SIZE)
	    wtemp = MIN_MKR_SIZE;
	vws->conv.marker_size = wtemp;
    }
    else
    {
	vws->conv.marker_size = attP->marker.size;
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_color                                          	    */
/*                                                                          */
/*		Sets marker color 					    */
/****************************************************************************/

Cerror          marker_color(index)
Cint            index;		/* marker color */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_marker_color(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_marker_color                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_marker_color(desc, index)
Ccgiwin        *desc;
Cint            index;		/* marker color */
{
    SETUP_CGIWIN(desc);
    return (_cgi_marker_color(desc->vws->att, index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_marker_color                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_marker_color(att, index)
Outatt         *att;
Cint            index;		/* marker color */
{
    int             err = NO_ERROR;

    if (att->asfs[5] == INDIVIDUAL)
    {
	err = _cgi_check_color(index);
	if (!err)
	{
	    att->marker.color = index;
	}
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: marker_size_specification_mode	 			    */
/*                                                                          */
/****************************************************************************/

Cerror          marker_size_specification_mode(mode)
Cspecmode       mode;		/* pixels or percent */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_att->mark_spec_mode = mode;
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_marker_size(_cgi_vws);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_marker_size_specification_mode 			    */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_marker_size_specification_mode(desc, mode)
Ccgiwin        *desc;
Cspecmode       mode;		/* pixels or percent */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->mark_spec_mode = mode;
    /* Change the interpretation of existing marker size: */
    _cgi_set_conv_marker_size(desc->vws);
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker	 					    */
/*                                                                          */
/*		displays marker points at each element of polycoors	    */
/****************************************************************************/

Cerror          polymarker(polycoors)
Ccoorlist      *polycoors;	/* list of points */
{
    int             ivw;
    Cerror          err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_polymarker(_cgi_vws, polycoors);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_polymarker 					    */
/*                                                                          */
/*		displays marker points at each element of polycoors	    */
/****************************************************************************/

Cerror          cgipw_polymarker(desc, polycoors)
Ccgiwin        *desc;
Ccoorlist      *polycoors;	/* list of points */
{
    SETUP_CGIWIN(desc);
    return (_cgi_polymarker(desc->vws, polycoors));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_polymarker 					    */
/*                                                                          */
/*		displays marker points at each element of polycoors	    */
/****************************************************************************/

Cerror          _cgi_polymarker(vws, polycoors)
View_surface   *vws;
Ccoorlist      *polycoors;	/* list of points */
{
    register Ccoor *point;	/* pointers to points */
    register View_surface *vwsP = vws;
    register int    i, nt;	/* counters */
    int             err = NO_ERROR;	/* error */

    /* actually draw polymarker */
    point = polycoors->ptlist;
    nt = polycoors->n;
    if (nt <= MAXPTS)
    {
	for (i = 0; (i++ < nt);)
	{			/* draw a marker at each point */
	    (void) _cgi_marker(vwsP, *point++);
	}
    }
    else
	err = ENMPTSTL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_marker	 					    */
/*                                                                          */
/*		draws a single marker					    */
/****************************************************************************/
Cerror          _cgi_marker(vws, c1)
View_surface   *vws;
Ccoor           c1;
{
    int             x0, y0, half, quarter, color;

    _cgi_devscale(c1.x, c1.y, x0, y0);
    /*
     * The following check ISN'T proper clipping, but is typically identical to
     * the clipping of polylines.  This clips to the REAL WINDOW (r_screen) and
     * not to the effective viewport, nor the user's clipping rectangle.
     */
    if (_cgi_err_check_69(x0, y0))	/* Control Point outside window? */
	return (EVALOVWS);	/* Do not render such a marker. */
    /* arms of plus are guaranteed to be of equal length  by int. division */
    half = vws->conv.marker_size / 2;
    color = vws->att->marker.color;
    switch (vws->att->marker.type)
    {
    case (DOT):
	PW_PUT_WITH_OP(vws->sunview.pw, x0, y0, _cgi_pix_mode, color);
	break;
    case (PLUS):
	pw_vector(vws->sunview.pw, x0, y0 - half,
		  x0, y0 + half, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, x0 - half, y0,
		  x0 + half, y0, _cgi_pix_mode, color);
	break;
    case (ASTERISK):
	pw_vector(vws->sunview.pw, x0, y0 - half,
		  x0, y0 + half, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, x0 - half, y0,
		  x0 + half, y0, _cgi_pix_mode, color);
	quarter = (3 * half) / 4;
	pw_vector(vws->sunview.pw, x0 - quarter,
	      y0 - quarter, x0 + quarter, y0 + quarter, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, x0 - quarter,
	      y0 + quarter, x0 + quarter, y0 - quarter, _cgi_pix_mode, color);
	break;
    case (CIRCLE):		/* should be drawn with circle not O */
	_cgi_hollow2_circle(vws, x0, y0, half, color);
	break;
    case (X):
	pw_vector(vws->sunview.pw, x0 - half,
		  y0 - half, x0 + half, y0 + half, _cgi_pix_mode, color);
	pw_vector(vws->sunview.pw, x0 - half,
		  y0 + half, x0 + half, y0 - half, _cgi_pix_mode, color);
	break;
    }
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_hollow2_circle 					    */
/*                                                                          */
/*		draws empty circle with coors in screen space		    */
/****************************************************************************/
int             _cgi_hollow2_circle(vws, x0, y0, rad, val)
View_surface   *vws;
short           x0, y0, rad, val;
{
    register int    x, y, d;

    x = 0;
    y = rad;
    d = 3 - 2 * rad;
    while (x < y)
    {
	_cgi_hollow2_circ_pts(vws, x0, y0, x, y, val);
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
	_cgi_hollow2_circ_pts(vws, x0, y0, x, y, val);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_hollow2_circ_pts 					    */
/*                                                                          */
/****************************************************************************/
int             _cgi_hollow2_circ_pts(vws, x0, y0, x, y, color)
View_surface   *vws;
short           x0, y0, x, y, color;
{				/* draw symmetry points of circle */
    struct pr_pos   points[8];
    points[0].x = -y;
    points[0].y = x;
    points[1].x = y;
    points[1].y = x;
    points[2].x = -x;
    points[2].y = y;
    points[3].x = x;
    points[3].y = y;
    points[4].x = -y;
    points[4].y = -x;
    points[5].x = y;
    points[5].y = -x;
    points[6].x = -x;
    points[6].y = -y;
    points[7].x = x;
    points[7].y = -y;
    pw_polypoint(vws->sunview.pw, x0, y0, 8, points,
		 _cgi_pix_mode | PIX_COLOR(color));
}
