#ifndef	lint
static char	sccsid[] = "@(#)charsubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Text functions
 */

/*
_cgi_inner_text
_cgi_get_glyph_polyline
_cgi_get_glyph
_cgi_free_text_cache
invalidate_old_cache_entry
_cgi_inq_font
_cgi_enter_font_info
cc_to_vdc
fcc_to_vdc
_cgi_rnd_flt_to_int
 */

#include "cgipriv.h"
#include <math.h>

#ifdef	TEXT_CACHE
int             _cgi_notextcache = 1;
#endif	TEXT_CACHE

#define SCALE(n) ((n) << CC_VDC_SHIFT)
#define UNSCALE(n) n = (n < 0 ?  \
	-(-(n) >> CC_VDC_SHIFT) : \
	(n) >> CC_VDC_SHIFT)

/* External variables and functions */
Gstate          _cgi_state;	/* CGI's global state */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
int             _cgi_ttext;
Ccoor           _cgi_00;	/* coordinate pair (0,0) */

extern char    *strcat(), *strcpy();	/* makes lint happy */

#define MAX_CHAR_PER_FONT	160
#define	MIN(a,b)	(((a)<(b))? (a):(b))
#define	MAX(a,b)	(((a)>(b))? (a):(b))

typedef struct
{
    long            gc_vdc_change_cnt;	/* matches vdc_change_cnt */
    long            gc_text_change_cnt;	/* matches text_change_cnt */
    short           gc_font;	/* font this glyph is cached from */
    unsigned        gc_npts;	/* length of gc_coors and gc_mvlist */
    Ccoor          *gc_coors;	/* polyline that draws this glyph */
    u_char         *gc_mvlist;	/* move list for gc_coors */
    Ccoor           gc_ll, gc_ur;	/* pixel bounding box, for clipping */
    Ccoor           gc_left;	/* left edge/baseline of glyph box */
    Ccoor           gc_right;	/* right edge/baseline of glyph box */
}               Cglyphcache;

typedef struct
{
    unsigned        gi_npts;
    short           gi_xmin;
    short           gi_xmax;
    short           gi_ymin;
    short           gi_ymax;
}               Cglyphinfo;

typedef struct
{
    Cglyphcache    *g_cache;	/* pointer to cached glyph information */
    Ccoor           g_ref;	/* reference position this glyph was set at */
    Ccoor           g_off;	/* pixel position to draw this glyph */
    Ccoor           g_left_box;	/* left side of bounding box when set */
    Ccoor           g_right_box;/* right side of bounding box when set */
}               Cglyph;

static Cfontinfo _cgi_fontinfo[SYMBOLS + 1];
static Cglyphcache _cgi_glyph_cache[MAX_CHAR_PER_FONT];
Cfontinfo       _cgi_inq_font();
static Ccoor    cc_to_vdc(), fcc_to_vdc(), cc_to_vdc_unscaled();
static Ccoor    fcc_to_vdc_unscaled ();


