#ifndef lint
static char	sccsid[] = "@(#)pixels2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Bitblt Output Primitives
 */

/*
bitblt_source_array
bitblt_pattern_array
bitblt_patterned_source_array 
set_global_drawing_mode 
set_drawing_mode 
inquire_drawing_mode 
inquire_device_bitmap 
inquire_bitblt_alignments 
*/

#include "cgipriv.h"		/* defines types used in this file  */
#include <pixrect/pixrect_hs.h>

#ifdef	i386
#define	UCHAR_MSB	0x01
#else	!i386
#define	UCHAR_MSB	0x80
#endif	i386

int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
int             _cgi_background_color;	/* background color (no longer used) */
View_surface   *_cgi_vws;	/* current view surface */

Ccombtype _cgi_drawing_mode;
int             _cgi_pix_drawing_mode;
Ccombtype _cgi_bit_drawing_mode;
Cbmode _cgi_bit_vis_mode;
Cbitmaptype _cgi_bit_source;
Cbitmaptype _cgi_bit_dest;


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: bitblt_source_array 					    */
/*                                                                          */
/*	DOES:	Read off screen the rectangle defined by (xo,yo),(xe,ye)    */
/*		Read it into the pixwin provided by bitsource at (0,0)      */
/*		Depending on drawing mode, perform an OP on user's bitsource*/
/*		Using _cgi_pix_mode, ROP bitsource onto			    */
/*		_cgi_vws->sunview.pw at xt,yt				    */
/*		Read result (at xt,yt) into raster array bittarget at (0,0) */
/*                                                                          */
/*	COULD:	Move area in raster array bitsource			    */
/*		defined by (xo,yo),(xe,ye)				    */
/*		to raster array bittarget, positioned at (xt,yt)	    */
/****************************************************************************/

Cerror          bitblt_source_array(bitsource, xo, yo, xe, ye, bittarget, xt, yt, name)
Cpixrect       *bitsource, *bittarget;	/* source and target bitmaps */
Cint            xo, yo, xe, ye, xt, yt;	/* coordinates of source and target
					 * bitmaps */
