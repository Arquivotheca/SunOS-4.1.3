#ifndef lint
static char	sccsid[] = "@(#)circarc2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
_cgi_check_ang
_cgi_partial_textured_circle
_cgi_partial_ang_textured_circ_pts
_cgi_pcirc_vector
_cgi_fill_partial_circle
_cgi_arc_fill_pts
_cgi_vector_arc_clip
_cgi_setup_pie_geom
_cgi_fill_partial_pie_circle
_cgi_partial_pie_circ_fill_pts
_cgi_vector_pcirc_pie_clip
_cgi_vector_pcirc_pie_overlap_cross 
*/

#include "cgipriv.h"
#include "cgipriv_arcs.h"

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */

#include <math.h>
Pixrect *_cgi_circrect;
static int      pattern2a[6][9] =
{
 {
  9000, 46, 0, 0, 0, 0, 0, 0, 0
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

short           _cgi_texture[8];


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_ang	 					    */
/*                                                                          */
/*		Check that angular endpoints are actually on the same circle*/
/****************************************************************************/

int
_cgi_check_ang(p1, p2, rad)
Ccoor           p1, p2;
int             rad;
{
    int             err;
    /* (After discussion with VP, 850401.) To allow an error of about 1
     * pixel on a full screen, we allow an error of 1 times 32K(VDC)/1K(typical
     * screen). ERROR = (rad + error)**2 = rad**2 + 2*rad*error + error**2. The
     * term error**2 is assumed negligible. We compare the sum of the squares
     * to ERROR by comparing (sum of squares) - (rad**2) to 2*rad*error, about
     * 64*rad. */
    err = 64 * rad;
#define	XSQUARES	( (p1.x - p2.x)*(p1.x - p2.x) )
#define	YSQUARES	( (p1.y - p2.y)*(p1.y - p2.y) )
#define	RADIUS_SQUARES	( rad*rad )
#define	SUM_OF_SQUARES	( XSQUARES + YSQUARES )
    return (abs(SUM_OF_SQUARES - RADIUS_SQUARES) > err);
#undef	XSQUARES
#undef	YSQUARES
#undef	SUM_OF_SQUARES
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_textured_circle				    */
/*                                                                          */
/*		draws circle with coors in screen space			    */
/****************************************************************************/
_cgi_partial_textured_circle(arc_geom, irad, orad, val, style)
struct cgi_arc_geom *arc_geom;		/* ptr to geometric info for arc */
int             irad;		/* inside radius */
int             orad;		/* outside radius */
short           val;
Clintype        style;
{
    int             i_convf, o_convf, i_angsum, o_angsum;	/* inner & outer circles */
    int             ix, iy, id;	/* x,y and d for inner circle */
    int             ox, oy, od;	/* x,y and d for outer circle */
    int             i, k, ptc;

    /* count number of points in an exterior octant */
    o_convf = _cgi_oct_circ_radius(orad);
    /* count number of points in an interior octant */
    i_convf = _cgi_oct_circ_radius(irad);
    for (k = 0; k < 8; k++)
	_cgi_texture[k] = 1;
    ix = 0;			/* x,y and d for inner circle */
    iy = irad;
    id = 3 - 2 * irad;
    ox = 0;			/* x,y and d for outer circle */
    oy = orad;
    od = 3 - 2 * orad;
    i = 0;
    o_angsum = i_angsum = (pattern2a[(int) style][0] << 10) / 2;
    ptc = pattern2a[(int) style][0] / 2;
    /* number of points in an inner circle pattern element */
    _cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
/*     angsum1 = pattern2a[(int) style][0] / 2; */
    while (ix < iy)	/* was ox < oy prior to 851031, wds */
    {
	if (id < 0)
	    id = id + 4 * ix + 6;
	else
	{
	    id = id + 4 * (ix - iy) + 10;
	    _cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
	    iy = iy - 1;
	    _cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
	}
	ix += 1;
	ptc++;
	i_angsum += i_convf;
	while (o_angsum <= i_angsum)
	{
	    if (od < 0)
	    {
		_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
		od = od + 4 * ox + 6;
		_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
	    }
	    else
	    {
		od = od + 4 * (ox - oy) + 10;
		_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
		oy = oy - 1;
		_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
	    }
	    ox += 1;
	    _cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
	    o_angsum += o_convf;
	}
	if (ptc >= pattern2a[(int) style][i])
	{
	    i++;
	    if ((i % 2) == 0)
		for (k = 0; k < 8; k++)
		    _cgi_texture[k] = 1;
	    else
		for (k = 0; k < 8; k++)
		    _cgi_texture[k] = 0;
	    o_angsum -= i_angsum;
	    i_angsum = 0;
	    ptc = 0;
	    i &= 7;
	}
    }
    if (ix == iy)		/* wds added 850430 */
	_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, val);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_ang_textured_circ_pts			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

_cgi_partial_ang_textured_circ_pts(arc_geom, ix, iy, ox, oy, color)
struct cgi_arc_geom *arc_geom;		/* ptr to geometric info for arc */
short           ix, iy, ox, oy, color;
{				/* draw symmetry points of circle */
    short           ia1, ia2, ia3, ia4, ib1, ib2, ib3, ib4;
    short           oa1, oa2, oa3, oa4, ob1, ob2, ob3, ob4;

      /* Made gaps not write _cgi_background_color: just leave alone. */
    if (_cgi_texture[1] == 1)
    {
	ia1 = arc_geom->xc + ix;
	ia2 = arc_geom->xc + iy;
	ia3 = arc_geom->xc - ix;
	ia4 = arc_geom->xc - iy;
	ib1 = arc_geom->yc + ix;
	ib2 = arc_geom->yc + iy;
	ib3 = arc_geom->yc - ix;
	ib4 = arc_geom->yc - iy;

	oa1 = arc_geom->xc + ox;
	oa2 = arc_geom->xc + oy;
	oa3 = arc_geom->xc - ox;
	oa4 = arc_geom->xc - oy;
	ob1 = arc_geom->yc + ox;
	ob2 = arc_geom->yc + oy;
	ob3 = arc_geom->yc - ox;
	ob4 = arc_geom->yc - oy;

	_cgi_pcirc_vector(ia4, ib1, oa4, ob1, color, arc_geom);
	_cgi_pcirc_vector(ia2, ib1, oa2, ob1, color, arc_geom);
	_cgi_pcirc_vector(ia3, ib2, oa3, ob2, color, arc_geom);
	_cgi_pcirc_vector(ia1, ib2, oa1, ob2, color, arc_geom);
	_cgi_pcirc_vector(ia4, ib3, oa4, ob3, color, arc_geom);
	_cgi_pcirc_vector(ia2, ib3, oa2, ob3, color, arc_geom);
	_cgi_pcirc_vector(ia3, ib4, oa3, ob4, color, arc_geom);
	_cgi_pcirc_vector(ia1, ib4, oa1, ob4, color, arc_geom);
    }
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pcirc_vector 					    */
/*                                                                          */
/****************************************************************************/

int
_cgi_pcirc_vector(x, y, xe, ye, color, arc_geom)
short           x, y, xe, ye, color;
struct cgi_arc_geom *arc_geom;	/* ptr to geometric info for arc */
{
#define	x1	arc_geom->left
#define	x2	arc_geom->right
#define	y1	arc_geom->top
#define	y2	arc_geom->bottom
#define	sx1	arc_geom->sleft
#define	sx2	arc_geom->sright
#define	sy1	arc_geom->stop
#define	sy2	arc_geom->sbottom

    switch (arc_geom->arc_type)
    {
    case SAME:
	{
	    /* Point is INSIDE arc if within (or ON) rectangle */
	    /* Vector is OUTSIDE if EITHER endpt is OUTSIDE the arc ??? */
	    /* compare (should I do the INSIDE) to (is it inside?) */
	    if ((arc_geom->do_outside == INSIDE) /* want to draw inside */
		==		/* BOTH start and end points are INSIDE (or ON)
				 * rectangle */
		((x1 <= x && x <= x2) && (y1 <= y && y <= y2)
		 && (x1 <= xe && xe <= x2) && (y1 <= ye && ye <= y2)
		 )
		)
		pw_vector(_cgi_vws->sunview.pw, x, y, xe, ye, _cgi_pix_mode, color);
	    break;
	}

    case ADJACENT:
    case OPP_QUAD:		/* OPPOSITE QUADRANTS ONLY */
    case OPP_OCTANT:		/* OPPOSITE OCTANTs */
	{
	    /* Point is INSIDE arc if within (or ON) EITHER rectangle */
	    /* Vector is OUTSIDE only if BOTH Endpoints are OUTSIDE */
	    if ((arc_geom->do_outside == INSIDE) ==
		(
		 (((x1 <= x && x <= x2) && (y1 <= y && y <= y2))
		  || ((sx1 <= x && x <= sx2) && (sy1 <= y && y <= sy2))
		  )		/* start point is INSIDE (or ON) one of the
				 * rectangles */
		 ||
		 (((x1 <= xe && xe <= x2) && (y1 <= ye && ye <= y2))
		  || ((sx1 <= xe && xe <= sx2) && (sy1 <= ye && ye <= sy2))
		  )		/* end  point is INSIDE (or ON) one of the
				 * rectangles */
		 )
		)		/* Boundry of X and Y regions are INSIDE arc */
		pw_vector(_cgi_vws->sunview.pw, x, y, xe, ye, _cgi_pix_mode, color);
	    break;
	}


    case SPECIAL:
	{			/* "Special" opposites: compare each point to 2
				 * rects */
	    /* Point is outside if COMPLETELY within EITHER RECTANGLE */
	    /* Vector is outside if EITHER Endpoint is OUTSIDE */
	    if ((arc_geom->do_outside == OUTSIDE) ==
		((x1 < x && x < x2) && (y1 < y && y < y2)
		 || (sx1 < x && x < sx2) && (sy1 < y && y < sy2)
		 || (x1 < xe && xe < x2) && (y1 < ye && ye < y2)
		 || (sx1 < xe && xe < sx2) && (sy1 < ye && ye < sy2)
		 )
		)		/* Boundry of X and Y regions are INSIDE arc */
		pw_vector(_cgi_vws->sunview.pw, x, y, xe, ye, _cgi_pix_mode, color);
	    break;
	}

    }				/* end switch (arc_geom->arc_type) */
#undef	x1	arc_geom->left
#undef	x2	arc_geom->right
#undef	y1	arc_geom->top
#undef	y2	arc_geom->bottom
#undef	sx1	arc_geom->sleft
#undef	sx2	arc_geom->sright
#undef	sy1	arc_geom->stop
#undef	sy2	arc_geom->sbottom
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_circle	 			    */
/*                                                                          */
/****************************************************************************/
_cgi_fill_partial_circle(c1, c2, c3, rad)
Ccoor          *c1, *c2, *c3;		/* wds 850221: Pointers to center and
					 * contact point */
int             rad;		/* radius */
{
    int             x, y, d, color;
    /* geometric info for a circular or elliptical arc */
    struct cgi_arc_geom arc_info;
    int             srad;
    float           m, b;

    _cgi_devscale(c1->x, c1->y, arc_info.xc, arc_info.yc);
    _cgi_devscale(c2->x, c2->y, arc_info.x2, arc_info.y2);
    _cgi_devscale(c3->x, c3->y, arc_info.x3, arc_info.y3);
    if ((arc_info.x3 - arc_info.x2) != 0)
    {
	m = (arc_info.y3 - arc_info.y2);
	/* don't worry if m=0, drawing scanlines which are horizontal anyway */
	m = m / (arc_info.x3 - arc_info.x2);
	b = arc_info.y3 - m * arc_info.x3;
    }
    else
    {
	m = 0;
	b = arc_info.x3;
    }
    _cgi_devscalen(rad, srad);
    _cgi_setup_arc_geom(&arc_info, srad, srad, 0);
    x = 0;
    y = srad;
    d = 3 - 2 * srad;
    color = _cgi_att->fill.color;
    while (x < y)
    {
	_cgi_arc_fill_pts(&arc_info, color, x, y, m, b);
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
	_cgi_arc_fill_pts(&arc_info, color, x, y, m, b);
}


/****************************************************************************/

_cgi_arc_fill_pts(arc_geom, color, x, y, m, b)
struct cgi_arc_geom *arc_geom;		/* ptr to geometric info for arc */
short           color, x, y;
float           m, b;
{				/* draw symmetry points of circle */
    short           a1, a2, a3, a4;
    a1 = arc_geom->xc + x;
    a2 = arc_geom->xc + y;
    a3 = arc_geom->xc - x;
    a4 = arc_geom->xc - y;

    /* From left to right along a single scanline */
    _cgi_vector_arc_clip(a2, a4, arc_geom->yc + x, color, arc_geom, m, b, 1);
    _cgi_vector_arc_clip(a3, a1, arc_geom->yc + y, color, arc_geom, m, b, 1);
    _cgi_vector_arc_clip(a2, a4, arc_geom->yc - x, color, arc_geom, m, b, 1);
    _cgi_vector_arc_clip(a3, a1, arc_geom->yc - y, color, arc_geom, m, b, 1);
}

/***************************************************************************/

#define SCAN_LINE(flag, xstart, xend, yline)				    \
    if(flag)								    \
	pw_vector(_cgi_vws->sunview.pw, xstart, y, xend, y, _cgi_pix_mode, color);   \
    else								    \
	(void) pr_vector(_cgi_circrect, xstart, y, xend, y, bop, 1);

/* slope == 0 means vertical line, use x_vert */
#define FIND_CROSS(x_to_set, slope, y_intercept, x_vert, scan_line)	\
    if (slope != 0)							\
    {float res;								\
	res = (scan_line - y_intercept) / slope + 0.5;			\
	x_to_set = res;							\
    }									\
    else								\
	x_to_set = x_vert;

#define	SCAN_OUTSIDE(flag,x,xe,y)					\
	if (arc_geom->do_outside)  {SCAN_LINE(flag,x,xe,y)}

#define	SCAN_INSIDE(flag,x,xe,y)					\
	if (!arc_geom->do_outside) {SCAN_LINE(flag,x,xe,y)}

/***************************************************************************/

_cgi_vector_arc_clip(x, xe, y, color, arc_geom, m, b, flag)
short           x, xe, y, color;
struct cgi_arc_geom *arc_geom;	/* ptr to geometric info for arc */
float           m, b;
short           flag;		/* TRUE if should use pw_vector (to screen),
				 * else use pr_vector */
{
    int             bop;
    char            x_within, xe_within, y_within;	/* value is within
							 * region */
    char            start_outside, end_outside;	/* POINT IS OUTSIDE ARC */

    switch (arc_geom->arc_type)
    {
    case SAME:
	{			/* same quadrant: is point within single
				 * rectangle? */
	    /* Boundry of X & Y regions are WITHOUT region (OUTSIDE ARC) */
	    x_within = (arc_geom->left < x) && (x < arc_geom->right);
	    xe_within = (arc_geom->left < xe) && (xe < arc_geom->right);
	    y_within = (arc_geom->top < y) && (y < arc_geom->bottom);
	    start_outside = (!x_within) || (!y_within);
	    end_outside = (!xe_within) || (!y_within);
	    break;
	}

    case ADJACENT:
    case OPP_QUAD:		/* OPPOSITE QUADRANTS ONLY */
    case OPP_OCTANT:		/* OPPOSITE OCTANTs */
	{
	    /* Adjacent quadrants: is point within EITHER rectangle */
	    /* Point is INSIDE ARC if WITHIN EITHER RECTANGLE */
	    x_within = (arc_geom->left <= x) && (x <= arc_geom->right);
	    xe_within = (arc_geom->left <= xe) && (xe <= arc_geom->right);
	    y_within = (arc_geom->top <= y) && (y <= arc_geom->bottom);
	    start_outside = (!x_within) || (!y_within);
	    end_outside = (!xe_within) || (!y_within);
	    if (start_outside)	/* Not in one rectangle: try other */
	    {
		x_within = (arc_geom->sleft <= x) && (x <= arc_geom->sright);
		y_within = (arc_geom->stop <= y) && (y <= arc_geom->sbottom);
		start_outside = (!x_within) || (!y_within);
	    }
	    if (end_outside)
	    {			/* Point not in one rectangle.  Try other */
		xe_within = (arc_geom->sleft <= xe) && (xe <= arc_geom->sright);
		y_within = (arc_geom->stop <= y) && (y <= arc_geom->sbottom);
		end_outside = (!xe_within) || (!y_within);
	    }
	    break;
	}

    case SPECIAL:
	{			/* "Special" opposites: compare each point to 2
				 * rectangles */
	    /* Boundry of X and Y regions are WITHOUT (i.e., OUTSIDE) region */
	    x_within = (arc_geom->left < x) && (x < arc_geom->right);
	    xe_within = (arc_geom->left < xe) && (xe < arc_geom->right);
	    y_within = (arc_geom->top < y) && (y < arc_geom->bottom);
	    /* Point is OUTSIDE ARC if WITHIN EITHER RECTANGLE */
	    start_outside = (x_within) && (y_within);
	    end_outside = (xe_within) && (y_within);
/*dbx	p	arc_geom->do_outside, start_outside, end_outside	*/
	    if (!start_outside)
	    {			/* Point not in one rectangle.  Try other */
		x_within = (arc_geom->sleft < x) && (x < arc_geom->sright);
		y_within = (arc_geom->stop < y) && (y < arc_geom->sbottom);
		start_outside = (x_within) && (y_within);
	    }
	    if (!end_outside)
	    {			/* Point not in one rectangle.  Try other */
		xe_within = (arc_geom->sleft < xe) && (xe < arc_geom->sright);
		y_within = (arc_geom->stop < y) && (y < arc_geom->sbottom);
		end_outside = (xe_within) && (y_within);
	    }
	    break;
	}

    }				/* end switch (arc_geom->arc_type) */
/*dbx	p	arc_geom->do_outside, start_outside, end_outside	*/

    if (start_outside == end_outside)
    {				/* both ends INSIDE or OUT?  Drawing INSIDE or
				 * OUT? */
	if ((arc_geom->do_outside == OUTSIDE) != start_outside)
	    return;
    }				/* Didn't return: line should be drawn
				 * completely */
    else
    {				/* line straddles boundry */
	if ((arc_geom->do_outside == OUTSIDE) != start_outside)
	{
	    FIND_CROSS(x, m, b, b, y);
	}			/* start point needs clipping */
	else
	{
	    FIND_CROSS(xe, m, b, b, y);
	}			/* start point OK: clip end */
    }
    if (!flag)
	bop = PIX_COLOR(255) | PIX_SRC;
    SCAN_LINE(flag, x, xe, y);
}

#undef	SCAN_LINE(flag, xstart, xend, yline)
#undef	FIND_CROSS(x_to_set, slope, y_intercept, x_vert, scan_line)
#undef	SCAN_OUTSIDE(flag,x,xe,y)
#undef	SCAN_INSIDE(flag,x,xe,y)


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_setup_pie_geom	 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

_cgi_setup_pie_geom(pie_ptr, yrad)
struct cgi_pie_clip_geom *pie_ptr;	/* ptr to geometric info for arc */
/* xc,yc, x2,y2, x3,y3 fields must be prepared already */
int             yrad;
{
    short           x1, y1, x2, y2, x3, y3;
    x1 = pie_ptr->xc, y1 = pie_ptr->yc;
    x2 = pie_ptr->x2, y2 = pie_ptr->y2;
    x3 = pie_ptr->x3, y3 = pie_ptr->y3;
    pie_ptr->do_outside = INSIDE;

    /* Set up min & max for each line */
    /* Determine "polarity" of line & area */
    if (y2 == y1)		/* Line 2 is horizontal: reverse min & max */
    {
	pie_ptr->y2min = (y1 + yrad + 1);	/* Will not be < y3min */
	pie_ptr->y2max = (y1 - yrad - 1);	/* Will not be > y3min */
    }
    else
	if (y2 < y1)
	{
	    pie_ptr->y2min = (y1 - yrad - 1);
	    pie_ptr->y2max = y1;
	    pie_ptr->left2 = INSIDE;
	}
	else			/* (y2 > y1) */
	{
	    pie_ptr->y2min = y1;
	    pie_ptr->y2max = (y1 + yrad + 1);
	    pie_ptr->left2 = OUTSIDE;
	}

    if (y3 == y1)		/* Line 3 is horizontal: reverse min & max */
    {
	pie_ptr->y3min = (y1 + yrad + 1);	/* Will not be < y2min */
	pie_ptr->y3max = (y1 - yrad - 1);	/* Will not be > y2min */
    }
    else
    {
	if (y3 < y1)
	{
	    pie_ptr->y3min = (y1 - yrad - 1);
	    pie_ptr->y3max = y1;
	    pie_ptr->left3 = OUTSIDE;
	}
	else			/* (y3 > y1) */
	{
	    pie_ptr->y3min = y1;
	    pie_ptr->y3max = (y1 + yrad + 1);
	    pie_ptr->left3 = INSIDE;
	}
    }

    /* For the most trivial scanline reject, set up absolue min & max */
    if ((y3 == y1) && (y2 == y1))
    {				/* Kluge to do top or bottom circle halfs */
	pie_ptr->do_outside = pie_ptr->trivial_half = OUTSIDE;
	if (x2 <= x3)
	{			/* Bottom half will be trivially drawn. */
	    pie_ptr->ymin = (y1 - yrad - 1);
	    pie_ptr->ymax = y1;
	}
	else			/* x2 > x3: Top half will be trivially drawn. */
	{
	    pie_ptr->ymin = y1;
	    pie_ptr->ymax = (y1 + yrad + 1);
	}
    }
    else
    {
	if (pie_ptr->y2min <= pie_ptr->y3min)
	    pie_ptr->ymin = pie_ptr->y2min;
	else
	    pie_ptr->ymin = pie_ptr->y3min;

	if (pie_ptr->y2max >= pie_ptr->y3max)
	    pie_ptr->ymax = pie_ptr->y2max;
	else
	    pie_ptr->ymax = pie_ptr->y3max;

	if ((y2 >= y1) && (y3 >= y1))
	    pie_ptr->trivial_half = (x2 < x3) ? OUTSIDE : INSIDE;
	/* The else looks funny, but is necessary, because of = cases */
	else
	    if ((y2 <= y1) && (y3 <= y1))
		pie_ptr->trivial_half = (x2 < x3) ? INSIDE : OUTSIDE;
    }

    pie_ptr->yoverlap = ((pie_ptr->y3min == pie_ptr->y2min)
			 && (pie_ptr->y3max == pie_ptr->y2max));
    /* if yoverlap == FALSE, there is no trivial half */

    /* For each line, determine slope and y-intercept */
    if ((x2 - x1) != 0)
    {
	pie_ptr->m2 = (y2 - y1);
	pie_ptr->m2 = pie_ptr->m2 / (x2 - x1);
	pie_ptr->b2 = y2 - pie_ptr->m2 * x2;
    }
    else			/* Line is vertical: Flag with "m" == 0, Stuff
				 * X-intercept into "b" */
    {
	pie_ptr->m2 = 0;
	pie_ptr->b2 = x2;
    }

    if ((x3 - x1) != 0)
    {
	pie_ptr->m3 = (y3 - y1);
	pie_ptr->m3 = pie_ptr->m3 / (x3 - x1);
	pie_ptr->b3 = y3 - pie_ptr->m3 * x3;
    }
    else			/* Line is vertical: Flag with "m" == 0, Stuff
				 * X-intercept into "b" */
    {
	pie_ptr->m3 = 0;
	pie_ptr->b3 = x3;
    }
/* dbx:		p	*pie_ptr	*/
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_partial_pie_circle 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

_cgi_fill_partial_pie_circle(c1, c2, c3, rad)
Ccoor          *c1, *c2, *c3;		/* wds 850221: Pointers to center and
					 * contact point */
int             rad;		/* radius */
{
    int             x, y, d, color;
    int             x1, y1;
    int             x2, y2;
    int             x3, y3;
    struct cgi_pie_clip_geom pie_info;
    int             srad;

    _cgi_devscale(c1->x, c1->y, x1, y1);
    _cgi_devscale(c2->x, c2->y, x2, y2);
    _cgi_devscale(c3->x, c3->y, x3, y3);
    _cgi_devscalen(rad, srad);
    /* _cgi_pcirc_box (x1, y1, x2, y2, x3, y3, srad, &x5, &y5, &x6, &y6, 0); */

    pie_info.xc = x1, pie_info.yc = y1;
    pie_info.x2 = x2, pie_info.y2 = y2;
    pie_info.x3 = x3, pie_info.y3 = y3;
    _cgi_setup_pie_geom(&pie_info, srad);

    x = 0;
    y = srad;
    d = 3 - 2 * srad;
    color = _cgi_att->fill.color;
    while (x < y)
    {
	_cgi_partial_pie_circ_fill_pts(x1, y1, &pie_info, color, x, y);
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
	_cgi_partial_pie_circ_fill_pts(x1, y1, &pie_info, color, x, y);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_pie_circ_fill_pts 			    */
/*                                                                          */
/*			_cgi_partial_pie_circ_fill_pts			    */
/****************************************************************************/

_cgi_partial_pie_circ_fill_pts(x0, y0, pie_ptr, color, x, y)
short           x0, y0, color, x, y;
struct cgi_pie_clip_geom *pie_ptr;
{				/* draw symmetry points of circle */
    short           a1, a2, a3, a4, b1b, b2, b3, b4;
    a1 = x0 + x;
    a2 = x0 + y;
    a3 = x0 - x;
    a4 = x0 - y;
    b1b = y0 + x;
    b2 = y0 + y;
    b3 = y0 - x;
    b4 = y0 - y;
    _cgi_vector_pcirc_pie_clip(a4, a2, b1b, color, pie_ptr, 1);
    _cgi_vector_pcirc_pie_clip(a3, a1, b2, color, pie_ptr, 1);
    _cgi_vector_pcirc_pie_clip(a4, a2, b3, color, pie_ptr, 1);
    _cgi_vector_pcirc_pie_clip(a3, a1, b4, color, pie_ptr, 1);
}


#define SCAN_LINE(flag, xstart, xend, yline)				    \
    if(flag)								    \
	pw_vector(_cgi_vws->sunview.pw, xstart, y, xend, y, _cgi_pix_mode, color);   \
    else								    \
	(void) pr_vector(_cgi_circrect, xstart, y, xend, y, bop, 1);

/* slope == 0 means vertical line, use x_vert */
#define FIND_CROSS(x_to_set, slope, y_intercept, x_vert, scan_line)	\
    if (slope != 0)							\
    {float res;								\
	res = (scan_line - y_intercept) / slope + 0.5;			\
	x_to_set = res;							\
    }									\
    else								\
	x_to_set = x_vert;

#define	SCAN_OUTSIDE(flag,x,xe,y)					\
	if (pie_ptr->do_outside)  {SCAN_LINE(flag,x,xe,y)}
#define	SCAN_INSIDE(flag,x,xe,y)					\
	if (!pie_ptr->do_outside) {SCAN_LINE(flag,x,xe,y)}
#define	SCAN_IF_OUTSIDE(out,flag,x,xe,y)				\
	if ((out) == pie_ptr->do_outside)  {SCAN_LINE(flag,x,xe,y)}
#define	SCAN_IF_INSIDE(out,flag,x,xe,y)					\
	if ((out) != pie_ptr->do_outside)  {SCAN_LINE(flag,x,xe,y)}

#define	SCAN_CROSS(x, xcross, xe, y, left_of_line)			\
	    if ((left_of_line) == (pie_ptr->do_outside))		\
		{SCAN_LINE(flag, x, xcross, y)}				\
	    else							\
		{SCAN_LINE(flag, xcross, xe, y)}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_vector_pcirc_pie_clip		 		    */
/*                                                                          */
/*	Clip a scanline segment from x to xe along y, using pie structure.  */
/****************************************************************************/
int
_cgi_vector_pcirc_pie_clip(x, xe, y, color, pie_ptr, flag)
short           x, xe, y, color;
struct cgi_pie_clip_geom *pie_ptr;
short           flag;
{
    int             bop;
    short           inrange2, inrange3, straddle2, straddle3, xc2, xc3;
    /* inrange means that the ymin & ymax include scanline y */
    /* straddle means that the the line crosses scanline y between x & xe */
    /* (A line cannot be straddled if it is not "in y range".) */
    /* xc2 will be x where y crosses line 2 (use m2 & b2); similar for xc3. */

    if (!flag)
	bop = PIX_COLOR(255) | PIX_SRC;

    if ((y < pie_ptr->ymin) || (y > pie_ptr->ymax))
    {				/* Trivial half (top or bottom) of circle */
	{
	    SCAN_IF_OUTSIDE(pie_ptr->trivial_half, flag, x, xe, y)
	}
    }
    else
    {				/* non-trivial */
	/* We do include y==MIN or y==MAX in "range" */
	inrange2 = (pie_ptr->y2min <= y) && (y <= pie_ptr->y2max);
	/* New semantics: inrange2 means line 2 is actually straddled */
	if (inrange2)
	{			/* scan line is within range for line 2 */
	    FIND_CROSS(xc2, pie_ptr->m2, pie_ptr->b2, pie_ptr->b2, y);
	    straddle2 = ((x < xc2) && (xc2 < xe));
	}
	else			/* (A line cannot be straddled if it is not "in
				 * y range".) */
	    straddle2 = 0;

	inrange3 = (pie_ptr->y3min <= y) && (y <= pie_ptr->y3max);
	if (inrange3)
	{			/* scan line is within range for line 3 */
	    FIND_CROSS(xc3, pie_ptr->m3, pie_ptr->b3, pie_ptr->b3, y);
	    straddle3 = ((x < xc3) && (xc3 < xe));
	}
	else			/* (A line cannot be straddled if it is not "in
				 * y range".) */
	    straddle3 = 0;
/*dbx:	    p	inrange2, inrange3	*/

	if (straddle2 && !straddle3)
	    /* only line 2 is straddled */
	{
	    SCAN_CROSS(x, xc2, xe, y, pie_ptr->left2)
	}
	else
	    if (!straddle2 && straddle3)
		/* only line 3 is straddled */
	    {
		SCAN_CROSS(x, xc3, xe, y, pie_ptr->left3)
	    }
	    else
		if (straddle2 && straddle3)
		{		/* both lines appear to be straddled */
		    if (xc2 == xc3)
		    {		/* really only one cross at center */
			if (pie_ptr->yoverlap)	/* there is a trivial half */
			{	/* Angle of V: flat center point? */
			     /* dbx */ if ((pie_ptr->do_outside)
					   == (pie_ptr->trivial_half)
				)
			    {
				SCAN_LINE(flag, x, xe, y)
			    }
			    else
			    {
				SCAN_LINE(flag, xc2, xc3, y)
			    }
			}
			else
			    /* Not overlapping: center of > or < shape */
			    /* treat as if only line2 is straddled */
			{
			    SCAN_CROSS(x, xc2, xe, y, pie_ptr->left2)
			}
		    }
		    else
		    {		/* both lines are really straddled */
			if (xc2 < xc3)
			    if (pie_ptr->left2 == pie_ptr->do_outside)
			    {
				SCAN_LINE(flag, x, xc2, y);
				SCAN_LINE(flag, xc3, xe, y);
			    }
			    else
			    {
				SCAN_LINE(flag, xc2, xc3, y)
			    }
			else	/* xc2 > xc3 */
			    if (pie_ptr->left3 == pie_ptr->do_outside)
			    {
				SCAN_LINE(flag, x, xc3, y);
				SCAN_LINE(flag, xc2, xe, y);
			    }
			    else
			    {
				SCAN_LINE(flag, xc3, xc2, y)
			    }
		    }		/* both lines are really straddled */
		}		/* both lines appear to be straddled */
		else		/* !straddle2 && !straddle3 */
		{		/* neither line is straddled */
		    if ((inrange2) && (!inrange3))
		    {		/* Only line 2 is in range */
			if (xe <= xc2)	/* to the Left of the line */
			{
			    SCAN_IF_OUTSIDE(pie_ptr->left2, flag, x, xe, y)
			}
			else	/* (x > xc2) to the Right of the line */
			{
			    SCAN_IF_INSIDE(pie_ptr->left2, flag, x, xe, y)
			}
		    }
		    else
			if ((!inrange2) && (inrange3))
			{	/* Only line 3 is in range */
			    if (xe <= xc3)	/* to the Left of the line */
			    {
				SCAN_IF_OUTSIDE(pie_ptr->left3, flag, x, xe, y)
			    }
			    else/* (x > xc3) to the Right of the line */
			    {
				SCAN_IF_INSIDE(pie_ptr->left3, flag, x, xe, y)
			    }
			}

			else
			    if ((!inrange2) && (!inrange3))
			    {	/* code for top or bottom circle halfs */
				 /* dbx:watch */ if (y == pie_ptr->yc)
				{	/* at center of circle (with y-overlap) */
				    /* Might give flat center point */
/*dbx* p y, pie_ptr->y2min, pie_ptr->y3min, pie_ptr->yc	*/
				    {
					SCAN_INSIDE(flag, pie_ptr->xc, pie_ptr->xc, y)
				    }
				    {
					SCAN_OUTSIDE(flag, x, xe, y)
				    }
				}
				else
				    /* the rest: at top or bottom of circle
				     * half */
				{
				    SCAN_INSIDE(flag, pie_ptr->xc, pie_ptr->xc, y)
				}
				return;
			    }	/* at top or bottom or center of circle */


			    else
				if ((inrange2) && (inrange3))
				{	/* Both lines in range -- but neither
					 * is straddled */
				    if ((xe <= xc2) && (xe <= xc3))
				    {	/* to the Left of both lines */
					SCAN_IF_OUTSIDE(
							(xc2 < xc3) ? pie_ptr->left2 : pie_ptr->left3,
							flag, x, xe, y)
				    }
				    else
					if ((x >= xc2) && (x >= xc3))
					{	/* to the Right of both lines */
					    SCAN_IF_INSIDE(
							   (xc2 > xc3) ? pie_ptr->left2 : pie_ptr->left3,
							   flag, x, xe, y)
					}
					else
					{
					    SCAN_IF_OUTSIDE(
							    (xe <= xc2) ? pie_ptr->left2 : pie_ptr->left3,
							    flag, x, xe, y)
					}
				}	/* Both lines in range */
		}		/* neither line is straddled */
    }				/* non-trivial */
}

#undef	SCAN_OUTSIDE(flag,x,xe,y)
#undef	SCAN_INSIDE(flag,x,xe,y)
#undef	SCAN_IF_OUTSIDE(out,flag,x,xe,y)
#undef	SCAN_IF_INSIDE(out,flag,x,xe,y)					\
#undef	SCAN_CROSS(x, xcross, xe, y, left_of_line)