/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_inner_text 						    */
/*									    */
/* This function renders text on a view surface. It	                    */
/* is the most important text routine. The first	                    */
/* decision made is about character precision. If the                       */
/* character precision is STRING, the pixrect (pw)                          */
/* text is used, otherwise the text is drawn with vectors.                  */
/* Both vector and pixrect text can be aligned.  Pixrect text		    */
/* path is restricted to RIGHT and LEFT.				    */
/* Vector text can be skewed. The first process				    */
/* attempts to take care of alignment. Skewing has already		    */
/* been done by the vector transform. When the text is actually being	    */
/* rendered, the path is taken into consideration.                          */
/*									    */
/****************************************************************************/
Cerror          _cgi_inner_text(vws, origin, tstring, extent)
View_surface   *vws;		/* view surface to do text upon */
Ccoor          *origin;		/* VDC coordinates */
char           *tstring;	/* text to draw */
Ccoor          *extent;		/* if non-null, return extent box, don;t draw */
{
    register Cglyph *glyphP;
    register Cglyph *newglyphP;
    register Outatt *attP = vws->att;
    register View_surface *vwsP = vws;
    register short  cnt;
    int             nglyph;
    Cerror          err = NO_ERROR;
    Cerror          terr;
    int             txtop;
    Cint            fixed_font;
    Cpathtype       text_path;
    int             vertical = 0;
    Cglyph          glyph_list[MAXCHAR];
    Chaligntype     halign;
    Cvaligntype     valign;
    struct pr_subregion bound;
    Ccoor           ref;	/* reference for current character */
    Ccoor           off;	/* offset for char box, and glyphs in it */
    Ccoor           concat;	/* relative new concatentation point */
    Ccoor           lleft, uleft, uright;	/* text extent box */
    Cfontinfo       font_info;	/* basic metrics of current font */
    Cfloat          continuous;	/* temp for continuous alignment value */
	int				loop;

    txtop = _cgi_pix_mode | PIX_COLOR(attP->text.attr.color);
    text_path = attP->text.attr.path;
    if (text_path == DOWN || text_path == UP)
	fixed_font = 1;
    else
	fixed_font = attP->text.fixed_font;
    halign = attP->text.attr.halign;
    valign = attP->text.attr.valign;
    concat = _cgi_00;
    off = *origin;

    if (strlen(tstring) > MAXCHAR)
	return (EMEMSPAC);
    if (strlen(tstring) <= 0)
    {
	if (extent != (Ccoor *) 0)
	{
	    extent[0] = _cgi_00;
	    extent[1] = _cgi_00;
	    extent[2] = _cgi_00;
	}
	return (err);
    }
		
    if (attP->text.attr.precision == STRING)
    {				/* raster text */
	char           *fontname;

	if (attP->text.attr.current_font != _cgi_state.open_font.num)
	{
	    /* reset font if necessary */
	    if (_cgi_state.open_font.ptr != (struct pixfont *) 0)
	    {
		(void) pf_close(_cgi_state.open_font.ptr);
		_cgi_state.open_font.num = -1;
	    }
	    switch (attP->text.attr.current_font)
	    {
	    case (ROMAN):
		fontname = "/usr/lib/fonts/fixedwidthfonts/gallant.r.10";
		break;
	    case (SCRIPT):
		fontname = "/usr/lib/fonts/fixedwidthfonts/sail.r.6";
		break;
	    default:
		/* load default font */
		fontname = "/usr/lib/fonts/fixedwidthfonts/gacha.r.8";
		break;
	    }			/* font set */
	    _cgi_state.open_font.ptr = pf_open(fontname);
	    if (_cgi_state.open_font.ptr == (struct pixfont *) 0)
		_cgi_perror(fontname);
	    else
		_cgi_state.open_font.num = attP->text.attr.current_font;
	}

	if (_cgi_state.open_font.ptr == (struct pixfont *) 0)
	    return (EFILACC);

	/* get concatenation point for raster text */
	vertical = 0;
	text_path = RIGHT;

	/* generate values for font_info structure */
	(void) pf_textbound(&bound, strlen("X"), _cgi_state.open_font.ptr, "X");
	_cgi_rev_dev_scale_only(font_info.ft_topline.x, font_info.ft_topline.y,
				0, bound.pos.y + 1);
	_cgi_rev_dev_scale_only(font_info.ft_bottomline.x,
				font_info.ft_bottomline.y,
				0, bound.pos.y + bound.size.y);
	font_info.ft_capline = font_info.ft_topline;

	font_info.ft_height.x = 0;
	font_info.ft_height.y = font_info.ft_topline.y
	    - font_info.ft_bottomline.y;

	_cgi_rev_dev_scale_only(font_info.ft_left.x, font_info.ft_left.y,
				bound.pos.x, 0);
	_cgi_rev_dev_scale_only(font_info.ft_right.x, font_info.ft_right.y,
				bound.pos.x + bound.size.x, 0);

	(void) pf_textbound(&bound, strlen(tstring),
			    _cgi_state.open_font.ptr, tstring);
	lleft.x = font_info.ft_left.x;
	lleft.y = font_info.ft_bottomline.y;
	uleft.x = font_info.ft_left.x;
	uleft.y = font_info.ft_topline.y;
	_cgi_rev_dev_scale_only(uright.x, uright.y,
				bound.pos.x + bound.size.x, bound.pos.y + 1);
	ref.x = uright.x - uleft.x;
	ref.y = 0;
    }
    else
    {				/* vector text */
	/*
	 *PLEASE NOTE - PLEASE NOTE - PLEASE NOTE - PLEASE_NOTE
	 *	from here to the end of this else block, the coordinate
	 *	values are unscaled. This was done to get better rounding
	 * 	on integer valued.  Also note that _cgi_get_glyph works
	 *  in unscaled coordinates
	*/
	char           *cp, *sp;
	char            ch, string_buf[MAXCHAR + 1];
	Cfloat          spfac;
	Ccoor           charwidth;	/* vector for character width */
	Ccoor           space;	/* vector for inter-character spacing */
	Ccoor			newspace;	/* scaled space */

	/*
	 * Iterate through the string getting pointers to the cached glyph
	 * information, and calculating offsets for each glyph within the
	 * string. RIGHT and LEFT text are both set to the RIGHT, but the
	 * contents of the string are reversed for LEFT path, with (0,0) being
	 * the left side  and baseline of the leftmost character (ie. the first
	 * character set, the one at glyph_list[0]).
	 * 
	 * DOWN and UP text are both set DOWN, but the contents of the string are
	 * reversed for UP path, with (0,0) being the center and baseline of
	 * the topmost character (ie. the first character set, the one at
	 * glyph_list[0]).
	 * 
	 * After the entire string is set, 'off.x' and 'off.y' are calculated to
	 * offset the entire character box so that the concatenation point is
	 * aligned appropriately for the path and alignment in use.
	 */
	font_info = _cgi_inq_font(attP->text.attr.current_font);
	ref = _cgi_00;
	glyphP = glyph_list;
	spfac = attP->text.attr.space * font_info.ft_cap;

	switch (text_path)
	{
	case RIGHT:
	    space = fcc_to_vdc(spfac, 0.);
		newspace = fcc_to_vdc_unscaled (spfac,0.);
	    cp = tstring;
	    break;
	case DOWN:
	    vertical = 1;
	    space = fcc_to_vdc(0., -spfac);
	    newspace = fcc_to_vdc_unscaled(0., -spfac);
	    cp = tstring;
	    break;
	case LEFT:
	    space = fcc_to_vdc(spfac, 0.);
	    newspace = fcc_to_vdc_unscaled(spfac, 0.);

	    /* Reverse the given string, and draw it RIGHT */
	    cp = &string_buf[strlen(tstring)];
	    *cp = 0;
	    for (sp = tstring; *sp;)
		*--cp = *sp++;
	    break;
	case UP:
	    vertical = 1;
	    space = fcc_to_vdc(0., -spfac);
	    newspace = fcc_to_vdc_unscaled(0., -spfac);

	    /* Reverse the given string, and draw it DOWN */
	    cp = &string_buf[strlen(tstring)];
	    *cp = 0;
	    for (sp = tstring; *sp;)
		*--cp = *sp++;
	    break;
	}

	while ((ch = *cp++) != '\0')
	{
	    /* Query font for character info and allocate space for vectors */
	    err = _cgi_get_glyph(attP->text.attr.current_font, ch,
				 &(glyphP->g_cache));
	    if (err)
		return (err);

	    /* if variable width font use actual character size */
	    if (fixed_font)
	    {
		/* MDH 880324: Didn't used to account for exp_factor here.
		 * Added, but note that this will also 'shift' the edges
		 * of the glyph box if they are both on the same side of the
		 * X axis.  As far as I know, no glyph does this.  */
		glyphP->g_left_box.x = SCALE(_cgi_rnd_flt_to_int(
		    ((Cfloat)font_info.ft_left.x) * attP->text.attr.exp_factor));
		glyphP->g_right_box.x = SCALE(_cgi_rnd_flt_to_int(
		    ((Cfloat)font_info.ft_right.x)* attP->text.attr.exp_factor));
		glyphP->g_left_box.y = SCALE(font_info.ft_left.y);
		glyphP->g_right_box.y = SCALE(font_info.ft_right.y);
	    }
	    else
	    {
		glyphP->g_left_box = glyphP->g_cache->gc_left;
		glyphP->g_right_box = glyphP->g_cache->gc_right;
	    }
	    charwidth.x = glyphP->g_right_box.x - glyphP->g_left_box.x;
	    charwidth.y = glyphP->g_right_box.y - glyphP->g_left_box.y;
	    glyphP->g_ref = ref;

	    /*
	     * Calculate character position information based on text path
	     */
	    switch (text_path)
	    {
	    case RIGHT:
	    case LEFT:
		glyphP->g_off.x = ref.x - glyphP->g_left_box.x;
		glyphP->g_off.y = ref.y - glyphP->g_left_box.y;
		ref.x += charwidth.x + newspace.x;
		ref.y += charwidth.y + newspace.y;
		break;
	    case UP:
	    case DOWN:
		glyphP->g_off.x = ref.x;
		glyphP->g_off.y = ref.y;

		/*
		 * Subtract the height vector (move down), and add the spacing
		 * (already computed for downward movement).
		 */
		ref.x += SCALE(-font_info.ft_height.x) + newspace.x;
		ref.y += SCALE(-font_info.ft_height.y) + newspace.y;
		break;
	    }
	    glyphP++;
	}

	/*
	 * now scale g_left_box, g_right_box, g_ref, and g_off 
	*/
	nglyph = glyphP - glyph_list;
	newglyphP = glyph_list;

	for (loop=0;loop<nglyph;loop++) {
		UNSCALE(newglyphP->g_off.x);
		UNSCALE(newglyphP->g_off.y);
		UNSCALE(newglyphP->g_left_box.x);
		UNSCALE(newglyphP->g_left_box.y);
		UNSCALE(newglyphP->g_right_box.x);
		UNSCALE(newglyphP->g_right_box.y);
		UNSCALE(newglyphP->g_ref.x);
		UNSCALE(newglyphP->g_ref.y);
		newglyphP++;
	}
	UNSCALE(ref.x);
	UNSCALE(ref.y);

	--glyphP;		/* leave it pointing at last glyph */
	if (vertical)
	{
	    lleft.x = glyphP->g_off.x
		+ font_info.ft_left.x
		+ font_info.ft_bottomline.x;
	    lleft.y = glyphP->g_off.y
		+ font_info.ft_left.y
		+ font_info.ft_bottomline.y;
	    uleft.x = glyph_list[0].g_off.x
		+ font_info.ft_left.x
		+ font_info.ft_topline.x;
	    uleft.y = glyph_list[0].g_off.y
		+ font_info.ft_left.y
		+ font_info.ft_topline.y;
	    uright.x = glyph_list[0].g_off.x
		+ font_info.ft_right.x
		+ font_info.ft_topline.x;
	    uright.y = glyph_list[0].g_off.y
		+ font_info.ft_right.y
		+ font_info.ft_topline.y;

	    /*
	     * The vertical cases (UP,DOWN) were set with the (0,0) reference
	     * at (center,baseline) of the glyph.  Adjust this to
	     * (left,baseline) to match the horizontal cases so the alignment
	     * below can treat both the same.
	     */
	    off.x -= font_info.ft_left.x;
	    off.y -= font_info.ft_left.y;
	}
	else
	{
	    lleft.x = glyph_list[0].g_off.x
		+ glyph_list[0].g_left_box.x
		+ font_info.ft_bottomline.x;
	    lleft.y = glyph_list[0].g_off.y
		+ glyph_list[0].g_left_box.y
		+ font_info.ft_bottomline.y;
	    uleft.x = glyph_list[0].g_off.x
		+ glyph_list[0].g_left_box.x
		+ font_info.ft_topline.x;
	    uleft.y = glyph_list[0].g_off.y
		+ glyph_list[0].g_left_box.y
		+ font_info.ft_topline.y;
	    uright.x = glyphP->g_off.x
		+ glyphP->g_right_box.x
		+ font_info.ft_topline.x;
	    uright.y = glyphP->g_off.y
		+ glyphP->g_right_box.y
		+ font_info.ft_topline.y;
	}
    }

    /*
     * Align text: 1) Adjust halign and valign for NORMAL/NRMAL cases. 2) If
     * path is UP or DOWN, set 'off' to adjust the (0,0) reference point from
     * center/baseline to left/baseline, to make it match horizontal text, so
     * alignment can work the same on both. 3) Do horizontal alignment: adjust
     * 'off' 4) Do vertical alignment: adjust 'off'
     * 
     * The text extent box (lleft, uleft, uright) is calculated to match the
     * coordinates the glyph point lists are in, ie. 'off' must be added to
     * them to get the actual pixel positions. This allows these coordinates to
     * be treated the same as the vector information.
     * 
     * The new concatentation point is relative to the current origin inde-
     * pendent of off.x or off.y. See GKS standard, ANSI X3.124-1985, p 159.
     */
    switch (text_path)
    {
    case RIGHT:
	if (halign == NRMAL)
	    halign = LFT;
	if (valign == NORMAL)
	    valign = BASE;
	break;
    case LEFT:
	if (halign == NRMAL)
	    halign = RGHT;
	if (valign == NORMAL)
	    valign = BASE;
	break;
    case UP:
	if (halign == NRMAL)
	    halign = CNTER;
	if (valign == NORMAL)
	    valign = BASE;
	break;
    case DOWN:
	if (halign == NRMAL)
	    halign = CNTER;
	if (valign == NORMAL)
	    valign = TOP;
	break;
    }

    switch (halign)
    {
    case LFT:
	if (!vertical)
	    concat = ref;
	break;
    case CNTER:
	off.x -= (uright.x - uleft.x) / 2;
	off.y -= (uright.y - uleft.y) / 2;
	break;
    case RGHT:
	off.x -= uright.x - uleft.x;
	off.y -= uright.y - uleft.y;
	if (!vertical)
	{
	    concat.x = -ref.x;
	    concat.y = -ref.y;
	}
	break;
    case CNT:
	continuous = attP->text.attr.hcalind;
	if (text_path == LEFT)
	    continuous = 1.0 - continuous;
	off.x -= (uright.x - uleft.x) * continuous;
	off.y -= (uright.y - uleft.y) * continuous;
	break;
    }

    switch (valign)
    {
    case TOP:
	/* topline of topmost character */
	off.x -= font_info.ft_topline.x;
	off.y -= font_info.ft_topline.y;
	if (vertical)
	    concat = ref;
	break;
    case CAP:
	/* capline of topmost character */
	off.x -= font_info.ft_capline.x;
	off.y -= font_info.ft_capline.y;
	if (vertical)
	    concat = ref;
	break;
    case HALF:
	/*
	 * halfway between the halfline of the topmost character and the
	 * halfline of the bottommost character. Halfline is assumed to be at
	 * capline/2.
	 */
	off.x -= font_info.ft_capline.x / 2;
	off.y -= font_info.ft_capline.y / 2;
	if (vertical)
	{
	    off.x -= glyphP->g_ref.x / 2;
	    off.y -= glyphP->g_ref.y / 2;
	}
	break;
    case BOTTOM:
	/* offset to bottomline of bottommost character */
	off.x -= font_info.ft_bottomline.x;
	off.y -= font_info.ft_bottomline.y;
	/* falls through to BASE case */
    case BASE:
	/* baseline of bottommost character */
	if (vertical)
	{
	    off.x -= glyphP->g_ref.x;
	    off.y -= glyphP->g_ref.y;
	    concat.x = -ref.x;
	    concat.y = -ref.y;
	}
	break;
    case CONT:
	continuous = attP->text.attr.vcalind;
	if (text_path == UP)
	    continuous = 1.0 - continuous;
	off.x -= (uleft.x - lleft.x) * continuous;
	off.y -= (uleft.y - lleft.y) * continuous;
	break;
    }

    /* Update saved concatenation point */
    attP->text.concat_pt.x = concat.x + origin->x;
    attP->text.concat_pt.y = concat.y + origin->y;

    /*
     * Draw text, if requested
     */
    if (extent == (Ccoor *) 0)
    {
	Ccoor           dc_off;

	if (attP->text.attr.precision == STRING)
	{
	    _cgi_devscale(off.x, off.y, dc_off.x, dc_off.y);
	    if (_cgi_ttext)
		/* draw transparent text */
		pw_ttext(vwsP->sunview.pw, dc_off.x, dc_off.y, txtop,
			 _cgi_state.open_font.ptr, tstring);
	    else
		/* draw solid text */
		pw_text(vwsP->sunview.pw, dc_off.x, dc_off.y, txtop,
			_cgi_state.open_font.ptr, tstring);
	}
	else
	{
	    Ccoor           saved_vdc_off, vdc_off;
	    Clinatt         saved_line;
	    Cint            saved_conv_width;

	    /*
	     * There ought to be a better way than setting these global
	     * variables...  wcl 6/27/86
	     */
	    saved_vdc_off.x = vwsP->xform.off.x;
	    saved_vdc_off.y = vwsP->xform.off.y;
	    saved_line = attP->line;
	    saved_conv_width = vwsP->conv.line_width;
	    attP->line.color = attP->text.attr.color;
	    attP->line.width = (Cfloat) 1.0;
	    vwsP->conv.line_width = 1;
	    attP->line.style = SOLID;
	    glyphP = glyph_list;
	    pw_lock(vwsP->sunview.pw, &vwsP->sunview.lock_rect);
	    for (cnt = nglyph; --cnt >= 0; glyphP++)
	    {
		if (glyphP->g_cache->gc_npts >= 2)
		{
		    vdc_off.x = off.x + glyphP->g_off.x;
		    vdc_off.y = off.y + glyphP->g_off.y;
		    vwsP->xform.off.x = saved_vdc_off.x;
		    vwsP->xform.off.y = saved_vdc_off.y;
		    _cgi_devscale(vdc_off.x, vdc_off.y, dc_off.x, dc_off.y);
		    vwsP->xform.off.x = dc_off.x;
		    vwsP->xform.off.y = dc_off.y;
		    terr = _cgi_polyline(vwsP, (int) glyphP->g_cache->gc_npts,
					 glyphP->g_cache->gc_coors,
					 glyphP->g_cache->gc_mvlist,
					 OFFSET_ONLY);
		    /* If any error occurs, save it, and continue */
		    if (!err)
			err = terr;
		}
	    }
	    pw_unlock(vwsP->sunview.pw);
	    vwsP->conv.line_width = saved_conv_width;
	    attP->line = saved_line;
	    vwsP->xform.off.x = saved_vdc_off.x;
	    vwsP->xform.off.y = saved_vdc_off.y;
	}
    }
    else
    {
	/*
	 * Return the bounding box to cgipw_inquire_text_extent.
	 */
	extent[0].x = lleft.x + off.x;
	extent[0].y = lleft.y + off.y;
	extent[1].x = uleft.x + off.x;
	extent[1].y = uleft.y + off.y;
	extent[2].x = uright.x + off.x;
	extent[2].y = uright.y + off.y;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_get_glyph_polyline					    */
/*									    */
/* Return a Ccoor pointlist, and a pw_polyline-compatible move list	    */
/* for a glyph in a font.  The reference point in return values		    */
/* is X==0 at lateral center of character box, Y==0 at baseline		    */
/****************************************************************************/
_cgi_get_glyph_polyline(font, ch, polypts, mvlist, glyph_infoP)
int             font;
char            ch;
Ccoor          *polypts;
u_char         *mvlist;
Cglyphinfo     *glyph_infoP;
{
    register Ccoor *polyptP;
    register u_char *mvP;
    register char  *ptP;
    register short  npts, penup, realpts;
    char           *pt_base;
    int             nglyphs;
    int             ymin, ymax;

    if (font < 4)
	_cgi_scribefont(font, ch, &pt_base, &nglyphs);
    else
	_cgi_stickfont(font - 4, ch, &pt_base, &nglyphs);

    ptP = pt_base;
    polyptP = polypts;
    mvP = mvlist;
    npts = *ptP++;
    glyph_infoP->gi_xmin = *ptP++;
    glyph_infoP->gi_xmax = *ptP++;
    realpts = 0;
    ymin = 0;
    ymax = 0;
    penup = 1;
    while (--npts >= 0)
    {
	if (*ptP == 31)
	{
	    penup = 1;
	    ptP += 2;
	}
	else
	{
	    polyptP->x = *ptP++;
	    polyptP->y = *ptP++ + 9;	/* move vertical ref. to baseline */
	    if (polyptP->y < ymin)
		ymin = polyptP->y;
	    if (polyptP->y > ymax)
		ymax = polyptP->y;
	    polyptP++;
	    *mvP++ = penup;
	    penup = 0;
	    realpts++;
	}
    }
    mvlist[0] = 0;
    glyph_infoP->gi_npts = realpts;
    glyph_infoP->gi_ymin = ymin;
    glyph_infoP->gi_ymax = ymax;
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_get_glyph 						    */
/*									    */
/* Return glyph cache entry, building it if necessary.			    */
/****************************************************************************/
Cerror          _cgi_get_glyph(font, ch, glyph_cacheP)
int             font;
char            ch;
Cglyphcache   **glyph_cacheP;
{
    register Ccoor *csptP;
    register Ccoor *coorP;
    register Cglyphcache *gcP;
    register Conv_text *textP = &_cgi_vws->conv.text;
    register short  i;
    register int    sin_up, cos_base, cos_up, sin_base;
    Cint            xmin = 0x7fffffff;
    Cint            xmax = -0x7fffffff;
    Cint            ymin = 0x7fffffff;
    Cint            ymax = -0x7fffffff;
    int             rx, ry;
    Ccoor           polypts[MAXPTS];
    u_char          mvlist_temp[MAXPTS];
    Cglyphinfo      glyph_info;

#define	X_CC_TO_DC(x,y) ((((x * cos_base) + (y * cos_up))		\
			+ (1 << (CC_DC_SHIFT - 1))) >> CC_DC_SHIFT)
#define	Y_CC_TO_DC(x,y) ((((x * sin_base) + (y * sin_up))		\
			+ (1 << (CC_DC_SHIFT - 1))) >> CC_DC_SHIFT)

    gcP = &_cgi_glyph_cache[(int) ch];
