#ifndef	lint
static char	sccsid[] = "@(#)cgiwindow.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI window functions
 */


/*
vdc_extent
cgipw_set_vdc_extent
cgipw_set_offset
_cgi_update_gp1_xform
device_viewport
clip_indicator
clip_rectangle
_cgi_windowset
_cgi_reset_clipping
_cgi_reset_intatt
_cgi_get_r_screen
*/

/*
bugs:
reverse screen device viewport
vdc extent work for all viewports.
implement clip rectangle in all functions.
x and y scaling factors should be computed separately
*/

#include "cgipriv.h"
#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <sun/fbio.h>
#include <sun/gpio.h>

Gstate          _cgi_state;	/* CGI global state */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
int             _cgi_criticalcnt;	/* critical region protection */
Ccoor           _cgi_00;	/* coordinate (0,0) */
caddr_t	      (*_cgi_win_get)();	/* -> "window_get" if canvas call */


/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  vdc_extent				 		    */
/*                                                                          */
/*		Defines the limits of VDC space				    */
/****************************************************************************/

Cerror          vdc_extent(c1, c2)
Ccoor          *c1, *c2;	/* bottom left-hand and top right-hand corner
				 * of VDC space */
{
    register Outatt *common_att = _cgi_state.common_att;
    int             ivw;
    Cerror          err, terr;

    if (_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = ENOTCCPW;
    else
	err = _cgi_check_state_5();
    if (!err)
    {
/*	    (void) printf("corners %d %d %d %d\n",c1->x,c2->x,c1->y,c2->y); */
	if ((c1->x < c2->x) && (c1->y < c2->y))
	{
	    if (!((c1->x <= -32768) || (c1->y <= -32768)
		  || (c2->x >= 32768) || (c2->y >= 32768)))
	    {
		common_att->vdc.rect.r_left = c1->x;
		common_att->vdc.rect.r_width = c2->x - c1->x + 1;
		common_att->vdc.rect.r_bottom = c1->y;
		common_att->vdc.rect.r_height = c2->y - c1->y + 1;
		common_att->vdc.clip.rect = common_att->vdc.rect;
		/* rescale all open windows */
		ivw = 0;
		while (_cgi_bump_vws(&ivw))
		{
		    terr = _cgi_windowset(_cgi_vws);
		    if (terr && !err)		/* capture first error */
			err = terr;
		    if (_cgi_vws->sunview.gp_att != (Gp1_attr *) 0)
			_cgi_update_gp1_xform(_cgi_vws);
		}
	    }
	    else
		err = EVDCSDIL;
	}
	else
	    err = EBADRCTD;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_set_vdc_extent					    */
/*                                                                          */
/*		Set global xscale and yscale, and update GP's values.	    */
/****************************************************************************/
Cerror          cgipw_set_vdc_extent(desc, c1, c2)
Ccgiwin        *desc;
Ccoor          *c1, *c2;	/* bottom left-hand and top right-hand corner
				 * of VDC space */
{
    register View_surface *vwsP;
    register Outatt *attP;
    Cerror          err;

    SETUP_CGIWIN(desc);
    vwsP = desc->vws;
    attP = vwsP->att;
    if ((c1->x < c2->x) && (c1->y < c2->y))
    {
	if (!((c1->x <= -32768) || (c1->y <= -32768)
	      || (c2->x >= 32768) || (c2->y >= 32768)))
	{
	    attP->vdc.use_pw_size = 0;
	    attP->vdc.rect.r_left = c1->x;
	    attP->vdc.rect.r_width = c2->x - c1->x + 1;
	    attP->vdc.rect.r_bottom = c1->y;
	    attP->vdc.rect.r_height = c2->y - c1->y + 1;
	    err = _cgi_windowset(vwsP);
	    if (vwsP->sunview.gp_att != (Gp1_attr *) NULL)
		_cgi_update_gp1_xform(vwsP);
	}
	else
	    err = EVDCSDIL;
    }
    else
	err = EBADRCTD;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_set_offset					    */
/*                                                                          */
/*		Set global xoff and yoff, and update GP's offset.	    */
/****************************************************************************/
Cerror          cgipw_set_offset(desc, xoff, yoff)
Ccgiwin        *desc;
Cint            xoff, yoff;
{
    register View_surface *vwsP = desc->vws;
    Cerror          err = NO_ERROR;

    SETUP_CGIWIN(desc);
    vwsP->xform.win_off.x = xoff;
    vwsP->xform.win_off.y = yoff;
    err = _cgi_windowset(vwsP);
    if (vwsP->sunview.gp_att != (Gp1_attr *) NULL)
	_cgi_update_gp1_xform(vwsP);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  _cgi_update_gp1_xform 			 	    */
/*                                                                          */
/*	      Gives CGI's VDC-to-DC transform to GP1.			    */
/*	      Uses vwsP argument to get transform.			    */
/*	      Updates GP's clip list for good luck.			    */
/****************************************************************************/

_cgi_update_gp1_xform(vws)
register View_surface *vws;	/* ptr to view surface to update from */
{
    register Gp1_attr *gp_att = vws->sunview.gp_att;
    float           xs, ys, xo, yo;

    (void) _cgi_gp1_pw_updclplst(vws->sunview.pw, gp_att);
    xs = (float) (vws->xform.scale.x.num >> 1)
	/ (float) vws->xform.scale.x.den;
    xo = (float) (vws->xform.off.x + gp_att->org_x);
    ys = (float) (vws->xform.scale.y.num >> 1)
	/ (float) vws->xform.scale.y.den;
    yo = (float) (vws->xform.off.y + gp_att->org_y);
    _cgi_gp1_set_scale(&xs, &ys, gp_att);
    _cgi_gp1_set_offset(&xo, &yo, gp_att);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  device_viewport				 	    */
/*                                                                          */
/*	      Defines where in device coordinates the screen will be mapped */
/****************************************************************************/

Cerror          device_viewport(name, c1, c2)
Cint            name;		/* name of view surface */
Ccoor          *c1, *c2;	/* bottom left-hand and top right-hand corner
				 * of viewsurface to map VDC onto (expressed in
				 * pixels) */
{
    register View_surface *cur_vws;
    int             err;
    Ccoor           size;

    if (_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = ENOTCCPW;
    else
	err = _cgi_check_state_5();
    if (!err)
	err = _cgi_context(name);
    if (!err)
    {
	cur_vws = _cgi_vws;
	size.x = c2->x - c1->x + 1;
	size.y = c2->y - c1->y + 1;
	if ((c1->x <= c2->x) && (c1->y <= c2->y))
	{
	    if ((c1->x >= cur_vws->real_screen.r_left) &&
		(size.x <= cur_vws->real_screen.r_width) &&
		(c1->y >= cur_vws->real_screen.r_top) &&
		(size.y <= cur_vws->real_screen.r_height))
	    {
		cur_vws->vport.r_left = c1->x;
		cur_vws->vport.r_width = size.x;
		cur_vws->vport.r_top = c1->y;
		cur_vws->vport.r_height = size.y;
		cur_vws->sunview.lock_rect = cur_vws->vport;

		/* set device scaling and clipping */
		err = _cgi_windowset(cur_vws);
		if (cur_vws->sunview.gp_att != (Gp1_attr *) NULL)
		    _cgi_update_gp1_xform(cur_vws);
	    }
	    else
		err = EBDVIEWP;
	}
	else
	    err = EBADRCTD;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  clip_indicator			 		    */
/*                                                                          */
/*		Defines clipping rules.					    */
/****************************************************************************/

Cerror          clip_indicator(cflag)
Cclip           cflag;		/* on or off */
{
    int             ivw;
    Cerror          err, terr;

    err = _cgi_check_state_5();
    if (!err)
    {
	switch (cflag)
	{
	case NOCLIP:
	    _cgi_pix_mode |= PIX_DONTCLIP;
	    break;
	case CLIP:
	    /* since PIX_CLIP is 0, _cgi_pix_mode |= PIX_CLIP is a no-op */
	    _cgi_pix_mode &= ~PIX_DONTCLIP;
	    break;
	case CLIP_RECTANGLE:
	    /* since PIX_CLIP is 0, _cgi_pix_mode |= PIX_CLIP is a no-op */
	    _cgi_pix_mode &= ~PIX_DONTCLIP;
	    if (_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
		err = ENOTCCPW;
	    break;
	}
    }
    _cgi_state.common_att->vdc.clip.indicator = cflag;
    ivw = 0;
    if (!err)
    {
	while (_cgi_bump_all_vws(&ivw))
	{
	    _cgi_att->vdc.clip.indicator = cflag;
	    terr = _cgi_reset_clipping(_cgi_vws);
	    if (terr && !err)		/* capture first error */
		err = terr;
	    if (_cgi_vws->sunview.gp_att != (Gp1_attr *) 0)
		_cgi_update_gp1_xform(_cgi_vws);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  clip_rectangle		 			    */
/*                                                                          */
/*		Defines clipping rectangle in VDC space			    */
/****************************************************************************/

Cerror          clip_rectangle(xmin, xmax, ymin, ymax)
Cint            xmin, xmax, ymin, ymax;
 /* bottom left-hand and top right-hand corner of clipping rectangle */
{
    register Outatt *common_att = _cgi_state.common_att;
    VdcRect         new_clip;
    int             ivw;
    Cerror          err, terr;

    err = _cgi_check_state_5();
    if (!err && _cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = ENOTCCPW;
    if (!err)
    {
	/*
	 * The clipping region includes values == xmax,ymax. In other words, a
	 * pixel is unclipped (ie. drawn) if its coordinates are (min <= coord
	 * && coord <= max)
	 */
	new_clip.r_left = xmin;
	new_clip.r_bottom = ymin;
	new_clip.r_width = xmax - xmin + 1;
	new_clip.r_height = ymax - ymin + 1;
	if ((xmin <= xmax) && (ymin <= ymax))	/* = added */
	{
	    if ((xmin < xmax) && (ymin < ymax))	/* previously != */
	    {
		if ((new_clip.r_left >= common_att->vdc.rect.r_left)
		    && (new_clip.r_bottom >= common_att->vdc.rect.r_bottom)
		    && (new_clip.r_width <= common_att->vdc.rect.r_width)
		    && (new_clip.r_height <= common_att->vdc.rect.r_height))
		{
		    common_att->vdc.clip.rect = new_clip;

		    /* reset clipping on all open windows */
		    ivw = 0;
		    while (_cgi_bump_vws(&ivw))
		    {
			terr = _cgi_windowset(_cgi_vws);
			if (terr && !err)
			err = terr;		/* capture first error */
			if (_cgi_vws->sunview.gp_att != (Gp1_attr *) 0)
			    _cgi_update_gp1_xform(_cgi_vws);
		    }
		}
		else
		    err = ECLIPTOL;
	    }
	    else
		err = ECLIPTOS;
	}
	else
	    err = EBADRCTD;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_windowset	 					    */
/*                                                                          */
/*		Sets VDC space to screen space conversion factors	    */
/*                                                                          */
/*		This routine maps min_vdc to the center of the left-most    */
/*		(resp. bottom-most) pixel, and max vdc to the center of	    */
/*		the right-most (resp. top-most) pixel.  Since CGI relies    */
/*		on pixwin clipping, and does not clip vectors in VDC	    */
/*		space prior to coordinate conversion, this will result in   */
/*		some coordinates outside the specified VDC range being	    */
/*		drawn, rather than being clipped.  This matches the GP	    */
/*		behavior, however, and a mismatch with the GP would be	    */
/*		more noticeable than a slight deviation from the	    */
/*		standard in clipping.					    */
/****************************************************************************/

Cerror          _cgi_windowset(vwsP)
register View_surface *vwsP;
{
    register Outatt *attP = vwsP->att;
    int             vport_xsize, vport_ysize;
    int             pix_vdc_xsize, pix_vdc_ysize;
    int             vdc_xsize, vdc_ysize;
    int             win_size, vdc_size;
    int             xleft, ybottom;
    float           winratio, vdcratio;
    Cerror          err = NO_ERROR;

    START_CRITICAL();

    attP = vwsP->att;
    vwsP->xform.off = _cgi_00;
    if (attP->vdc.use_pw_size)	/* wds added 850614 for null-transform pixwin
				 * vws */
    {
	IDENTITY_SCALE(vwsP->xform.scale.x);
	IDENTITY_SCALE(vwsP->xform.scale.y);
    }
    else
    {
	/*
	 * The meaning of min and max for vport, vdc, r_screen, etc. is: an
	 * in-range number X may take on values: min <= X < max
	 */
	vport_xsize = vwsP->vport.r_width;
	vport_ysize = vwsP->vport.r_height;
	vdc_xsize = abs(attP->vdc.rect.r_width);
	vdc_ysize = abs(attP->vdc.rect.r_height);
	winratio = (float) vport_xsize / (float) vport_ysize;
	vdcratio = (float) vdc_xsize / (float) vdc_ysize;

	if (winratio < vdcratio)
	{
	    win_size = vport_xsize - 1;
	    vdc_size = vdc_xsize - 1;
	}
	else
	{
	    win_size = vport_ysize - 1;
	    vdc_size = vdc_ysize - 1;
	}
	vwsP->xform.scale.x.num = win_size << 1;
	vwsP->xform.scale.x.den = vdc_size;
	vwsP->xform.scale.y.num = (-win_size) << 1;
	vwsP->xform.scale.y.den = vdc_size;
	_cgi_devscalen(vdc_xsize - 1, pix_vdc_xsize);
	_cgi_devscalen(vdc_ysize - 1, pix_vdc_ysize);
	pix_vdc_xsize++;
	pix_vdc_ysize++;

	/*
	 * Center VDC rectangle within viewport.
	 * Pix_vdc_xsize and pix_vdc_ysize are always <= vport_xsize
	 * and vport_ysize, respectively
	 */
	vwsP->xform.off.x += ((vport_xsize - pix_vdc_xsize) / 2);
	vwsP->xform.off.y += ((vport_ysize - pix_vdc_ysize) / 2);

	/*
	 * Account for offset of viewport within window (ie. relative
	 * to real_screen).
	 */
	vwsP->xform.off.x += vwsP->vport.r_left;
	vwsP->xform.off.y += vwsP->vport.r_top;

	/*
	 * Scaling vdc_xmin and vdc_ymin may yield non-zero values,
	 * so offset for that.
	 */
	_cgi_dev_scale_only(attP->vdc.rect.r_left,
			    attP->vdc.rect.r_bottom,
			    xleft, ybottom);
	vwsP->xform.off.x -= xleft;
	vwsP->xform.off.y -= ybottom;

	/**
	 ** The transform handles the problem of Y positive == up in VDC
	 ** but Y positive == down in DC by doing
	 **	DC = (window_height - 1) - (VDC * scaling)
	 ** This is acheived by making the scaling negative, and offsetting
	 ** by (window_height - 1).
	 **/
	vwsP->xform.off.y += pix_vdc_ysize - 1;
    }
    _cgi_reset_intatt(vwsP);	/* reset relevant internal attributes */
    err = _cgi_reset_clipping(vwsP);	/* reset internal clipping parameters */

    /*
     * Delay version stamping the changed attributes until everything is done.
     */
    _cgi_state.vdc_change_cnt++;

    END_CRITICAL();
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_clipping 					    */
/*                                                                          */
/*		Resets clip region after a change in vdc extent,	    */
/*		window size, clip rectangle, clip indicator, or		    */
/*		viewport size.						    */
/*                                                                          */
/*		This assumes that _cgi_windowset has already been done,	    */
/*		so that the edges of VDC space fit within the viewport.	    */
/****************************************************************************/

Cerror          _cgi_reset_clipping(vwsP)
register View_surface *vwsP;
{
    register Outatt *attP = vwsP->att;
    Cint            xmin, xmax, ymin, ymax;
    Cint            vxmin, vxmax, vymin, vymax;
    Cerror          err = NO_ERROR;

    START_CRITICAL();

    /*
     * Don't change the pw_region in CGIPW mode.  This test should be the same
     * as the one in _cgi_windowset, so _cgi_att.vdc.use_pw_size is the correct
     * thing to test, not _cgi_state.cgipw_mode.
     */
    if (!attP->vdc.use_pw_size)
    {
	if (vwsP->sunview.pw != vwsP->sunview.orig_pw
	&&  vwsP->sunview.pw != (Pixwin *) NULL)
	{
	    pw_close(vwsP->sunview.pw);
	    vwsP->sunview.pw = (Pixwin *) NULL;
	}
	if (attP->vdc.clip.indicator == CLIP_RECTANGLE)
	{
	    vxmin = attP->vdc.clip.rect.r_left;
	    vxmax = attP->vdc.clip.rect.r_left + attP->vdc.clip.rect.r_width -1;
	    vymin = attP->vdc.clip.rect.r_bottom;
	    vymax = attP->vdc.clip.rect.r_bottom+attP->vdc.clip.rect.r_height-1;
	}
	else
	{
	    vxmin = attP->vdc.rect.r_left;
	    vxmax = attP->vdc.rect.r_left + attP->vdc.rect.r_width - 1;
	    vymin = attP->vdc.rect.r_bottom;
	    vymax = attP->vdc.rect.r_bottom + attP->vdc.rect.r_height - 1;
	}

	/*
	 * Transform the bottom-left and top-right corners of the clip region,
	 * and set a pw_region corresponding to this area. It is better not to
	 * use _cgi_devscalen for this, since the result obtained for the
	 * top-right corner could be different if the scaled width is added to
	 * the scaled origin, since each is rounded separately before adding.
	 */
	_cgi_devscale_clip(vxmin, vymin, xmin, ymax);
	_cgi_devscale_clip(vxmax, vymax, xmax, ymin);
	vwsP->conv.clip.r_left = xmin;
	vwsP->conv.clip.r_width = xmax - xmin + 1;
	vwsP->conv.clip.r_top = ymin;
	vwsP->conv.clip.r_height = ymax - ymin + 1;

	if (attP->vdc.clip.indicator != NOCLIP)
	{
	    vwsP->sunview.pw = pw_region(vwsP->sunview.orig_pw,
					 vwsP->conv.clip.r_left,
					 vwsP->conv.clip.r_top,
					 vwsP->conv.clip.r_width,
					 vwsP->conv.clip.r_height);
	    if (vwsP->sunview.pw == (Pixwin *) NULL)
		err = EMEMSPAC;
	}
	else
	{
	    vwsP->sunview.pw = vwsP->sunview.orig_pw;
	}
    }

    END_CRITICAL();
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_intatt 					    */
/*                                                                          */
/*		resets attributes that depend on screen size		    */
/****************************************************************************/

_cgi_reset_intatt(vws)
View_surface   *vws;
{
    _cgi_set_conv_line_width(vws);
    _cgi_set_conv_marker_size(vws);
    _cgi_set_conv_perimeter_width(vws);
    _cgi_reset_internal_text(vws);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_get_r_screen 					    */
/*                                                                          */
/*		Determine what screen extent we're mapping to.		    */
/*		Returns L_FALSE if CGI should not do damage repair.	    */
/****************************************************************************/

Clogical        _cgi_get_r_screen(vws, rect)
View_surface   *vws;
Rect           *rect;
{
    register Canvas canvas;

    /*
     * If the output surface is a canvas, use the canvas width and height.
     * Otherwise, use the pixwin region.
     */
    if ( (_cgi_win_get)  &&	/* we have a "window_get" func pointer */
         ((canvas = vws->sunview.canvas) != (Canvas) 0) )
    {
	rect->r_left = 0;
	rect->r_top = 0;
	rect->r_width = (int) (*_cgi_win_get)(canvas, CANVAS_WIDTH);
	rect->r_height = (int) (*_cgi_win_get)(canvas, CANVAS_HEIGHT);
	return (L_FALSE);
    }
    else if (vws->sunview.orig_pw->pw_clipdata != NULL
	     && vws->sunview.orig_pw->pw_clipdata->pwcd_regionrect != 0)
    {
	pw_get_region_rect(vws->sunview.orig_pw, rect);
	return (L_TRUE);
    }
    else
    {
	win_getsize(vws->sunview.windowfd, rect);
	return (L_TRUE);
    }
}
