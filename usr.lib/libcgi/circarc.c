#ifndef lint
static char	sccsid[] = "@(#)circarc.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
circular_arc_center_close
cgipw_circular_arc_center_close
circular_arc_center
cgipw_circular_arc_center
_cgi_circular_arc_center
_cgi_setup_arc_geom
*/
/*
bugs:
3pt test
3pt development
optimize out floats
*/

#include "cgipriv.h"		/* defines types used in this file  */
#ifndef window_hs_DEFINED
#include <sunwindow/window_hs.h>
#endif

#define	PI		3.14159
#define	EPSILON		1e-5

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* current attribute information */

#include <math.h>
int             _cgi_pix_mode;

#define	NDEBUG			/* define for production */
#ifndef	NDEBUG			/* define for production */
#   include <stdio.h>		/* WDS 850327 */
#endif	NDEBUG			/* define for production */
#include <assert.h>		/* Defines null assert macro ifdef NDEBUG */
#include "cgipriv_arcs.h"	/* defines types used with arcs  */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: circular_arc_center_close 				    */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/

Cerror          circular_arc_center_close(ca, c2x, c2y, c3x, c3y, rad, close)
Ccoor          *ca;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
Cint            rad;		/* radius */
Cclosetype      close;		/* PIE or CHORD */
{
    int             ivw, err;	/* error */
    Ccoor           cb;

    err = _cgi_err_check_4();
    if (!err)
    {
	cb = *ca;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_circular_arc_center_close(_cgi_vws, &cb,
						 c2x, c2y,
						 c3x, c3y,
						 rad, close);
	    *ca = cb;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_circular_arc_center_close			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_circular_arc_center_close(desc, ca, c2x, c2y, c3x, c3y, rad, close)
Ccgiwin        *desc;
Ccoor          *ca;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
Cint            rad;		/* radius */
Cclosetype      close;		/* PIE or CHORD */

{
    SETUP_CGIWIN(desc);
    return (_cgi_circular_arc_center_close(desc->vws, ca,
					   c2x, c2y,
					   c3x, c3y,
					   rad, close));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circular_arc_center_close				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_circular_arc_center_close(vws, ca, c2x, c2y, c3x, c3y, rad, close)
register View_surface *vws;
Ccoor          *ca;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
Cint            rad;		/* radius */
Cclosetype      close;		/* PIE or CHORD */

{
    register Outatt *attP = vws->att;
    int             err;
    Ccoor	    pts[3];
    static Clinatt  templin;
    int             itemp;

    err = 0;
    if (rad < 0)
    {
	/* Documentation (pg 3-7) implies 180 degrees opposite is used. */
	c2x = ca->x + ca->x - c2x;
	c2y = ca->y + ca->y - c2y;
	c3x = ca->x + ca->x - c3x;
	c3y = ca->y + ca->y - c3y;
	rad = -rad;
    }
    pts[0].x = c2x;
    pts[0].y = c2y;
    pts[1].x = pts[2].x = c3x;		/* save twice covers either closure */
    pts[1].y = pts[2].y = c3y;
    err += _cgi_check_ang(*ca, pts[0], rad);
    err += _cgi_check_ang(*ca, pts[1], rad);
    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
    if (err == 0)
    {
	/* substitute perimeter atts for line atts */
	if (attP->fill.visible == ON)
	{
	    templin = attP->line;
	    attP->line.style = attP->fill.pstyle;
	    attP->line.width = attP->fill.pwidth;
	    itemp = vws->conv.line_width;
	    vws->conv.line_width = vws->conv.perimeter_width;
	    attP->line.color = attP->fill.pcolor;
	}
	switch (close)
	{
	case (CHORD):
	    {
		switch (attP->fill.style)
		{
		case SOLIDI:
		    {
			_cgi_fill_partial_circle(ca, &pts[0], &pts[1], rad);
			break;
		    }
		case HOLLOW:
		    {
			break;
		    }
		case PATTERN:
		    {
			_cgi_fill_partial_pattern_circle(ca, &pts[0], &pts[1],
					    rad, attP->fill.pattern_index, 1);
			break;
		    }
		case HATCH:
		    {
			_cgi_fill_partial_pattern_circle(ca, &pts[0], &pts[1],
					      rad, attP->fill.hatch_index, 0);
			break;
		    }
		}
		if (attP->fill.visible == ON)
		{
		    err = _cgi_polyline(vws, 2, pts, POLY_DONTCLOSE, XFORM);
		}
		break;
	    }
	case (PIE):
	    {
		switch (attP->fill.style)
		{
		case SOLIDI:
		    {
			_cgi_fill_partial_pie_circle(ca, &pts[0], &pts[1], rad);
			break;
		case HOLLOW:
			{
			    break;
			}
		case PATTERN:
			{
			    _cgi_fill_partial_pattern_pie_circle(
						 ca, &pts[0], &pts[1], rad,
						 attP->fill.pattern_index, 1);
			    break;
			}
		case HATCH:
			{
			    _cgi_fill_partial_pattern_pie_circle(
						   ca, &pts[0], &pts[1], rad,
						   attP->fill.hatch_index, 0);
			    break;
			}
		    }
		}
		if (attP->fill.visible == ON)
		{
		    pts[1] = *ca;
		    err = _cgi_polyline(vws, 3, pts, POLY_DONTCLOSE, XFORM);
		}
		break;
	    }
	};
	if (err == 0  &&  attP->fill.visible == ON)
	{
	    (void) circular_arc_center(ca, c2x, c2y, c3x, c3y, rad);
	    attP->line = templin;
	    vws->conv.line_width = itemp;
	}
    }
    else
	err = EARCPNCI;
    pw_unlock(vws->sunview.pw);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: circular_arc_center					    */
/*                                                                          */
/****************************************************************************/

Cerror          circular_arc_center(c1, c2x, c2y, c3x, c3y, rad)
Ccoor          *c1;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
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
	    err = _cgi_circular_arc_center(_cgi_vws, &ca,
					   c2x, c2y, c3x, c3y, rad, 0);
	    *c1 = ca;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_circular_arc_center				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_circular_arc_center(desc, c1, c2x, c2y, c3x, c3y, rad)
Ccgiwin        *desc;
Ccoor          *c1;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
Cint            rad;		/* radius */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_circular_arc_center(desc->vws, c1, c2x, c2y, c3x, c3y, rad, 0);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circular_arc_center	 			    */
/*                                                                          */
/*                                                                          */
/*		draws closed arc centered at c1 with radius rad             */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/
Cerror          _cgi_circular_arc_center(vws, c1,
			                       c2x, c2y, c3x, c3y, rad, check)
View_surface   *vws;
Ccoor          *c1;		/* center */
Cint            c2x, c2y, c3x, c3y;	/* endpoints */
Cint            rad;		/* radius */
short           check;		/* 3pt check flag */
{
    Outatt         *attP = vws->att;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    int             srad;
    int             tot, err;
    int             color, width;
    Clintype        style;
    Ccoor           c2, c3;

    if (rad < 0)
    {
	/* Documentation (pg 3-7) implies 180 degrees opposite is used. */
	c2x = c1->x + c1->x - c2x;
	c2y = c1->y + c1->y - c2y;
	c3x = c1->x + c1->x - c3x;
	c3y = c1->y + c1->y - c3y;
	rad = -rad;
    }
    c2.x = c2x;
    c2.y = c2y;
    c3.x = c3x;
    c3.y = c3y;
    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(c2.x, c2.y, x2, y2);
    _cgi_devscale(c3.x, c3.y, x3, y3);
    style = attP->line.style;
    color = attP->line.color;
    width = vws->conv.line_width;

    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);

    /*
     * If check != 0, the user called a 3pt arc, * then endpoints have already
     * been checked; * otherwise, check endpoints.
     */
    if (!check)
    {
	tot = _cgi_check_ang(*c1, c2, rad);
	tot += _cgi_check_ang(*c1, c3, rad);
    }
    else
	tot = 0;
/*     (void) printf ("tot %d \n", tot); */
    if (tot != 0)
	err = 64;
    else
    {
	err = 0;
	if (rad)
	{
	    _cgi_devscalen(rad, srad);
	    if (width > srad)
		width = srad;
/*
 * The special-case code for width 1 circular arcs was disabled to fix two
 * problems:  1) Bug 1002999, arc segments of width 1 were drawn in the
 * wrong direction (ie. from point 2 to point 1, rather than from point 1
 * to point 2), and 2) pr_curve() was not ported for the SPARC architecture,
 * so SYS4/3.4 needed this change to not use the stuff in circfast.c, which
 * used pr_curve.
 *	WCL	2/5/87
 */
#ifdef	ADD_QWIKTEST
	    if (width > 1)
#endif	ADD_QWIKTEST
	    {
		/* geometric info for a circular or elliptical arc */
		struct cgi_arc_geom arc_info;
		arc_info.xc = x1, arc_info.yc = y1;
		arc_info.x2 = x2, arc_info.y2 = y2;
		arc_info.x3 = x3, arc_info.y3 = y3;
		_cgi_setup_arc_geom(&arc_info, srad, srad, width);
		_cgi_partial_textured_circle(&arc_info,
				      srad, (srad + width - 1), color, style);
	    }
#ifdef	ADD_QWIKTEST
	    else
	    {
		_cgi_textured_arc(x1, y1, x2, y2, x3, y3, srad, 1, color, style);
	    }
#endif	ADD_QWIKTEST
	}
	else
	    /*
	     * pw_put (vws->sunview.pw, x1, y1, color); doesn't use a raster
	     * "op" code
	     */
	    pw_vector(vws->sunview.pw, x1, y1, x1, y1, _cgi_pix_mode, color);
    }
    pw_unlock(vws->sunview.pw);
    return (err);
}



/****************************************************************************/
/*		Defines clipping region for circular or elliptical arc.     */
/*		center and two endpoints must already be in structure.      */
/*		Flags are set as well, to help the arc-clipper.             */
/****************************************************************************/
_cgi_setup_arc_geom(arc_geom, xrad, yrad, width)
struct cgi_arc_geom *arc_geom;		/* ptr to geometric info for arc */
int             xrad, yrad, width;
{
    int             ccode = 0, xcode = 0, ycode = 0;
    /* wds: ccode and ycode are for debugging only: which fork did we take? */

    short           x1, y1, x2, y2, x3, y3;
#define	xb2	arc_geom->left
#define	xb3	arc_geom->right
#define	yb2	arc_geom->top
#define	yb3	arc_geom->bottom
/* Second "box" (needed in some arc_type cases): */
#define	xs2	arc_geom->sleft
#define	xs3	arc_geom->sright
#define	ys2	arc_geom->stop
#define	ys3	arc_geom->sbottom
/* Important to cut XMAJOR octants with VERTICAL lines & vice versa. */
#define XMAJ_OCT(xcoord,ycoord)	( abs(xcoord - x1) <= abs(ycoord - y1) )
#define YMAJ_OCT(xcoord,ycoord)	( abs(xcoord - x1) >= abs(ycoord - y1) )
    x1 = arc_geom->xc, y1 = arc_geom->yc;
    x2 = arc_geom->x2, y2 = arc_geom->y2;
    x3 = arc_geom->x3, y3 = arc_geom->y3;

    arc_geom->arc_type = UNKNOWN;

    if (((y2 - y1 == 0) && (y3 - y1) == 0))
	ccode = 1, arc_geom->do_outside = (x3 - x1) >= (x2 - x1) ? OUTSIDE : INSIDE;
    else if (((x3 - x1 == 0) && (x2 - x1) == 0))
	ccode = 2, arc_geom->do_outside = (y2 - y1) >= (y3 - y1) ? OUTSIDE : INSIDE;
    else			/* Typical case follows */
	ccode = 3, arc_geom->do_outside =
	    ((y2 - y1) * (x3 - x1) < (y3 - y1) * (x2 - x1)) ? OUTSIDE : INSIDE;
/*dbx	p	ccode, arc_geom->do_outside 	*/

#define X_ARC_RADIUS	(xrad+width+2)
#define Y_ARC_RADIUS	(yrad+width+2)
#define PULL_IN			0	/* In case we change our mind */
    /* determine X coordinates of circle box: xb2 is left */
    if ((x2 <= x1) && (x3 <= x1))
    {				/* Both to left of center */
	xb2 = x1 - X_ARC_RADIUS;/* left of circle */
	if (x2 >= x3)		/* MAX(x2,x3) is closer to x1 */
	    xcode = 1, xb3 = x2 + PULL_IN;
	else
	    xcode = 2, xb3 = x3 + PULL_IN;
    }
    else if ((x2 >= x1) && (x3 >= x1))
    {				/* Both to right of center */
	if (x2 <= x3)		/* MIN(x2,x3) is closer to x1 */
	    xcode = 3, xb2 = x2 - PULL_IN;
	else
	    xcode = 4, xb2 = x3 - PULL_IN;
	xb3 = x1 + X_ARC_RADIUS;/* right of circle */
    }
    else
	/*
	 * (xcode>4) is flag that left and right were not trivially set
     */ if ((x2 <= x1) && (x3 >= x1))
    {				/* Want one side to be whichever is closer to
				 * x1 */
	/*
	 * If xs and ys are equally close to center, use x2 as closer. * But if
	 * xs are equally close to center and ys are not, * use point with y
	 * further from center.
	 */
	if ((-(x2 - x1) < (x3 - x1))
	    || ((-(x2 - x1) == (x3 - x1)) && (abs(y2 - y1) >= abs(y3 - y1)))
	    )
	{			/* x2 is closer to x1 */
	    xcode = 5, xb2 = x2 + PULL_IN;
	    xb3 = x1 + X_ARC_RADIUS;	/* far right */
	}
	else
	{			/* x3 is closer to x1 */
	    xcode = 6, xb2 = x1 - X_ARC_RADIUS;	/* far left */
	    xb3 = x3 - PULL_IN;
	}
    }
    else if ((x2 >= x1) && (x3 <= x1))
    {				/* Want one side to be whichever is closer to
				 * x1 */
	if (((x2 - x1) < -(x3 - x1))
	    || (((x2 - x1) == -(x3 - x1)) && (abs(y2 - y1) >= abs(y3 - y1)))
	    )
	{			/* x2 is closer to x1 */
	    xcode = 7, xb3 = x2 - PULL_IN;
	    xb2 = x1 - X_ARC_RADIUS;	/* far left */
	}
	else
	{			/* x3 is closer to x1 */
	    xcode = 8, xb2 = x3 + PULL_IN;
	    xb3 = x1 + X_ARC_RADIUS;	/* far right */
	}
    }


/*dbx	p	xcode	*/
    assert(xcode != 0);
    assert(xb2 < xb3);
    /*
     * If xcode odd,  one of the box sides is from x2 (which was closer). If
     * xcode even, one of the box sides is from x3 (which was closer). The
     * other box side is beyond the circle (towards other point).
     */
#define	X2_CLOSER	(  xcode & 1  )
#define	X3_CLOSER	( (xcode & 1) == 0 )

    /* determine Y coordinates of circle box: yb2 is top */
    if ((y2 <= y1) && (y3 <= y1))
    {				/* Both endpoints above center */
	yb2 = y1 - Y_ARC_RADIUS;/* top of circle */
	if (xcode <= 4)		/* Xs on same side of center too: 1 quadrant */
	{
	    arc_geom->arc_type = SAME;
	    if (y2 < y3)	/* MAX(y2,y3) is closer to y1 */
		ycode = 1, yb3 = y3 + PULL_IN;
	    else
		ycode = 2, yb3 = y2 + PULL_IN;
	}
	else			/* Xs on different sides of center: adjacent
				 * quadrants */
	{			/* don't bother to determine OCTANTs of the
				 * endpoints */
	    /* though not necessary to use 2 rects for +- 2 octants. */
	    /* MIN(x2,x3) is LEFT  of left rect.  */
	    /* MAX(x2,x3) is RIGHT of right rect. */
	    arc_geom->arc_type = ADJACENT;
	    ys2 = y1 - Y_ARC_RADIUS;	/* top of circle */
	    xb3 = x1;		/* right side of left rect is center line */
	    xs2 = x1;		/* left  side of right rect is center line */
	    if (x2 <= x3)
	    {
		xcode = 31, xb2 = x2 + PULL_IN;
		ycode = 31, yb3 = y2 + PULL_IN;
		xs3 = x3 - PULL_IN;
		ys3 = y3 + PULL_IN;
	    }
	    else
	    {
		xcode = 32, xb2 = x3 + PULL_IN;
		ycode = 32, yb3 = y3 + PULL_IN;
		xs3 = x2 - PULL_IN;
		ys3 = y2 + PULL_IN;
	    }
	}
    }				/* Both endpoints above center */
    else if ((y2 >= y1) && (y3 >= y1))
    {				/* Both endpoints below center */
	if (xcode <= 4)		/* Xs on same side of center too: 1 quadrant */
	{
	    arc_geom->arc_type = SAME;
	    if (y2 > y3)	/* MIN(y2,y3) is closer to y1 */
		ycode = 3, yb2 = y3 - PULL_IN;
	    else
		ycode = 4, yb2 = y2 - PULL_IN;
	}
	else			/* Xs on different sides of center: adjacent
				 * quadrants */
	{			/* don't bother to determine OCTANTs of the
				 * endpoints */
	    /* though not necessary to use 2 rects for +- 2 octants. */
	    /* MIN(x2,x3) is LEFT  of left rect.  */
	    /* MAX(x2,x3) is RIGHT of right rect. */
	    arc_geom->arc_type = ADJACENT;
	    xb3 = x1;		/* right side of left rect is center line */
	    xs2 = x1;		/* left  side of right rect is center line */
	    if (x2 <= x3)
	    {
		xcode = 33, xb2 = x2 + PULL_IN;
		ycode = 33, yb2 = y2 - PULL_IN;
		xs3 = x3 - PULL_IN;
		ys2 = y3 - PULL_IN;
	    }
	    else
	    {
		xcode = 34, xb2 = x3 + PULL_IN;
		ycode = 34, yb2 = y3 - PULL_IN;
		xs3 = x2 - PULL_IN;
		ys2 = y2 - PULL_IN;
	    }
	    ys3 = y1 + Y_ARC_RADIUS;	/* bottom of circle */
	}			/* ADJACENT quadrants */
	yb3 = y1 + Y_ARC_RADIUS;/* bottom of circle */
    }				/* Both endpoints below center */
    else
    {				/* top and bottom cannot be trivially set */
	if ((y2 <= y1) && (y3 >= y1))
	{			/* y2 above center and y3 below center */
	    if (xcode <= 4)	/* ADJACENT QUADs */
	    {
		/*
		 * won't bother to determine OCTANTs of endpts, * though not
		 * necessary to use 2 rects for points * that are only +- 2
		 * octants.
		 */
		ycode = 35, arc_geom->arc_type = ADJACENT;
		yb3 = y1;	/* bottom of top rect is center line */
		ys2 = y1;	/* top of bottom rect is center line */
		/* MIN(y2,y3) is TOP    of top    rect */
		yb2 = y2 + PULL_IN;
		/* MAX(y2,y3) is BOTTOM of bottom rect */
		ys3 = y3 - PULL_IN;
		if (xcode <= 2)
		{		/* Both to left of center */
		    xb2 = x1 - X_ARC_RADIUS;	/* already far left */
		    xs2 = x1 - X_ARC_RADIUS;	/* far left */
		    xcode = 32, xb3 = x2 + PULL_IN;
		    xs3 = x3 + PULL_IN;
		}
		else		/* xcode == 3 or 4 */
		{		/* Both to right of center */
		    assert((xcode == 3) || (xcode == 4));
		    xb3 = x1 + X_ARC_RADIUS;	/* already far right */
		    xs3 = x1 + X_ARC_RADIUS;	/* far right */
		    xcode = 33, xb2 = x2 - PULL_IN;
		    xs2 = x3 - PULL_IN;
		}
	    }			/* (xcode <= 4) : ADJACENT QUADRANTS */
	    else		/* STILL: y2 above center and y3 below center */
	    {			/* points 2 & 3 are in opposite quadrants */
		/*
		 * Note: if x's are equally close & y's are too, * X2_CLOSER
		 * will be true:  use SPECIAL code.
		 */
		if ((-(y2 - y1) <= (y3 - y1))	/* is y2 closer? */
		    ==
		    (X2_CLOSER)	/* is x2 closer? */
		    )
		{		/* SPECIAL opposite qudarants */
		     /* dbx */ arc_geom->arc_type = SPECIAL;
		    arc_geom->do_outside = INSIDE;
		    if (xcode <= 6)	/* xcode == 5 or 6 */
		    {		/* (x2 <= x1) &&  (x3 >= x1) */
			assert((xcode == 5) || (xcode == 6));
			ycode = 26, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y1;	/* center line */
			xcode = 26, xb2 = x2 + PULL_IN;
			xb3 = x1 + X_ARC_RADIUS;
			ys2 = y1 - Y_ARC_RADIUS;
			ys3 = y3 - PULL_IN;
			xs2 = x1;	/* center line */
			xs3 = x1 + X_ARC_RADIUS;
		    }
		    else	/* xcode == 7 or 8 */
		    {		/* (x2 >= x1) &&  (x3 <= x1) */
			assert((xcode == 7) || (xcode == 8));
			ycode = 28, yb2 = y1;	/* center line */
			yb3 = y1 + Y_ARC_RADIUS;
			xcode = 28, xb2 = x3 + PULL_IN;
			xb3 = x1 + X_ARC_RADIUS;
			ys2 = y2 + PULL_IN;
			ys3 = y1 + Y_ARC_RADIUS;
			xs2 = x1;	/* center line */
			xs3 = x1 + X_ARC_RADIUS;
		    }
		}		/* SPECIAL opposite qudarants */
		else		/* STILL: y2 above center and y3 below */
		{		/* OPPOSITE QUADRANTs (OPPOSITE OCTANT too?) */
		    /* Which OCTANT is point 2 in? */
		    if (XMAJ_OCT(x2, y2) && YMAJ_OCT(x3, y3))
		    {		/* OPPOSITE QUADRANTS ONLY */
			arc_geom->arc_type = OPP_QUAD;
			ycode = 42, yb2 = y1 - Y_ARC_RADIUS;	/* top */
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y3 - PULL_IN;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 16, xb2 = x2 + PULL_IN;
			    xb3 = x1 + X_ARC_RADIUS;
			    xs2 = x1;	/* center */
			    xs3 = x1 + X_ARC_RADIUS;
			}
			else
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = INSIDE;
			    xcode = 18, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x2 - PULL_IN;
			    xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x1;	/* center */
			}
		    }		/* OPPOSITE QUADRANTS ONLY */
		    else if (YMAJ_OCT(x2, y2) && XMAJ_OCT(x3, y3))
			/* STILL: y2 above center and y3 below */
		    {		/* OPPOSITE QUADRANTS ONLY */
			arc_geom->arc_type = OPP_QUAD;
			ycode = 43, yb2 = y2 - PULL_IN;
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y1 + Y_ARC_RADIUS;	/* bottom */
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = INSIDE;
			    xcode = 16, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x1;	/* center */
			    xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x3 - PULL_IN;
			}
			else
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 18, xb2 = x1;	/* center */
			    xb3 = x1 + X_ARC_RADIUS;
			    xs2 = x3 + PULL_IN;
			    xs3 = x1 + X_ARC_RADIUS;
			}
		    }		/* OPPOSITE QUADRANTS ONLY */
		    else if (XMAJ_OCT(x2, y2) && XMAJ_OCT(x3, y3))
			/* STILL: y2 above center and y3 below */
		    {		/* OPPOSITE OCTANTs */
			arc_geom->arc_type = OPP_OCTANT;
			ycode = 12, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y1 + Y_ARC_RADIUS;
			xb2 = x1 - X_ARC_RADIUS;
			xs2 = x1 - X_ARC_RADIUS;
			arc_geom->do_outside = INSIDE;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    xcode = 16, xb3 = x2 + PULL_IN;
			    xs3 = x3 - PULL_IN;
			}
			else	/* (xcode == 7) || (xcode == 8) */
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    xcode = 18, xb3 = x2 - PULL_IN;
			    xs3 = x3 + PULL_IN;
			}
		    }		/* OPPOSITE OCTANTs */
		    else	/* YMAJ_OCT(x2,y2) && YMAJ_OCT(x3,y3) */
			/* * * * * * * * STILL: y2 above center and y3 below */
		    {		/* OPPOSITE OCTANTs */
			assert(YMAJ_OCT(x2, y2));
			assert(YMAJ_OCT(x3, y3));
			arc_geom->arc_type = OPP_OCTANT;
			ycode = 13, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y2 + PULL_IN;
			ys2 = y1 - Y_ARC_RADIUS;
			ys3 = y3 - PULL_IN;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 16, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x1;	/* center line */
			    xs2 = x1;	/* center line */
			    xs3 = x1 + X_ARC_RADIUS;
			}
			else	/* (xcode == 7) || (xcode == 8) */
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = INSIDE;
			    xcode = 18, xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x1;	/* center line */
			    xb2 = x1;	/* center line */
			    xb3 = x1 + X_ARC_RADIUS;
			}
		    }		/* OPPOSITE OCTANTs */
		}		/* OPPOSITE QUADRANTs (OPPOSITE OCTANT too?) */
	    }			/* points 2 & 3 in opposite quadrants */
	}			/* ( (y2 <= y1) && (y3 >= y1) ): y2 above and
				 * y3 below */
	else
	{			/* ( (y2 >= y1) && (y3 <= y1) ): y2 below and
				 * y3 above */
	    if (xcode <= 4)	/* ADJACENT QUADRANTS */
	    {
		/*
		 * won't bother to determine OCTANTs of endpts, * though not
		 * necessary to use 2 rects for points * that are only +- 2
		 * octants.
		 */
		ycode = 37, arc_geom->arc_type = ADJACENT;
		/* MIN(y2,y3) is TOP    of top    rect */
		yb2 = y3 + PULL_IN;
		yb3 = y1;	/* bottom of top rect is center line */
		ys2 = y1;	/* top of bottom rect is center line */
		/* MAX(y2,y3) is BOTTOM of bottom rect */
		ys3 = y2 - PULL_IN;
		if (xcode <= 2)
		{		/* Both to left of center */
		    xb2 = x1 - X_ARC_RADIUS;	/* already far left */
		    xs2 = x1 - X_ARC_RADIUS;	/* far left */
		    xcode = 32, xb3 = x3 + PULL_IN;
		    xs3 = x2 + PULL_IN;
		}
		else		/* xcode == 3 or 4 */
		{		/* Both to right of center */
		    assert((xcode == 3) || (xcode == 4));
		    xb3 = x1 + X_ARC_RADIUS;	/* already far right */
		    xs3 = x1 + X_ARC_RADIUS;	/* far right */
		    xcode = 33, xb2 = x3 - PULL_IN;
		    xs2 = x2 - PULL_IN;
		}
	    }			/* (xcode <= 4) : ADJACENT QUADRANTS */
	    else		/* STILL: y2 below center and y3 above center */
	    {			/* points 2 & 3 are in opposite quadrants */
		/*
		 * Note: if x's are equally close & y's are too, * X2_CLOSER
		 * will be true:  use SPECIAL code.
		 */
		if (((y2 - y1) <= -(y3 - y1))	/* is y2 closer? */
		    ==
		    (X2_CLOSER)	/* is x2 closer? */
		    )
		{		/* SPECIAL opposite qudarants */
		     /* dbx */ arc_geom->arc_type = SPECIAL;
		    arc_geom->do_outside = OUTSIDE;
		    if (xcode <= 6)
		    {		/* (x2 <= x1) &&  (x3 >= x1) */
			assert((xcode == 5) || (xcode == 6));
			ycode = 26, yb2 = y1;	/* center line */
			yb3 = y1 + Y_ARC_RADIUS;
			xcode = 26, xb2 = x2 + PULL_IN;
			xb3 = x1 + X_ARC_RADIUS;
			ys2 = y3 + PULL_IN;
			ys3 = y1 + Y_ARC_RADIUS;
			xs2 = x1;	/* center line */
			xs3 = x1 + X_ARC_RADIUS;
		    }
		    else	/* xcode == 8 */
		    {		/* (x2 >= x1) &&  (x3 <= x1) */
			assert((xcode == 7) || (xcode == 8));
			ycode = 28, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y1;	/* center line */
			xcode = 28, xb2 = x3 + PULL_IN;
			xb3 = x1 + X_ARC_RADIUS;
			ys2 = y1 - Y_ARC_RADIUS;
			ys3 = y2 - PULL_IN;
			xs2 = x1;	/* center line */
			xs3 = x1 + X_ARC_RADIUS;
		    }
		}		/* SPECIAL opposite qudarants */
		else		/* STILL: y2 below center and y3 above */
		{		/* OPPOSITE QUADRANTs (OPPOSITE OCTANT too?) */
		    /* Which OCTANT is point 2 in? */
		    if (YMAJ_OCT(x2, y2) && XMAJ_OCT(x3, y3))
		    {		/* OPPOSITE QUADRANTS ONLY */
			arc_geom->arc_type = OPP_QUAD;
			ycode = 46, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y2 - PULL_IN;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 16, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x3 - PULL_IN;
			    xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x1;	/* center line */
			}
			else
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = INSIDE;
			    xcode = 18, xb2 = x3 + PULL_IN;
			    xb3 = x1 + X_ARC_RADIUS;
			    xs2 = x1;	/* center line */
			    xs3 = x1 + X_ARC_RADIUS;
			}
		    }		/* OPPOSITE QUADRANTS ONLY */
		    else if (XMAJ_OCT(x2, y2) && YMAJ_OCT(x3, y3))
			/* STILL: y2 below center and y3 above */
		    {		/* OPPOSITE QUADRANTS ONLY */
			arc_geom->arc_type = OPP_QUAD;
			ycode = 47, yb2 = y3 + PULL_IN;
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y1 + Y_ARC_RADIUS;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = INSIDE;
			    xcode = 16, xb2 = x1;	/* center line */
			    xb3 = x1 + X_ARC_RADIUS;
			    xs2 = x2 + PULL_IN;
			    xs3 = x1 + X_ARC_RADIUS;
			}
			else
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 18, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x1;	/* center line */
			    xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x2 - PULL_IN;
			}
		    }		/* OPPOSITE QUADRANTS ONLY */
		    else if (XMAJ_OCT(x2, y2) && XMAJ_OCT(x3, y3))
			/* STILL: y2 below center and y3 above */
		    {		/* OPPOSITE OCTANTs */
			arc_geom->arc_type = OPP_OCTANT;
			ycode = 15, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y1;	/* center line */
			ys2 = y1;	/* center line */
			ys3 = y1 + Y_ARC_RADIUS;
			xb2 = x1 - X_ARC_RADIUS;
			xs2 = x1 - X_ARC_RADIUS;
			arc_geom->do_outside = OUTSIDE;	/* lie? */
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    xcode = 16, xb3 = x3 - PULL_IN;
			    xs3 = x2 + PULL_IN;
			}
			else	/* (xcode == 7) || (xcode == 8) */
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    xcode = 18, xb3 = x3 + PULL_IN;
			    xs3 = x2 - PULL_IN;
			}
		    }		/* OPPOSITE OCTANTs */
		    else	/* YMAJ_OCT(x2,y2) && YMAJ_OCT(x3,y3) */
			/* * * * * * * * STILL: y2 below center and y3 above */
		    {		/* OPPOSITE OCTANTs */
			assert(YMAJ_OCT(x2, y2));
			assert(YMAJ_OCT(x3, y3));
			arc_geom->arc_type = OPP_OCTANT;
			ycode = 16, yb2 = y1 - Y_ARC_RADIUS;
			yb3 = y2 + PULL_IN;
			ys2 = y1 - Y_ARC_RADIUS;
			ys3 = y3 - PULL_IN;
			if (xcode <= 6)
			{	/* x2 left && x3 right */
			    assert((xcode == 5) || (xcode == 6));
			    arc_geom->do_outside = OUTSIDE;
			    xcode = 16, xb2 = x1 - X_ARC_RADIUS;
			    xb3 = x1;	/* center line */
			    xs2 = x1;	/* center line */
			    xs3 = x1 + X_ARC_RADIUS;
			}
			else	/* (xcode == 7) || (xcode == 8) */
			{	/* x3 left && x2 right */
			    assert((xcode == 7) || (xcode == 8));
			    arc_geom->do_outside = INSIDE;
			    xcode = 18, xs2 = x1 - X_ARC_RADIUS;
			    xs3 = x1;	/* center line */
			    xb2 = x1;	/* center line */
			    xb3 = x1 + X_ARC_RADIUS;
			}
		    }		/* OPPOSITE OCTANTs */
		}		/* OPPOSITE QUADRANTs (OPPOSITE OCTANT too?) */
	    }			/* points 2 & 3 are in opposite quadrants */
	}			/* ( (y2 >= y1) && (y3 <= y1) ):  y2 below and
				 * y3 above */
    }				/* top and bottom cannot be trivially set */