#ifdef	TEXT_CACHE
    if (_cgi_notextcache
	|| gcP->gc_vdc_change_cnt != _cgi_state.vdc_change_cnt
	|| gcP->gc_text_change_cnt != _cgi_state.text_change_cnt
	|| gcP->gc_font != font)
    {
#endif	TEXT_CACHE
	/*
	 * Invalidate old entry: free space associated with it, and sets its
	 * change_cnt values to 0.  The latter is not necessary for the current
	 * routine, but allows invalidate_old_cache_entry() to be used
	 * elsewhere.
	 */
	invalidate_old_cache_entry(gcP);

	/*
	 * Get new transform parameters; reset transform parameters stored in
	 * glyph cache structure; get character-space copy of point list.
	 */
	gcP->gc_vdc_change_cnt = _cgi_state.vdc_change_cnt;
	gcP->gc_text_change_cnt = _cgi_state.text_change_cnt;
	gcP->gc_font = font;
	sin_base = textP->dc_sin_base;
	cos_base = textP->dc_cos_base;
	sin_up = textP->dc_sin_up;
	cos_up = textP->dc_cos_up;

	_cgi_get_glyph_polyline(font, ch, polypts, mvlist_temp, &glyph_info);
	gcP->gc_npts = glyph_info.gi_npts;

	if (glyph_info.gi_npts >= 2)
	{
	    /*
	     * Allocate new space and copy temporary move list to it.
	     */
	    gcP->gc_coors = (Ccoor *) malloc(gcP->gc_npts * sizeof(Ccoor));
	    if (gcP->gc_coors == (Ccoor *) 0)
		return (EMEMSPAC);
	    gcP->gc_mvlist = (u_char *) malloc(gcP->gc_npts * sizeof(u_char));
	    if (gcP->gc_mvlist == (u_char *) 0)
	    {
		free((char *) gcP->gc_coors);
		return (EMEMSPAC);
	    }
	    bcopy((char *) mvlist_temp, (char *) gcP->gc_mvlist, (int) gcP->gc_npts);
	    /*
	     * Transform coordinates to DC and copy to allocated space.
	     */
	    csptP = polypts;
	    coorP = gcP->gc_coors;
	    for (i = gcP->gc_npts; --i >= 0;)
	    {
		rx = csptP->x;
		ry = csptP++->y;
		coorP->x = X_CC_TO_DC(rx, ry);
		coorP->y = Y_CC_TO_DC(rx, ry);
		xmin = MIN(xmin, coorP->x);
		xmax = MAX(xmax, coorP->x);
		ymin = MIN(ymin, coorP->y);
		ymax = MAX(ymax, coorP->y);
		coorP++;
	    }
	}
	else
	{
	    Cint            xleft, yleft, xright, yright;

	    /*
	     * Glyphs without any vectors still have two 'points' that are
	     * implicit: the left and right side of the box. These should be
	     * transformed, and used to set xmin, xmax, ymin, ymax.
	     */
	    xleft = X_CC_TO_DC(glyph_info.gi_xmin, 0);
	    yleft = Y_CC_TO_DC(glyph_info.gi_xmin, 0);
	    xright = X_CC_TO_DC(glyph_info.gi_xmax, 0);
	    yright = Y_CC_TO_DC(glyph_info.gi_xmax, 0);
	    xmin = MIN(xleft, xright);
	    ymin = MIN(yleft, yright);
	    xmax = MAX(xleft, xright);
	    ymax = MAX(yleft, yright);
	}
	gcP->gc_ll.x = xmin;
	gcP->gc_ll.y = ymax;
	gcP->gc_ur.x = xmax;
	gcP->gc_ur.y = ymin;
	gcP->gc_left = cc_to_vdc_unscaled(glyph_info.gi_xmin, 0);
	gcP->gc_right = cc_to_vdc_unscaled(glyph_info.gi_xmax, 0);
#ifdef	TEXT_CACHE
    }
