#ifndef lint
static char	sccsid[] = "@(#)polysubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
bugs:
line endstyle
more vdipw atts
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */

static short    _cgi_tex_dotted[] = {1, 5, 1, 5, 1, 5, 1, 5, 0};
static short    _cgi_tex_dashed[] = {7, 7, 7, 7, 0};
static short    _cgi_tex_dashdot[] = {7, 3, 1, 3, 7, 3, 1, 3, 0};
static short    _cgi_tex_dashdotdotted[] = {9, 3, 1, 3, 1, 3, 1, 3, 0};
static short    _cgi_tex_longdashed[] = {13, 7, 0};

Pr_texture     *_cgi_line_setup(texP, brushP, npts)
Pr_texture     *texP;
Pr_brush       *brushP;
int             npts;
{
    brushP->width = _cgi_vws->conv.line_width;

    switch (_cgi_att->line.style)
    {
    case SOLID:
	return ((Pr_texture *) 0);
    case DOTTED:
	texP->pattern = _cgi_tex_dotted;
	break;
    case DASHED:
	texP->pattern = _cgi_tex_dashed;
	break;
    case DASHED_DOTTED:
	texP->pattern = _cgi_tex_dashdot;
	break;
    case DASH_DOT_DOTTED:
	texP->pattern = _cgi_tex_dashdotdotted;
	break;
    case LONG_DASHED:
	texP->pattern = _cgi_tex_longdashed;
	break;
    }
    texP->offset = 0;
    texP->options.startpoint = 0;
    texP->options.endpoint = 0;
    texP->options.balanced = 0;
    texP->options.givenpattern = 0;
    texP->options.res_fat = 0;
    texP->options.res_poly = 0;
    texP->options.res_close = 0;
    texP->options.res_mvlist = 0;

    switch (_cgi_att->endstyle)
    {
    case NATURAL:
	break;
    case BEST_FIT:
	/*
	 * Balancing is not currently done over the whole polyline. This
	 * produces ugly results, so drop through to POINT mode if so. 
	 */
	if (npts <= 2)
	    texP->options.balanced = 1;
    case POINT:
	texP->options.startpoint = 1;
	texP->options.endpoint = 1;
	break;
    }
    return (texP);
}