#ifndef	NDEBUG			/* define for production: then exclude the
				 * following */
    assert(xb2 <= xb3);
    assert(yb2 <= yb3);
/*dbx	p	ccode, xcode, ycode	*/
/*dbx*	p	*arc_geom	 */
    switch (arc_geom->arc_type)
    {
    case UNKNOWN:
	{
	    assert(0);
	    break;
	}

    case SAME:
	{
/*dbx*      source dbxbox	;  Same   quads */
	    assert((xcode < 10) && (ycode < 10));
	    break;
	}

    case ADJACENT:
	{
/*dbx*      source dbxadj	;  Adjacent  quads */
	    assert((xcode >= 30) && (ycode >= 30));
	    assert(xs2 <= xs3);
	    assert(ys2 <= ys3);
	    break;
	}

    case OPP_QUAD:
	{
/*dbx*	    source dbxquad	;  Opposite Quads, but not Octs */
	    assert(ycode > 40);
	    assert((xcode > 10) && (xcode < 20));
	    break;
	}

    case OPP_OCTANT:
	{
/*dbx*	    source dbxoct	;  Opposite Octs as well as Quads */
	    assert((xcode > 10) && (xcode < 20));
	    assert(ycode > 10 && ycode < 20);
	    break;
	}

    case SPECIAL:
	{
/*dbx*	    source dbxspec	;  Special opps */
	    assert(ycode > 20 && ycode < 30);
	    assert(xs2 <= xs3);
	    assert(ys2 <= ys3);
	    break;
	}

    }				/* end switch (arc_geom->arc_type) */
#endif	NDEBUG			/* define for production: the above will be
				 * excluded */

#ifdef lint
    /*
     * wds: ccode and ycode are for debugging only: They tell which fork we
     * took.  This code makes lint shut up about it.
     */
    ccode = ccode;
    ycode = ycode;
#endif lint

#undef	xb2	arc_geom->left
#undef	xb3	arc_geom->right
#undef	yb2	arc_geom->top
#undef	yb3	arc_geom->bottom
#undef	xs2	arc_geom->sleft	/* for "special" opposites */
#undef	xs3	arc_geom->sright
#undef	ys2	arc_geom->stop
#undef	ys3	arc_geom->sbottom
#undef XMAJ_OCT(xcoord,ycoord)
#undef YMAJ_OCT(xcoord,ycoord)
}
