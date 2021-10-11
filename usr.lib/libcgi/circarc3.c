#ifndef lint
static char	sccsid[] = "@(#)circarc3.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI circular arc  functions
 */

/*
circular_arc_3pt_close
cgipw_circular_arc_3pt_close
_cgi_circular_arc_3pt_close
circular_arc_3pt
cgipw_circular_arc_3pt
_cgi_circular_arc_3pt
_cgi_determine_center_arc
_cgi_check_colinear
*/
/*
bugs:
3pt test
overlap in box
floats
*/

#include "cgipriv.h"		/* defines types used in this file  */
#ifndef window_hs_DEFINED
#include <sunwindow/window_hs.h>
#endif
#include <math.h>

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: circular_arc_3pt_close 				    */
/*                                                                          */
/*		Draws closed circular arc starting at c1 and ending at c3   */
/*              which is guaranteed to pass through point c2.	 	    */
/*              Arc is closed with either PIE or CHORD.		 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/****************************************************************************/

Cerror          circular_arc_3pt_close(c1, c2, c3, close)
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
Cclosetype      close;		/* PIE or CHORD */
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
	    err = _cgi_circular_arc_3pt_close(_cgi_vws, c1, c2, c3, close);
	    *c1 = ca;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_circular_arc_3pt_close 				    */
