#ifndef lint
static char	sccsid[] = "@(#)polygon.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
polygon
cgipw_polygon
_cgi_draw_global_polygon
partial_polygon
*/

#include "cgipriv.h"

Gstate          _cgi_state;	/* CGI global state */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */

int             _cgi_polybase;	/* number of elements in global polygon list */
Ccoor           _cgi_global_polylist[MAXPTS];	/* polygon list */
short           _cgi_closed_points[MAXPTS];	/* Flags for global polygon
						 * list (see below) */
#define	POLY_NORMAL	0	/* Normal drawn polygon edge */
#define	POLYLINE_END	1	/* Continue subpolygon, but do not draw edge */
#define	POLYGON_END	2	/* End of subpolygon (end of bound) */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polygon	 					    */
/*                                                                          */
/*		draws non-closed polygon based on global polygon list	    */
/****************************************************************************/

Cerror          polygon(polycoors)
Ccoorlist      *polycoors;	/* list of points */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
	if (polycoors->n > MAXPTS)
	    err = ENMPTSTL;
    if (!err)
    {
	if (!_cgi_state.cgipw_mode)
	{			/* not in cgipw mode */
	    (void) partial_polygon(polycoors, CLOSE);
	    /* The last subpolygon must be closed. */
	    _cgi_closed_points[_cgi_polybase - 1] |= POLYGON_END;
	}
	else
	    err = ENOTCSTD;	/* cgi in cgipw mode */
    }
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    /* _cgi_bump_vws sets up _cgi_vws and _cgi_att */
	    err = _cgi_draw_global_polygon();
	}
	_cgi_polybase = 0;	/* Clear global polygon list */
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_polygon 						    */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_polygon(desc, polycoors)
Ccgiwin        *desc;
Ccoorlist      *polycoors;	/* list of points */
{
    int             err = NO_ERROR;

    if (_cgi_state.cgipw_mode)
    {				/* in cgipw mode */
	SETUP_CGIWIN(desc);
	(void) partial_polygon(polycoors, CLOSE);
	/* The last subpolygon must be closed. */
	_cgi_closed_points[_cgi_polybase - 1] |= POLYGON_END;
	err = _cgi_draw_global_polygon();
	_cgi_polybase = 0;	/* Clear global polygon list */
    }
    else
	err = ENOTCSTD;		/* not compatible with standard CGI mode */
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_draw_global_polygon 				    */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_draw_global_polygon()
/* All the points are already on the global polygon list */
{
    Cerror          err = NO_ERROR;
    Cerror          terr;
    Ccoor           devlist[MAXPTS];	/* device coordinate polygon list */
    register Ccoor *gpt, *dpt;	/* Pointers for global and device lists */
    register int    i, j, k;	/* counters */
    int             np, npl[MAXPTS];
    Clinatt         templin;	/* Holds line attributes during perimeter */
    int             itemp;	/* converted line width during perimeter */
    register u_char *mvP;
    register Ccoor *coorP;
    u_char          mvlist[MAXPTS];	/* for disjoint perimeter drawing */
    Ccoor           coorlist[MAXPTS];

    /* _cgi_vws->sunview.pw and _cgi_att already set up */

    for (gpt = &_cgi_global_polylist[0], dpt = &devlist[0];
	 (dpt < &devlist[_cgi_polybase]);
	 gpt++, dpt++)
    {
	_cgi_devscale(gpt->x, gpt->y, dpt->x, dpt->y);
    }

    if (_cgi_polybase >= 3)
    {
	pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);

	/* set up number of points in each bound, number of bounds */
	for (i = 0, j = 0, np = 0; i < _cgi_polybase; i++)
	{
	    if ((_cgi_closed_points[i] & POLYGON_END) == 0)
	    {
		j++;
	    }
	    else
	    {
		npl[np++] = j + 1;
		j = 0;
	    }
	}

	/* fill */
	switch (_cgi_att->fill.style)
	{
	case SOLIDI:
	    {
		_cgi_cpw_poly2(devlist, _cgi_polybase, 0, 0, np, npl, 0);
		break;
	    }
	case HOLLOW:
	    {
		break;
	    }
	case PATTERN:
	    {
		_cgi_cpw_poly2(devlist, _cgi_polybase,
			       _cgi_att->fill.pattern_index, 1, np, npl, 1);
		break;
	    }
	case HATCH:
	    {
		_cgi_cpw_poly2(devlist, _cgi_polybase,
			       _cgi_att->fill.hatch_index, 0, np, npl, 1);
		break;
	    }
	}

	/* draw perimeter? */
	if (_cgi_att->fill.visible == ON)
	{
	    /* save line attributes while drawing with perimeter attributes */
	    templin = _cgi_att->line;
	    _cgi_att->line.style = _cgi_att->fill.pstyle;
	    _cgi_att->line.width = _cgi_att->fill.pwidth;
	    itemp = _cgi_vws->conv.line_width;
	    _cgi_vws->conv.line_width = _cgi_vws->conv.perimeter_width;
	    _cgi_att->line.color = _cgi_att->fill.pcolor;
	    mvP = mvlist;
	    *mvP++ = 0;		/* don't close individual sub-polygons */
	    coorP = coorlist;

	    /*
	     * i counts thru list.  j counts up polyline.  k stays at start of
	     * sub-polygon.
	     */
	    for (i = j = k = 0; i < _cgi_polybase; i++)
	    {
		switch (_cgi_closed_points[i])
		{
		case POLY_NORMAL:
		    *mvP++ = 0;
		    *coorP++ = devlist[i];
		    break;
		case POLYLINE_END:
		    /*
		     * Reached end of polygon or polyline (i.e., last drawn
		     * edge)
		     */
		    *mvP++ = 1;	/* don't draw to NEXT edge */
		    *coorP++ = devlist[i];
		    break;
		case POLYGON_END:
		    /*
		     * subpolygon end: close back to first point in boundry
		     */
		    *coorP++ = devlist[i];
		    *mvP = 0;
		    *coorP = devlist[k];
		    terr = _cgi_polyline(_cgi_vws, i - k + 2, coorlist, mvlist,
					 DONT_XFORM);
		    if (!err)
			err = terr;
		    mvP = &mvlist[1];
		    coorP = coorlist;
		    k = i + 1;	/* start of next subpolygon */
		    break;
		}
	    }
	    _cgi_att->line = templin;
	    _cgi_vws->conv.line_width = itemp;
	}			/* end (_cgi_att->fill.visible == ON) */
	pw_unlock(_cgi_vws->sunview.pw);	/* unlock the window */
    }
    else
	err = EPLMTHPT;		/* 1984 standard says 0 points is a no-op, 1, a
				 * dot; 2, line */
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: partial_polygon	 				    */
/*                                                                          */
/*		adds coordinates to global polyline list		    */
/****************************************************************************/

Cerror          partial_polygon(polycoors, cflag)
Ccoorlist      *polycoors;	/* list of points */
Ccflag          cflag;		/* close previous subpolygon (or add nondrawn) */
{
    Cerror          err;
    register Ccoor *point;
    register int    i;

    err = _cgi_err_check_4();
    if (!err)
	if (polycoors->n > MAXPTS)
	    err = ENMPTSTL;
    if (!err)
    {
	if (_cgi_polybase > 0)
	    _cgi_closed_points[_cgi_polybase - 1] |=
		(cflag == CLOSE) ? POLYGON_END : POLYLINE_END;

	/* Add the points from this call to the global polygon list */
	for (point = polycoors->ptlist, i = 0; (i < polycoors->n); i++)
	{
	    _cgi_closed_points[_cgi_polybase] = POLY_NORMAL;
	    _cgi_global_polylist[_cgi_polybase++] = *point++;
	    if (_cgi_polybase > MAXPTS)
	    {			/* polygon list is full */
		err = EGPLISFL;
		break;
	    }
	}
    }
    return (_cgi_errhand(err));
}