/* Note: xe,ye (extents) are in screen space; other coordinates in VDC space */
Cint            name;
{
    int             err;
    int             height, width, x, x1, y, y1;

    err = _cgi_err_check_4();
    if (!err)
    {
	_cgi_devscale(xt, yt, x1, y1);	/* target */
	_cgi_devscale(xo, yo, x, y);	/* source */
	width = xe;		/* x and y extent */
	height = ye;
	err = _cgi_err_check_69(x1, y1);
	if (!err)
	    err = _cgi_err_check_69(x, y);
	if (!err)
	    err = _cgi_err_check_69(xe, ye);
	if (!err)
	{
	    err = _cgi_context(name);
	    if (!err)
	    {
		/*
		 * We'll read into the user's pixrects, which will need to
		 * already be allocated, even if the user isn't interested. 
		 */

		pw_read(bitsource, 0, 0, width, height, PIX_SRC,
			_cgi_vws->sunview.pw, x, y);
		pw_read(bittarget, 0, 0, width, height, PIX_SRC,
			_cgi_vws->sunview.pw, x1, y1);
		if (_cgi_bit_vis_mode == TRANSPARENT)
		{
		    Pixrect        *mprect1;

		    /* Transparent: use a stencil based on the source pixrect */
		    if (_cgi_vws->sunview.depth != 1)
		    {
			mprect1 = mem_create(xe, ye, 1);
			(void) _cgi_make_1_deep(mprect1, bitsource, xe, ye);
		    }
		    else
			mprect1 = bitsource;
		    /* Stencil: where source was zero, do not change dest */
		    pw_stencil(_cgi_vws->sunview.pw, x1, y1, xe, ye,
		       _cgi_pix_drawing_mode, mprect1, 0, 0, bitsource, 0, 0);
		    if (mprect1 != bitsource)
			(void) pr_destroy(mprect1);	/* no longer need
							 * pixrect */
		}
		else
		{
		    pw_write(_cgi_vws->sunview.pw, x1, y1, xe, ye,
			     _cgi_pix_drawing_mode, bitsource, 0, 0);
		}

		pw_read(bittarget, 0, 0, width, height, PIX_SRC,
			_cgi_vws->sunview.pw, x1, y1);
	    }
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: bitblt_pattern_array 					    */
/*                                                                          */
/*	DOES:	The above is performed.	 And also:			    */
/*		Read result (at xt,yt) into raster array pixtarget (to 0,0) */
/*                                                                          */
/*	COULD:	ROP onto a subregion (raster array) at (xo,yo) size (xe,ye) */
/*		a replicated pattern of size (px,py) given by pixpat 	    */
/****************************************************************************/

Cerror          bitblt_pattern_array
                (pixpat, px, py, pixtarget, rx, ry, ox, oy, dx, dy, name)
Cpixrect       *pixpat;		/* pattern source array */
Cint            px, py;		/* pattern extent */
Cpixrect       *pixtarget;	/* destination pattern array */
Cint            rx, ry;		/* pattern reference point */
Cint            ox, oy;		/* destination origin in VDC */
Cint            dx, dy;		/* destination extent */
Cint            name;		/* viewsurface name */
{
    int             err, x, y;

    err = _cgi_err_check_4();
    if (!pixpat)
	err = EPXNOTCR;
    if (!err)
    {
	_cgi_devscale(ox, oy, x, y);
	err = _cgi_err_check_69(x, y);
	if (!err)
	    err = _cgi_err_check_69(x + dx, y + dy);
	if (!err)
	{
	    err = _cgi_context(name);
	    if (!err)
	    {
		Pixrect        *repl_source;	/* Gets source for stencil */

		pw_read(pixtarget, 0, 0, dx, dy, PIX_SRC,
			_cgi_vws->sunview.pw, x, y);
		if (_cgi_bit_vis_mode == TRANSPARENT)
		{
		    Pixrect        *repl_stencil;	/* Gets (1-deep) stencil */
		    repl_stencil = mem_create(dx, dy, 1);	/* Full size, 1-bit */
		    /* Transparent: use a stencil based on the source pixrect */
		    if (pixpat->pr_depth != 1)
		    {
			Pixrect        *mprect1;	/* 1-bit deep pattern
							 * tile */
			mprect1 = mem_create(px, py, 1);
			(void) _cgi_make_1_deep(mprect1, pixpat, px, py);
			/* replicate 1-bit tile to 1-bit stencil */
			(void) pr_replrop(repl_stencil, 0, 0, dx, dy, PIX_SRC,
					  mprect1, rx % px, ry % py);
			(void) pr_destroy(mprect1);	/* no longer need
							 * pixrect */
			repl_source = mem_create(dx, dy,
						 _cgi_vws->sunview.depth);
			(void) pr_replrop(repl_source, 0, 0, dx, dy, PIX_SRC,
					  pixpat, rx % px, ry % py);
		    }
		    else
		    {
			(void) pr_replrop(repl_stencil, 0, 0, dx, dy, PIX_SRC,
					  pixpat, rx % px, ry % py);
			repl_source = repl_stencil;
		    }
		    /* Stencil: where source was zero, do not change dest */
		    pw_stencil(_cgi_vws->sunview.pw, x, y, dx, dy,
			       _cgi_pix_drawing_mode, repl_stencil,
			       0, 0, repl_source, 0, 0);
		    (void) pr_destroy(repl_stencil);	/* no longer need
							 * stencil */
		    (void) pr_destroy(repl_source);	/* no longer need source */
		}
		else
		{
		    pw_replrop(_cgi_vws->sunview.pw, x, y, dx, dy,
			     _cgi_pix_drawing_mode, pixpat, rx % px, ry % py);
		}
		/* We'll use the user's pixrects.  S/He'll have to allocate */
		pw_read(pixtarget, 0, 0, dx, dy, PIX_SRC,
			_cgi_vws->sunview.pw, x, y);
	    }
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: bitblt_patterned_source_array 				    */
/*                                                                          */
/*	DOES:	Replicate pixpat tile into size of extent (dx,dy) mprect2.  */
/*		Stencil source area (at sx,sy) of screen through pattern.   */
/*		Depending on drawing mode, perform an OP on the result.     */
/*		Read result (of mask and OP) into user's pixsource (at 0,0).*/
/*		COPY pixsource onto _cgi_vws->sunview.pw at (xt,yt).	    */
/*		Read result (at xt,yt) into raster array pixtarget at (0,0) */
/*                                                                          */
/*	SHOULD:	Perform ROP onto the raster array pixtarget at (xo,yo).	    */
/*		The source is pixsource (at sx,sy).  The extent is (dx,dy). */
/*		The pattern is replicated from the pixpat tile (size dx,dy).*/
/****************************************************************************/
Cerror          bitblt_patterned_source_array(pixpat, px, py, pixsource, sx, sy,
		                      pixtarget, rx, ry, ox, oy, dx, dy, name)
Cpixrect       *pixpat;		/* pattern source array */
Cint            px, py;		/* pattern extent */
Cpixrect       *pixsource;	/* source array */
Cint            sx, sy;		/* source origin */
Cpixrect       *pixtarget;	/* destination pattern array */
Cint            rx, ry;		/* pattern reference point */
Cint            ox, oy;		/* destination origin */
Cint            dx, dy;		/* destination extent */
Cint            name;
/* dx & dy and rx & ry are device coordinates.  Other coordinates are in VDC. */
{
    int             err;
    int             height, width, x, x1, y, y1;
    Pixrect        *mprect_sou, *mprect_pat, *mprect_target;

    err = _cgi_err_check_4();
    if (!pixpat)
	err = EPXNOTCR;
    if (!err)
    {
	_cgi_devscale(ox, oy, x1, y1);	/* target */
	_cgi_devscale(sx, sy, x, y);	/* source */
	width = dx;		/* x and y extent */
	height = dy;
	err = _cgi_err_check_69(x, y);
	if (!err)
	    err = _cgi_err_check_69(x1, y1);
	if (!err)
	{
	    err = _cgi_context(name);
	    if (!err)
	    {
		if ((pixsource->pr_depth == _cgi_vws->sunview.depth)
		    && (pixsource->pr_width >= width)
		    && (pixsource->pr_height >= height)
		    )
		    mprect_sou = pixsource;
		else
		    mprect_sou = mem_create(dx, dy, _cgi_vws->sunview.depth);
		/*
		 * Won't do my work in user's pixrects; they may be too small 
		 */

		if ((pixtarget->pr_depth == _cgi_vws->sunview.depth)
		    && (pixtarget->pr_width >= width)
		    && (pixtarget->pr_height >= height)
		    )
		    mprect_target = pixtarget;
		else
		    mprect_target = mem_create(dx, dy, _cgi_vws->sunview.depth);
		/*
		 * Won't do my work in user's pixrects; they may be too small 
		 */

		/* replicate pattern into mprect_pat: stencil must have depth=1 */
		mprect_pat = mem_create(dx, dy, 1);
		/*
		 * Where repeated pattern tile is non-zero, full-size stencil
		 * will be binary 1. 
		 */
		if (pixpat->pr_depth == 1)
		{
		    (void) pr_replrop(mprect_pat, 0, 0, dx, dy,
				      PIX_SRC, pixpat, rx % px, ry % py);
		}
		else
		{
		    Pixrect        *mprect1;	/* 1-bit-deep pattern tile */

		    mprect1 = mem_create(px, py, 1);
		    (void) _cgi_make_1_deep(mprect1, pixpat, px, py);
		    (void) pr_replrop(mprect_pat, 0, 0, dx, dy,
				      PIX_SRC, mprect1, rx % px, ry % py);
		    (void) pr_destroy(mprect1);	/* no longer need pixrect */
		}

		/* read SRC from screen into mprect_sou */
		pw_read(mprect_sou, 0, 0, width, height, PIX_SRC,
			_cgi_vws->sunview.pw, x, y);

		/* read target area of screen into pixtarget */
		pw_read(mprect_target, 0, 0, width, height, PIX_SRC,
			_cgi_vws->sunview.pw, x1, y1);

		/* ROP mprect_sou onto pixtarget using mprect_pat as mask */
		/* WDS: SunWindows Ref (p 2-10) says must be depth 1 */
		(void) pr_stencil(mprect_target, 0, 0, dx, dy,
				  _cgi_pix_drawing_mode,
				  mprect_pat, 0, 0, mprect_sou, 0, 0);

		/* where mprect_pat was zero mprect_target is unchanged */
		/* COPY onto screen the result of src and pattern */
		pw_write(_cgi_vws->sunview.pw, x1, y1, dx, dy,
			 PIX_SRC, mprect_target, 0, 0);
		/* result on screen is also in mprect_target */

		/*
		 * copy mprect_sou to user's pixsource, if they are different
		 * pixrects 
		 */
		if (pixsource && mprect_sou != pixsource)
		{
		    (void) pr_rop(pixsource, 0, 0, dx, dy, PIX_SRC,
				  mprect_sou, 0, 0);
		    (void) pr_destroy(mprect_sou);
		}

		/*
		 * copy mprect_target to user's pixtarget, if they are
		 * different pixrects 
		 */
		if (pixtarget && mprect_target != pixtarget)
		{
		    (void) pr_rop(pixtarget, 0, 0, dx, dy, PIX_SRC,
				  mprect_target, 0, 0);
		    (void) pr_destroy(mprect_target);
		}
	    }
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_global_drawing_mode				    */
/*                                                                          */
/*                                                                          */
/*		Determines how most primitives are displayed		    */
/****************************************************************************/

Cerror          set_global_drawing_mode(combination)
Ccombtype       combination;	/* combination rules */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_drawing_mode = combination;
	switch (_cgi_drawing_mode)
	{
	case REPLACE:
	    _cgi_pix_mode = PIX_SRC;
	    break;
	case AND:
	    _cgi_pix_mode = PIX_DST & PIX_SRC;
	    break;
	case OR:
	    _cgi_pix_mode = PIX_DST | PIX_SRC;
	    break;
	case NOT:
	    _cgi_pix_mode = PIX_NOT(PIX_SRC);
	    break;
	case XOR:
	    _cgi_pix_mode = PIX_SRC ^ PIX_DST;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_drawing_mode (index)				    */
/*                                                                          */
/*		Determines how bitblt primitives are displayed		    */
/****************************************************************************/

Cerror          set_drawing_mode(visibility, source, destination, combination)
Cbmode          visibility;	/* transparent or opaque */
Cbitmaptype     source;		/* NOT source bits */
Cbitmaptype     destination;	/* NOT destination bits */
Ccombtype       combination;	/* combination rules */
{
    int             pix_src, pix_dst;
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_bit_drawing_mode = combination;
	_cgi_bit_vis_mode = visibility;
	_cgi_bit_source = source;
	_cgi_bit_dest = destination;
	pix_src = (source == BITNOT) ? PIX_NOT(PIX_SRC) : PIX_SRC;
	pix_dst = (destination == BITNOT) ? PIX_NOT(PIX_DST) : PIX_DST;
	switch (_cgi_pix_drawing_mode)
	{
	case REPLACE:
	    _cgi_pix_drawing_mode = pix_src;
	    break;
	case AND:
	    _cgi_pix_drawing_mode = pix_src & pix_dst;
	    break;
	case OR:
	    _cgi_pix_drawing_mode = pix_src | pix_dst;
	    break;
	case NOT:
	    _cgi_pix_drawing_mode = PIX_NOT(PIX_SRC);
	    /* wds: I choose to disregard (source == BITNOT) here. */
	    break;
	case XOR:
	    _cgi_pix_drawing_mode = pix_src ^ pix_dst;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_drawing_mode					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          inquire_drawing_mode(visibility, source, destination, combination)
Cbmode         *visibility;	/* transparent or opaque */
Cbitmaptype    *source;		/* NOT source bits */
Cbitmaptype    *destination;	/* NOT destination bits */
Ccombtype      *combination;	/* combination rules */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	*combination = _cgi_bit_drawing_mode;
	*visibility = _cgi_bit_vis_mode;
	*source = _cgi_bit_source;
	*destination = _cgi_bit_dest;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_device_bitmap (name)				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cpixrect       *inquire_device_bitmap(name)
Cint            name;
{
    int             err;

    err = _cgi_context(name);
    if (!err)
	return (_cgi_vws->sunview.pw->pw_pixrect);
    else
	return (0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_bitblt_alignments 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          inquire_bitblt_alignments(base, width, px, py, maxpx, maxpy, name)
Cint           *base;		/* bitmap base alignment */
Cint           *width;		/* width alignment */
Cint           *px, *py;	/* pattern extent alignment */
Cint           *maxpx, *maxpy;	/* maximum pattern size */
Cint            name;
{
    int             err;

    err = _cgi_err_check_4();
    err = _cgi_context(name);
    if (!err)
    {
	if (!err)
	{
	    *base = 0;
	    *width = 0;
	    *px = 0;
	    *py = 0;
	    *maxpx = _cgi_vws->vport.r_width;
	    *maxpy = _cgi_vws->vport.r_height;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_make_1_deep					    */
/*                                                                          */
/*		Generates tile-size 1-bit-deep mask (stencil) pixrect	    */
/*		from any non-depth-1 pattern tile.			    */
/*		Treats any non-0 pattern as single-bit 1 pixel.		    */
/*		Memory pixrect is assumed to be all zeros from creation.    */
/*                                                                          */
/****************************************************************************/
static int      _cgi_make_1_deep(memrect, pixpat, px, py)
Cpixrect       *memrect;	/* stencil pixrect (px X py X 1-bit-deep) */
Cpixrect       *pixpat;		/* Pattern tile (px X py X n, n != 1) */
int             px, py;		/* Size of the pattern tile */
/* Note: memrect must already be mem_created, with all pixels zeroed. */
{
    register u_char *ptr8, *ptr1;	/* Pointers to pixels in tile, stencil */
    register short  i, j, bit_place;
    register u_char *stx8, *stx1;	/* Start of each ROW (in X) */
    register int    incx8, incx1;	/* Bytes in each ROW (in X) */

    if ((pixpat->pr_depth == 1) || (memrect->pr_depth != 1))
	return (1);		/* Arguments indicate improper use of this
				 * function */

    if ((pixpat->pr_width < px) || (pixpat->pr_height < py))
    {
	px = pixpat->pr_width;
	py = pixpat->pr_height;
    }
    stx8 = (u_char *) mprd8_addr(mpr_d(pixpat), 0, 0, pixpat->pr_depth);
    stx1 = (u_char *) mprd_addr(mpr_d(memrect), 0, 0);
    incx8 = mpr_mdlinebytes(pixpat);
    incx1 = mpr_mdlinebytes(memrect);

    for (i = 0; i < py; i++)	/* For each ROW in destination area */
    {
	ptr8 = stx8;
	ptr1 = stx1;
	bit_place = UCHAR_MSB;
	for (j = 0; j < px; j++)/* For each PIXEL in destination ROW */
	{			/* fill one row */
	    if (*ptr8++ != 0)
		*ptr1 |= bit_place;
#ifdef	i386
	    if ((bit_place <<= 1) == 0)
#else	!i386
	    if ((bit_place >>= 1) == 0)
#endif	i386
	    {
		bit_place = UCHAR_MSB;
		ptr1++;
	    }
	}
	stx8 += incx8;
	stx1 += incx1;
    }
    return (0);
}