/*                                                                          */
/*		Draws closed circular arc starting at c1 and ending at c3   */
/*              which is guaranteed to pass through point c2.	 	    */
/*              Arc is closed with either PIE or CHORD.		 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_circular_arc_3pt_close(desc, c1, c2, c3, close)
Ccgiwin        *desc;
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
Cclosetype      close;		/* PIE or CHORD */
{
    Cerror          err;

    err = _cgi_err_check_4();
    if (!err)
    {
	SETUP_CGIWIN(desc);
	err = _cgi_circular_arc_3pt_close(desc->vws, c1, c2, c3, close);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circular_arc_3pt_close 				    */
/*                                                                          */
/*		Draws closed circular arc starting at c1 and ending at c3   */
/*              which is guaranteed to pass through point c2.	 	    */
/*              Arc is closed with either PIE or CHORD.		 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/*	wds:	Rewritten to determine center & radius			    */
/*		And then call cgipw_circular_arc_center with them.	    */
/*                                                                          */
/*	WDS:	STILL needs code to check direction & always pass thru c2.  */
/*		(Reversing c1 and c3 args to this routine should have	    */
/*		NO EFFECT.  It should not draw the "other half" the circle. */
/****************************************************************************/

Cerror          _cgi_circular_arc_3pt_close(vws, c1, c2, c3, close)
View_surface   *vws;
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
Cclosetype      close;		/* PIE or CHORD */
{
    Cerror          err = NO_ERROR;
    int             colinear;
    Ccoor           center;	/* center */
    short           rad;

    colinear = _cgi_check_colinear(c1, c2, c3);
    if (!colinear)
	colinear = _cgi_determine_center_arc(c1, c2, c3, &center, &rad);
    if (!colinear)
	err = _cgi_circular_arc_center_close(vws, &center,
					     c1->x, c1->y,
					     c3->x, c3->y,
					     rad, close);
    else
    {
	register Outatt *attP = vws->att;
	Clinatt		templin;
	int             itemp;
	Ccoor           pts[2];

	pts[0] = *c1;
	pts[2] = *c3;
	templin = attP->line;
	itemp = vws->conv.line_width;
        /* substitute perimeter atts for line atts */
        if (attP->fill.visible == ON)
        {
	    /* WDS: CGI Manual says a line is drawn if points are colinear */
	    /* MDH: With current *edge* color & other attrs. */
            attP->line.style = attP->fill.pstyle;
            attP->line.width = attP->fill.pwidth;
            attP->line.color = attP->fill.pcolor;
            vws->conv.line_width = vws->conv.perimeter_width;
        }
	else
	{
	    /* WDS: CGI Manual says a line is drawn if points are colinear */
	    /* MDH: draw a width 1, fill-color line */
            attP->line.style = SOLID;
            attP->line.width = 1.0;
            attP->line.color = attP->fill.color;
            vws->conv.line_width = 1;
	}
	err = _cgi_polyline(vws, 2, pts, POLY_DONTCLOSE, XFORM);
	attP->line = templin;
	vws->conv.line_width = itemp;
    }

    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: circular_arc_3pt	 				    */
/*                                                                          */
/*		Draws circular arc starting at c1 and ending at c3	    */
/*              which is guaranteed to pass through point c2.	 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/****************************************************************************/

Cerror          circular_arc_3pt(c1, c2, c3)
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
{
    int             ivw;
    Cerror          err;	/* error */
    Ccoor           ca;		/* JACK -- what is this var doing??? */

    err = _cgi_err_check_4();
    if (!err)
    {
	ca = *c1;
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_circular_arc_3pt(_cgi_vws, c1, c2, c3);
	    *c1 = ca;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_circular_arc_3pt	 				    */
/*                                                                          */
/*		Draws circular arc starting at c1 and ending at c3	    */
/*              which is guaranteed to pass through point c2.	 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_circular_arc_3pt(desc, c1, c2, c3)
Ccgiwin        *desc;
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
{
    Cerror          err;

    err = _cgi_err_check_4();
    if (!err)
    {
	SETUP_CGIWIN(desc);
	err = _cgi_circular_arc_3pt(desc->vws, c1, c2, c3);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_circular_arc_3pt	 				    */
/*                                                                          */
/*		Draws circular arc starting at c1 and ending at c3	    */
/*              which is guaranteed to pass through point c2.	 	    */
/*              If points are colinear (or coincident, draw line c1->c3).   */
/*                                                                          */
/*	WDS:	STILL needs code to check direction & always pass thru c2.  */
/*		(Reversing c1 and c3 args to this routine should have	    */
/*		NO EFFECT.  It should not draw the "other half" the circle. */
/****************************************************************************/

Cerror          _cgi_circular_arc_3pt(vws, c1, c2, c3)
View_surface   *vws;
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
{
    Ccoor           center;	/* center */
    short           rad;	/* radius */
    Cerror          err = NO_ERROR;
    int             colinear;

    colinear = _cgi_check_colinear(c1, c2, c3);
    if (!colinear)
	colinear = _cgi_determine_center_arc(c1, c2, c3, &center, &rad);
    if (!colinear)
	err = _cgi_circular_arc_center(vws, &center,
				    c1->x, c1->y, c3->x, c3->y, (int) rad, 0);
    else
    {
	Ccoor           pts[2];
	/* WDS: CGI Manual says a line is drawn if points are colinear */
	/* With current line color & other attrs. */
	pts[0] = *c1;
	pts[2] = *c3;
	err = _cgi_polyline(vws, 2, pts, POLY_DONTCLOSE, XFORM);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_determine_center_arc				    */
/*                                                                          */
/*              Uses three points on circumference of circle                */
/*              to determine center and radius.                             */
/*              Returns 0 if it works.		                            */
/****************************************************************************/
int             _cgi_determine_center_arc(c1, c2, c3, ca, radp)
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
Ccoor          *ca;		/* center -- an output value */
short          *radp;		/* radius -- an output value */
{
    float           xd21, yd21, xd32, yd32;
    double          rrad;
    float           b, b1, xr, yr, xdr, ydr;
    float           m10bis, m21bis;

/* 	find intersection points of three lines */
    xd21 = c2->x - c1->x;
    yd21 = c2->y - c1->y;
    xd32 = c3->x - c2->x;
    yd32 = c3->y - c2->y;
    m10bis = -xd21 / yd21;	/* slope of first perpindicular line */
    m21bis = -xd32 / yd32;	/* slope of second perpindicular line */
    /*
     * Since the two lines share point c2, if slopes are equal, they're
     * colinear. but slopes won't be equal, just very close to it?
     */
# define	EPS	0.5
    if (fabs(m21bis - m10bis) <= EPS)
	return (1);		/* Too close to colinear or coincident */

    b = (c2->y + c1->y) / 2 - m10bis * (c1->x + c2->x) / 2;
    b1 = (c2->y + c3->y) / 2 - m21bis * (c3->x + c2->x) / 2;
    xr = (b1 - b) / (m10bis - m21bis);
    yr = m10bis * xr + b;
    xr +=.5;
    yr +=.5;
    ca->x = xr;
    ca->y = yr;
    xdr = ca->x - c1->x;	/* find center */
    ydr = ca->y - c1->y;

    rrad = (xdr * xdr);
    rrad += (ydr * ydr);
    rrad = pow((double) (rrad), 0.5) + 0.5;
    *radp = rrad;

    return (0);			/* Normal return */
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_colinear 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_check_colinear(c1, c2, c3)
Ccoor          *c1, *c2, *c3;	/* starting, intermediate and ending points */
{

#define	SAME_POINT(p1,p2) ( (p1->x == p2->x)  && ( p1->y == p2->y) )
#define	X_DIST(p1,p2)		(p2->x - p1->x)	/* Final minus initial */
#define	Y_DIST(p1,p2)		(p2->y - p1->y)	/* Final minus initial */
    if (SAME_POINT(c1, c2) || SAME_POINT(c2, c3) || SAME_POINT(c3, c1))
	return (1);
    if ((X_DIST(c1, c2) == X_DIST(c2, c3)) && (Y_DIST(c1, c2) == Y_DIST(c2, c3)))
	return (1);
    if ((X_DIST(c2, c3) == X_DIST(c3, c1)) && (Y_DIST(c2, c3) == Y_DIST(c3, c1)))
	return (1);

    return (0);
}
