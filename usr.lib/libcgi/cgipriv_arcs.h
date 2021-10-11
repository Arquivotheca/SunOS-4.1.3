/*	@(#)cgipriv_arcs.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

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
 * CGI ARC definitions
 */

typedef enum
{
    INSIDE = 0, OUTSIDE = 1,	/* code assumes this "polarity": compares to
				 * booleans */
}               outside_t;

typedef enum
{
    UNKNOWN, SAME, ADJACENT, OPP_QUAD, OPP_OCTANT, SPECIAL
    /*
     * The two arc "endpoints" are (relative to the circle's "center"): in
     * quadrants which are not yet determined, in the SAME quadrant, in
     * adjacent quadrants (both points left, right, above, or below center), in
     * opposite quadrants (one point left, one right. One above, other below)
     * (In opposite quadrants, either use two rects, or one big, SIMPLE, rect.)
     * or a SPECIAL case (due to strange quantization problems?) 
     */
#define OPPOSITE	OPP_OCTANT	/* Directly opposite OCTANTS */
}               arc_quad_t;

struct cgi_arc_geom
{				/* geometric info for a circular or elliptical
				 * arc */
    short           xc, yc;	/* center of circle */
    short           x2, y2, /* one arc point */ x3, y3;	/* other */
    short           left, right, top, bottom;
    short           sleft, sright, stop, sbottom;
    outside_t       do_outside;
    arc_quad_t      arc_type;
};

struct cgi_pie_clip_geom
{				/* geometric info for pie-closed filled arc */
    short           xc, yc;	/* center of circle */
    short           x2, y2, /* one arc point */ x3, y3;	/* other */
    float           m2, b2, m3, b3;	/* slopes & intercepts thru xc,yc */
    short           y2min, y2max, y3min, y3max;
    short           ymin, ymax;
    outside_t       do_outside, left2, left3, trivial_half;
    short           yoverlap;	/* trivial_half exists only if yoverlap */
};
