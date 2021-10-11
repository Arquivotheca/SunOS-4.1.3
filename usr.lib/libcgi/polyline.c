#ifndef	lint
static char	sccsid[] = "@(#)polyline.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif	lint

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
 * CGI Polyline functions
 */

/*
polyline
cgipw_polyline
_cgi_polyline
disjoint_polyline
cgipw_disjoint_polyline
rectangle
cgipw_rectangle
_cgi_rectangle
_cgi_pattern_rect
_cgi_arr_dev_xform
*/

#include "cgipriv.h"		/* defines types used in this file  */
#include <pixrect/gp1cmds.h>

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
u_char         *_cgi_disjoint_mvlist;
Cpixrect       *_cgi_pattern();

#define gp_lock(pw, rect, vws)						\
	    (void) _cgi_gp1_pw_lock(pw, rect, vws->sunview.gp_att)
#define gp_unlock(pw)		pw_unlock(pw)


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polyline	 					    */
/*                                                                          */
/*		Polyline draws non-closed polyline. Elements of polyline    */
/* 		are  drawn by the textured line algorithm in pw_polyline    */
/****************************************************************************/

Cerror          polyline(polycoors)
Ccoorlist      *polycoors;	/* list of points */
{
    int             ivw;
    Cerror          err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	    err = _cgi_polyline(_cgi_vws, polycoors->n, polycoors->ptlist,
				POLY_DONTCLOSE, XFORM);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_polyline						    */
/*                                                                          */
/*		Polyline draws non-closed polyline. Elements of polyline    */
/* 		are  drawn by the textured line algorithm in _cgi_polyline  */
/****************************************************************************/

Cerror          cgipw_polyline(desc, polycoors)
Ccgiwin        *desc;
Ccoorlist      *polycoors;	/* list of points */
{
    SETUP_CGIWIN(desc);
    return (_cgi_polyline(desc->vws, polycoors->n, polycoors->ptlist,
			  POLY_DONTCLOSE, XFORM));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_polyline						    */
/*                                                                          */
/*		Draws closed or non-closed polyline.			    */
/* 		Actual work is done by pw_polyline or _cgi_gp1_polyline.    */
/****************************************************************************/

Cerror          _cgi_polyline(vws, npts, coorlist, mvlist, xformflg)
View_surface   *vws;
register int    npts;		/* number of points in coorlist */
Ccoor          *coorlist;	/* list of points */
u_char         *mvlist;		/* draw/move flags for each segment */
Cxformflg       xformflg;	/* if TRUE, transform coordinates */
{
    register Outatt *attP = vws->att;
    register View_surface *vwsP = vws;
    register Gp1_attr *gpattP = vws->sunview.gp_att;
    struct pr_pos   conv_ptlist[MAXPTS];
    Pr_texture      tex, *texP;
    Pr_brush        brush;
    extern Pr_texture *_cgi_line_setup();
    struct pr_pos  *point;
    int             err = NO_ERROR;	/* error */
    int             usegp;
    int             op;
    int             dx, dy;

    if (npts >= 2)
    {
	if (npts <= MAXPTS)
	{
	    texP = _cgi_line_setup(&tex, &brush, npts);
	    op = PIX_COLOR(attP->line.color) | _cgi_pix_mode;

	    /*
	     * If we have untransformed points, give them directly to the GP,
	     * otherwise let pw_polyline take care of it.
	     */
	    if (xformflg == XFORM && vwsP->sunview.gp_att != (Gp1_attr *) 0)
	    {
		/*
		 * See if we need textured or wide line capability, which is
		 * not available pre-3.2.
		 */
		if (attP->line.style != SOLID || vwsP->conv.line_width != 1)
		{
		    /*
		     * Can't do it if GP1_CGI_LINE command not available, or
		     * textured lines if GP1_SET_LINE_WIDTH not available, or
		     * wide lines if GP1_SET_LINE_TEX not available.
		     */
		    if (gpattP->cmdver[GP1_CGI_LINE >> 8] == 0
			|| (attP->line.style != SOLID
			    && gpattP->cmdver[(GP1_SET_LINE_TEX >> 8)] == 0)
			|| (vwsP->conv.line_width != 1
			    && gpattP->cmdver[(GP1_SET_LINE_WIDTH >> 8)] == 0))
		    {
			usegp = 0;
		    }
		    else
		    {
			usegp = 1;
		    }
		}
		else if (gpattP->cmdver[GP1_CGIVEC >> 8] != 0)
		    /* Ok if at least GP1_CGIVEC is available */
		    usegp = 1;
		else
		    usegp = 0;
	    }
	    else
		usegp = 0;

	    if (usegp)
	    {

		gpattP = vwsP->sunview.gp_att;
		_cgi_gp1_set_op(_cgi_pix_mode, gpattP);
		_cgi_gp1_set_color(attP->line.color, gpattP);
		_cgi_gp1_set_linewidth(vwsP->conv.line_width, gpattP);
		_cgi_gp1_set_linetexture(texP, gpattP);
		gp_lock(vwsP->sunview.pw, &vwsP->sunview.lock_rect, vwsP);
		_cgi_gp1_pw_polyline(vwsP->sunview.pw, gpattP, npts,
				     coorlist, mvlist);
		gp_unlock(vwsP->sunview.pw);
		if (vwsP->sunview.pw->pw_prretained)
		{
		    if (X_DEVSCALE_NEEDED(vwsP) || Y_DEVSCALE_NEEDED(vwsP))
		    {
			_cgi_arr_dev_xform(vwsP, (unsigned) npts, coorlist,
					   conv_ptlist);
			dx = 0;
			dy = 0;
			point = conv_ptlist;
		    }
		    else
		    {
			dx = vwsP->xform.off.x;
			dy = vwsP->xform.off.y;
			point = (struct pr_pos *) coorlist;
		    }
		    if (mvlist == POLY_DISJOINT)
			mvlist = _cgi_disjoint_mvlist;
		    (void) pr_polyline(vwsP->sunview.pw->pw_prretained, dx, dy,
				       npts, point, mvlist, &brush, texP, op);
		}
	    }
	    else
	    {
		if (mvlist == POLY_DISJOINT)
		    mvlist = _cgi_disjoint_mvlist;

		switch (xformflg)
		{
		case XFORM:
		    if (X_DEVSCALE_NEEDED(vwsP) || Y_DEVSCALE_NEEDED(vwsP))
		    {
			_cgi_arr_dev_xform(vwsP, (unsigned) npts, coorlist,
					   conv_ptlist);
			dx = 0;
			dy = 0;
			point = conv_ptlist;
			break;
		    }
		    /* if scaling not needed, fall through to OFFSET_ONLY */
		case OFFSET_ONLY:
		    dx = vwsP->xform.off.x;
		    dy = vwsP->xform.off.y;
		    point = (struct pr_pos *) coorlist;
		    break;
		case DONT_XFORM:
		    dx = 0;
		    dy = 0;
		    point = (struct pr_pos *) coorlist;
		    break;
		}
		pw_polyline(vwsP->sunview.pw, dx, dy, npts,
			    point, mvlist, &brush, texP, op);
	    }
	}
	else
	    err = ENMPTSTL;
    }
    else
	err = EPLMTWPT;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: disjoint_polyline 					    */
/*                                                                          */
/*		Disjoint_polyline draws a non-closed polyline. Only  	    */
/*		alternate elements of the polyline			    */
/* 		are  drawn by the textured line algorithm. 		    */
/****************************************************************************/

Cerror          disjoint_polyline(polycoors)
Ccoorlist      *polycoors;	/* list of points */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_polyline(_cgi_vws, polycoors->n, polycoors->ptlist,
				POLY_DISJOINT, XFORM);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_disjoint_polyline 				    */
/*                                                                          */
/*		Disjoint_polyline draws a non-closed polyline. 		    */
/*		Only  alternate elements of the polyline		    */
/* 		are  drawn by the textured line algorithm. 		    */
/****************************************************************************/

Cerror          cgipw_disjoint_polyline(desc, polycoors)
Ccgiwin        *desc;
Ccoorlist      *polycoors;	/* list of points */
{
    int             err;	/* error */

    SETUP_CGIWIN(desc);
    err = _cgi_polyline(desc->vws, polycoors->n, polycoors->ptlist,
			POLY_DISJOINT, XFORM);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: rectangle		    			            */
/*                                                                          */
/*		draws filled box from coordinate rbc (right-hand bottom     */
/*              corner) to ltc (left-hand top corner).                      */
/****************************************************************************/

Cerror          rectangle(rbc, ltc)
Ccoor          *rbc, *ltc;	/* corners defining rectangle */
{
    int             ivw;
    Cerror          err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw) && !err)
	{
	    err = _cgi_rectangle(_cgi_vws, rbc, ltc);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_rectangle	    			            */
/*                                                                          */
/*		draws filled box from coordinate rbc (right-hand bottom     */
/*              corner) to ltc (left-hand top corner).			    */
/****************************************************************************/
int             cgipw_rectangle(desc, rbc, ltc)
Ccgiwin        *desc;
Ccoor          *rbc, *ltc;	/* corners defining rectangle */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_rectangle(desc->vws, rbc, ltc);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_rectangle						    */
/*                                                                          */
/*		draws filled box from coordinate rbc (right-hand bottom     */
/*              corner) to ltc (left-hand top corner).			    */
/****************************************************************************/
int             _cgi_rectangle(vws, rbc, ltc)
register View_surface *vws;
Ccoor          *rbc, *ltc;	/* corners defining rectangle */
{
    register Outatt *attP = vws->att;
    int             d1, d2, xa, x1, y1, x2, y2, color;
    Cerror          err = NO_ERROR;

    /* get device coordinates of inner rectangle */
    _cgi_devscale(rbc->x, rbc->y, x2, y1);	/* Exchange coords: put mins in
						 * 1s */
    _cgi_devscale(ltc->x, ltc->y, x1, y2);	/* put maximums in x2 and y2. */
    color = PIX_COLOR(attP->fill.color) | _cgi_pix_mode;
    d1 = x2 - x1;
    if (d1 < 0)
    {
	xa = x1;
	x1 = x2;
	x2 = xa;
	d1 = -d1;
    }				/* now, x1 < x2 */
    d2 = y2 - y1;
    if (d2 < 0)
    {
	xa = y1;
	y1 = y2;
	y2 = xa;
	d2 = -d2;
    }				/* now, y1 < y2 */

    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
    /* fill interior of rectangle */
    switch (attP->fill.style)
    {
    case SOLIDI:
	{
	    pw_write(vws->sunview.pw, x1, y1, d1 + 1, d2 + 1,
		     color, 0, 0, 0);
	    break;
	}
    case HOLLOW:
	{
	    break;
	}
    case PATTERN:
	{
	    _cgi_pattern_rect(vws, x1, y1, d1 + 1, d2 + 1,
			      attP->fill.pattern_index, 1);
	    break;
	}
    case HATCH:
	{
	    _cgi_pattern_rect(vws, x1, y1, d1 + 1, d2 + 1,
			      attP->fill.hatch_index, 0);
	    break;
	}
    }

    /* draw perimeter of rectangle */
    if (attP->fill.visible == ON)
    {
	Ccoor           corners[8];	/* DC coordinates */
	Clintype        s_style;
	Cint            s_width;
	Cint            s_color;
	int             width, xw, hw;

	/*
	 * Push out the corners so the filled interior and the border share no
	 * pixels.
	 */
	x1--;
	y1--;
	x2++;
	y2++;

	s_style = attP->line.style;
	s_width = vws->conv.line_width;
	s_color = attP->line.color;
	attP->line.style = attP->fill.pstyle;
	vws->conv.line_width = vws->conv.perimeter_width;
	attP->line.color = attP->fill.pcolor;

	/*
	 * This assumes excess width is displayed left and up by pw_polyline,
	 * and normalizes so that all width is displayed out (away from center
	 * of rectangle).
	 */
	width = vws->conv.perimeter_width;
	hw = (width - 1) >> 1;
	xw = (width & 1) ^ 1;

	/* Top horizontal line */
	corners[0].x = x1 - (width - 1);
	corners[0].y = y1 - hw;
	corners[1].x = x2 + (width - 1);
	corners[1].y = y1 - hw;

	/* Right vertical */
	corners[2].x = x2 + (hw + xw);;
	corners[2].y = y1 + 1;
	corners[3].x = x2 + (hw + xw);;
	corners[3].y = y2 - 1;

	/* Bottom horizontal */
	corners[4].x = x2 + (width - 1);
	corners[4].y = y2 + (hw + xw);
	corners[5].x = x1 - (width - 1);
	corners[5].y = y2 + (hw + xw);

	/* Left vertical */
	corners[6].x = x1 - hw;
	corners[6].y = y2 - 1;
	corners[7].x = x1 - hw;
	corners[7].y = y1 + 1;
	err = _cgi_polyline(vws, 8, &corners[0], POLY_DISJOINT, DONT_XFORM);
	attP->line.style = s_style;
	vws->conv.line_width = s_width;
	attP->line.color = s_color;
    }
    pw_unlock(vws->sunview.pw);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pattern_rect 					    */
/*                                                                          */
/*		draws pattern/hatch rectangle				    */
/*		 with coors in screen space			    	    */
/****************************************************************************/
int             _cgi_pattern_rect(vws, x0, y0, d1, d2, index, flag)
View_surface   *vws;
int             x0, y0, d1, d2, index, flag;
{
    Cpixrect       *patrect;

    patrect = _cgi_pattern(index, flag);
    pw_replrop(vws->sunview.pw, x0, y0, d1, d2, _cgi_pix_mode, patrect, x0, y0);
    (void) pr_destroy(patrect);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_arr_dev_xform 					    */
/*                                                                          */
/*		transform VDC coordinate list to DEVICE coordinates	    */
/*		This is faster than looping over _cgi_devscale()	    */
/****************************************************************************/
_cgi_arr_dev_xform(vws, npoints, vdc_coord_ptr, dev_coord_ptr)
View_surface   *vws;
register unsigned npoints;	/* count down to 0 */
register Ccoor *vdc_coord_ptr;
register struct pr_pos *dev_coord_ptr;
{
    register View_surface *vws_ptr = vws;

    if ((X_DEVSCALE_NEEDED(vws_ptr)) || (Y_DEVSCALE_NEEDED(vws_ptr)))
    {
	if (X_DEVSCALE_NEEDED(vws_ptr))
	{
	    if (Y_DEVSCALE_NEEDED(vws_ptr))	/* Typical non-cgipw case */
		while (npoints--)
		{
		    DEV_XFORM_X(vdc_coord_ptr, dev_coord_ptr,, vws_ptr);
		    DEV_XFORM_Y(vdc_coord_ptr, dev_coord_ptr, ++, vws_ptr);
		}
	    else		/* X scale needed, not Y */
		while (npoints--)
		{
		    DEV_XFORM_X(vdc_coord_ptr, dev_coord_ptr,, vws_ptr);
		    DEV_OFF_Y(vdc_coord_ptr, dev_coord_ptr, ++);
		}
	}
	else			/* Y scale needed, not X */
	{
	    while (npoints--)
	    {
		DEV_OFF_X(vdc_coord_ptr, dev_coord_ptr,);
		DEV_XFORM_Y(vdc_coord_ptr, dev_coord_ptr, ++, vws_ptr);
	    }
	}
    }
    else			/* Neither X nor Y needs scaling (typical cgipw
				 * case) */
    {
	while (npoints--)
	{
	    DEV_OFF_X(vdc_coord_ptr, dev_coord_ptr,);
	    DEV_OFF_Y(vdc_coord_ptr, dev_coord_ptr, ++);
	}
    }
}