#endif	TEXT_CACHE

    *glyph_cacheP = gcP;
    return (NO_ERROR);

#undef	X_CC_TO_DC(x,y)
#undef	Y_CC_TO_DC(x,y)
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_free_text_cache					    */
/*									    */
/*	Free all malloc'd space associated with glyph cache, and	    */
/*	invalidate all entries.						    */
/*									    */
/****************************************************************************/
_cgi_free_text_cache()
{
    register Cglyphcache *gcP;
    register int    cnt;

    gcP = _cgi_glyph_cache;
    for (cnt = MAX_CHAR_PER_FONT; --cnt >= 0; gcP++)
	invalidate_old_cache_entry(gcP);
}

/****************************************************************************/
/*									    */
/* FUNCTION: invalidate_old_cache_entry					    */
/*									    */
/*	Free malloc'd space associated with a single glyph cache entry,	    */
/*	and invalidate it by setting change_cnt to 0.			    */
/*									    */
/****************************************************************************/
static          invalidate_old_cache_entry(gcP)
register Cglyphcache *gcP;
{
    if (gcP->gc_coors != (Ccoor *) NULL)
    {
	free((char *) gcP->gc_coors);
	gcP->gc_coors = NULL;
    }
    if (gcP->gc_mvlist != (u_char *) NULL)
    {
	free((char *) gcP->gc_mvlist);
	gcP->gc_mvlist = NULL;
    }
    gcP->gc_vdc_change_cnt = 0;
    gcP->gc_text_change_cnt = 0;
    gcP->gc_font = NO_FONT;
    gcP->gc_npts = 0;
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_inq_font 						    */
/*									    */
/* Returns a pointer to a Cfontinfo structure for the given font.	    */
/****************************************************************************/
Cfontinfo       _cgi_inq_font(font)
int             font;
{
    register Cfontinfo *fontP;

    fontP = &_cgi_fontinfo[font];
    if (!fontP->ft_flag)
	_cgi_enter_font_info(font);

#ifdef	TEXT_CACHE
    if (_cgi_notextcache
	|| fontP->ft_vdc_change_cnt != _cgi_state.vdc_change_cnt
	|| fontP->ft_text_change_cnt != _cgi_state.text_change_cnt)
    {
#endif	TEXT_CACHE
	fontP->ft_vdc_change_cnt = _cgi_state.vdc_change_cnt;
	fontP->ft_text_change_cnt = _cgi_state.text_change_cnt;
	fontP->ft_topline = cc_to_vdc(0, fontP->ft_top);
	fontP->ft_capline = cc_to_vdc(0, fontP->ft_cap);
	fontP->ft_bottomline = cc_to_vdc(0, fontP->ft_bottom);
	fontP->ft_height.x = fontP->ft_topline.x - fontP->ft_bottomline.x;
	fontP->ft_height.y = fontP->ft_topline.y - fontP->ft_bottomline.y;
	fontP->ft_left = cc_to_vdc(fontP->ft_xmin, 0);
	fontP->ft_right = cc_to_vdc(fontP->ft_xmax, 0);
#ifdef	TEXT_CACHE
    }
#endif	TEXT_CACHE

    return (*fontP);
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_enter_font_info					    */
/*									    */
/* Fills in a Cfontinfo structure by reading the glyphs in the font.	    */
/****************************************************************************/
_cgi_enter_font_info(font)
int             font;
{
    register short  i;
    char            ch;
    int             nglyphs;
    char           *pt_base;
    int             font_top = 0;
    int             font_cap = 0;
    int             font_bottom = 0;
    int             font_xmin = 0;
    int             font_xmax = 0;
    Ccoor           polypts[MAXPTS];
    u_char          mvlist[MAXPTS];
    Cglyphinfo      glyph_info;

    /* Get number of glyphs in font */
    ch = ' ';
    if (font < 4)
	_cgi_scribefont(font, ch, &pt_base, &nglyphs);
    else
	_cgi_stickfont(font - 4, ch, &pt_base, &nglyphs);

    for (i = nglyphs; --i >= 0;)
    {
	_cgi_get_glyph_polyline(font, ch, polypts, mvlist, &glyph_info);
	font_top = MAX(font_top, glyph_info.gi_ymax);
	if ('A' <= ch && ch <= 'Z')
	    font_cap = MAX(font_cap, glyph_info.gi_ymax);
	font_bottom = MIN(font_bottom, glyph_info.gi_ymin);
	font_xmin = MIN(font_xmin, glyph_info.gi_xmin);
	font_xmax = MAX(font_xmax, glyph_info.gi_xmax);
	ch++;
    }

    _cgi_fontinfo[font].ft_top = font_top;
    _cgi_fontinfo[font].ft_cap = font_cap;
    _cgi_fontinfo[font].ft_bottom = font_bottom;
    _cgi_fontinfo[font].ft_xmin = font_xmin;
    _cgi_fontinfo[font].ft_xmax = font_xmax;
    _cgi_fontinfo[font].ft_flag = 1;
}

/****************************************************************************/
/*									    */
/* FUNCTION: cc_to_vdc							    */
/*									    */
/*	Convert an x,y position in integer character coordinates	    */
/*	to a Ccoor in VDC.  Since this function is static, it needn't	    */
/*	start with "_cgi_".  This function is currently used to set up	    */
/*	glyph and font extent information, which isn't done often.	    */
/*	If it gets used more in the future, it should be turned into a	    */
/*	macro.								    */
/****************************************************************************/
static Ccoor    cc_to_vdc(x, y)
int             x, y;
{
    register int    temp;
    register int    half = (1 << (CC_VDC_SHIFT - 1));
    Ccoor           vdc_coor;

    /**
     ** The integer rounding may look funny, but it's correct.  In
     ** particular, the expression
     **		(temp - half) >> CC_VDC_SHIFT		[wrong]
     ** is NOT equivalent to
     **		-((-temp + half) >> CC_VDC_SHIFT)	[correct]
     **
     ** because the truncation of the fractional part of temp is
     ** always towards more negative numbers, rather than toward 0.
     ** Integer shifts of signed numbers tend toward -1 as the amount
     ** of shift increases.  If temp were some negative number with
     ** small magnitude, say -0.001, and we subtracted 1/2 and
     ** truncated, we would get -1 as a result, rather than 0.
     **/

    /* Generate x VDC coordinate */
    temp = ((x * _cgi_vws->conv.text.vdc_cos_base)
	    + (y * _cgi_vws->conv.text.vdc_cos_up));
    if (temp < 0)
	vdc_coor.x = -((-temp + half) >> CC_VDC_SHIFT);
    else
	vdc_coor.x = (temp + half) >> CC_VDC_SHIFT;

    /* Generate y VDC coordinate */
    temp = ((x * _cgi_vws->conv.text.vdc_sin_base)
	    + (y * _cgi_vws->conv.text.vdc_sin_up));
    if (temp < 0)
	vdc_coor.y = -((-temp + half) >> CC_VDC_SHIFT);
    else
	vdc_coor.y = (temp + half) >> CC_VDC_SHIFT;

    return (vdc_coor);
}
/****************************************************************************/
/*									    */
/* FUNCTION: cc_to_vdc_unscaled							    */
/*									    */
/*	Convert an x,y position in integer character coordinates	    */
/*	to a Ccoor in VDC.  Since this function is static, it needn't	    */
/*	start with "_cgi_".  This function is currently used to set up	    */
/*	glyph and font extent information, which isn't done often.	    */
/*	If it gets used more in the future, it should be turned into a	    */
/*	macro.								    */
/****************************************************************************/
static Ccoor    cc_to_vdc_unscaled(x, y)
int             x, y;
{
    register int    temp;
    register int    half = (1 << (CC_VDC_SHIFT - 1));
    Ccoor           vdc_coor;

    /**
     ** The integer rounding may look funny, but it's correct.  In
     ** particular, the expression
     **		(temp - half) >> CC_VDC_SHIFT		[wrong]
     ** is NOT equivalent to
     **		-((-temp + half) >> CC_VDC_SHIFT)	[correct]
     **
     ** because the truncation of the fractional part of temp is
     ** always towards more negative numbers, rather than toward 0.
     ** Integer shifts of signed numbers tend toward -1 as the amount
     ** of shift increases.  If temp were some negative number with
     ** small magnitude, say -0.001, and we subtracted 1/2 and
     ** truncated, we would get -1 as a result, rather than 0.
     **/

    /* Generate x VDC coordinate */
    temp = ((x * _cgi_vws->conv.text.vdc_cos_base)
	    + (y * _cgi_vws->conv.text.vdc_cos_up));
    if (temp < 0)
	vdc_coor.x = -(-temp + half);
    else
	vdc_coor.x = (temp + half);

    /* Generate y VDC coordinate */
    temp = ((x * _cgi_vws->conv.text.vdc_sin_base)
	    + (y * _cgi_vws->conv.text.vdc_sin_up));
    if (temp < 0)
	vdc_coor.y = -(-temp + half);
    else
	vdc_coor.y = (temp + half) ;

    return (vdc_coor);
}

/****************************************************************************/
/*									    */
/* FUNCTION: fcc_to_vdc							    */
/*									    */
/*	Convert an x,y position in floating-point character coordinates	    */
/*	to a Ccoor in VDC.  Floating character coordinates are used when    */
/*	additional precision is necessary, such as for the amount of	    */
/*	spacing.  Since this function is static, it needn't start with	    */
/*	"_cgi_".							    */
/****************************************************************************/
static Ccoor    fcc_to_vdc(x, y)
Cfloat          x, y;
{
    register Cfloat temp;
    static Cfloat   scaling = (Cfloat) (1 << CC_VDC_SHIFT);
    Ccoor           vdc_coor;	/* returned values go here */

    /* Generate x VDC coordinate */
    temp = (((x * _cgi_vws->conv.text.vdc_cos_base)
	     + (y * _cgi_vws->conv.text.vdc_cos_up))) / scaling;
    vdc_coor.x = _cgi_rnd_flt_to_int(temp);

    /* Generate y VDC coordinate */
    temp = (((x * _cgi_vws->conv.text.vdc_sin_base)
	     + (y * _cgi_vws->conv.text.vdc_sin_up))) / scaling;
    vdc_coor.y = _cgi_rnd_flt_to_int(temp);

    return (vdc_coor);
}
static Ccoor    fcc_to_vdc_unscaled(x, y)
Cfloat          x, y;
{
    register Cfloat temp;
    static Cfloat   scaling = (Cfloat) (1 << CC_VDC_SHIFT);
    Ccoor           vdc_coor;	/* returned values go here */

    /* Generate x VDC coordinate */
    temp = (((x * _cgi_vws->conv.text.vdc_cos_base)
	     + (y * _cgi_vws->conv.text.vdc_cos_up)));
    vdc_coor.x = _cgi_rnd_flt_to_int(temp);

    /* Generate y VDC coordinate */
    temp = (((x * _cgi_vws->conv.text.vdc_sin_base)
	     + (y * _cgi_vws->conv.text.vdc_sin_up)));
    vdc_coor.y = _cgi_rnd_flt_to_int(temp);

    return (vdc_coor);
}

/****************************************************************************/
/*									    */
/* FUNCTION: _cgi_rnd_flt_to_int					    */
/*									    */
/*	DO the obvious rounding on a floating-point number.  This is a	    */
/*	function because it's done a lot, and the function call overhead    */
/*	is relatively cheap compared to the floating-point time, although   */
/*	if it were called a lot it should be turned into a macro.	    */
/****************************************************************************/
Cint            _cgi_rnd_flt_to_int(fnum)
Cfloat          fnum;
{
    if (fnum < 0)
	return ((Cint) (fnum - 0.5));
    else
	return ((Cint) (fnum + 0.5));
}
