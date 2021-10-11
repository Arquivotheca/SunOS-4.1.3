#ifndef lint
static char	sccsid[] = "@(#)polygonsubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI polygon functions
 */

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
Pixrect        *_cgi_pattern();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_cpw_poly2						    */
/*                                                                          */
/*		draws filled polygon with screen coords			    */
/****************************************************************************/
/* ARGSUSED WCL: npts unused */
_cgi_cpw_poly2(vlist, npts, patindex, flag, bounds, npt, patflag)
Ccoor          *vlist;		/* vertices themselves (screen coords) */
short           npts;		/* number of vertices total */
/* ## warning: argument npts unused in function _cgi_cpw_poly2 */
int             patindex;
int             flag;
int             bounds;		/* number of boundries */
int            *npt;		/* array: number of vertices in boundry i */
short           patflag;
{
    int             fillop;
    Pixrect        *patrect;

    fillop = PIX_COLOR(_cgi_att->fill.color) | _cgi_pix_mode;
    if (patflag)
    {
	patrect = _cgi_pattern(patindex, flag);
    }
    else
	patrect = (Pixrect *) 0;;
    pw_polygon_2(_cgi_vws->sunview.pw, 0, 0, bounds, npt,
		 (struct pr_pos *) vlist, fillop,
		 patrect, 0, 0);
    if (patrect)
	(void) pr_destroy(patrect);
}
