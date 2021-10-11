#ifndef lint
static char	sccsid[] = "@(#)pixels.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Raster Output Primitives & Inquiry Primatives
 */

/*
cell_array
cgipw_cell_array
_cgi_cell_array
pixel_array
cgipw_pixel_array
_cgi_pixel_array
inquire_cell_array
inquire_pixel_array
*/

/*
bugs:
error numbers
cgipw
absolute value of pixel array args.
true parallelogram cell array
fake trig
new drawing home.
*/

#include "cgipriv.h"		/* defines types used in this file  */
#include <pixrect/pixrect_hs.h>
#include <math.h>

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
int             _cgi_errno = 0;	/* Gets pixel_array error 69s */

extern          pw_getcmsdata();
/* Args: Pixwin *pw; struct  colormapseg *cms; int     *planes; */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cell_array	 					    */
/*                                                                          */
/*	Points p,q,r define a parellogram with pq as the diagonal where	    */
/*	p is the lower left-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtual size of the array  which is   */
/*	mapped onto the area defined by p,q, and r.			    */
/****************************************************************************/

Cerror          cell_array(pcell, qcell, rcell, dx, dy, colorind)
Ccoor          *pcell, *qcell, *rcell;	/* corners of parallelogram */
Cint            dx, dy;		/* size of color array */
Cint           *colorind;	/* array of color values */
{
    int             ivw;
    Cerror          err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
	if (dx < 0 || dy < 0)
	    err = ECELLPOS;
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_cell_array(_cgi_vws, pcell, qcell, rcell, dx, dy, colorind);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_cell_array 					    */
/*                                                                          */
/*	Points p,q,r define a parellogram with pq as the diagonal where	    */
/*	p is the LOWER LEFT-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtual size of the array  which is   */
/*	mapped onto the area defined by p, q, and r.			    */
/*                                                                          */
/* 	All cells are the same size in DEVICE COORDINATES, not necessarily  */
/* 	VDC coors.  This algorithm DEVIATES from the latest CGI and GKS:    */
/* 	pixel CENTERS are not used to determine rendering; only integral    */
/* 	multiplication and skewing is performed -- not resampling.	    */
/****************************************************************************/
Cerror          cgipw_cell_array(desc, pcell, qcell, rcell, d1, d2, colorind)
Ccgiwin        *desc;
Ccoor          *pcell, *qcell, *rcell;	/* VDC corners of parallelogram */
Cint            d1, d2;		/* size of color array in first & second axes */
Cint           *colorind;	/* array of color values */
{
    SETUP_CGIWIN(desc);
    return (_cgi_cell_array(desc->vws, pcell, qcell, rcell, d1, d2, colorind));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_cell_array 					    */
/*                                                                          */
/*	Points p,q,r define a parellogram with pq as the diagonal where	    */
/*	p is the LOWER LEFT-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtual size of the array  which is   */
/*	mapped onto the area defined by p, q, and r.			    */
/*                                                                          */
/* 	All cells are the same size in DEVICE COORDINATES, not necessarily  */
/* 	VDC coors.  This algorithm DEVIATES from the latest CGI and GKS:    */
/* 	pixel CENTERS are not used to determine rendering; only integral    */
/* 	multiplication and skewing is performed -- not resampling.	    */
/****************************************************************************/
Cerror          _cgi_cell_array(vws, pcell, qcell, rcell, d1, d2, colorind)
View_surface   *vws;
Ccoor          *pcell, *qcell, *rcell;	/* VDC corners of parallelogram */
Cint            d1, d2;		/* size of color array in first & second axes */
Cint           *colorind;	/* array of color values */
{
    Ccoor           pc, qc, rc;	/* DC corners of parallelogram */
    short           xsign, ysign;
    Ccoor           inc1, inc2;	/* inc1 x&y is for d1 steps from P to R. inc2
				 * x&y is for d2 steps from R to Q. */
    register int    i, j;	/* Loop counters. */
    int             err = NO_ERROR;	/* initialize error condition */

    /* scale coordinates */
    _cgi_devscale(pcell->x, pcell->y, pc.x, pc.y);
    _cgi_devscale(qcell->x, qcell->y, qc.x, qc.y);
    _cgi_devscale(rcell->x, rcell->y, rc.x, rc.y);

    pw_lock(vws->sunview.pw, &vws->sunview.lock_rect);
    if ((qc.x == rc.x) && (pc.y == rc.y))
    {				/* Rect; P and R both top or bottom: draw
				 * aligned rectangles horiz'lly */
/* No attempt is made to "resample", finding elements over pixel-centers */
/* Merely replicate elements to identical little rectangles */
	register int    x, y, xsize, ysize;

	/* compute size of each rectangular element */
	xsign = (qc.x - pc.x) < 0 ? -1 : 1;	/* One pixel further toward Q
						 * in X */
	inc1.x = (qc.x - pc.x + xsign) / d1;	/* Final - Initial + One more */
	xsize = abs(inc1.x);
	ysign = (qc.y - pc.y) < 0 ? -1 : 1;	/* One pixel further toward Q
						 * in Y */
	inc2.y = (qc.y - pc.y + ysign) / d2;	/* Final - Initial + One more */
	ysize = abs(inc2.y);
	if (xsize == 0 || ysize == 0)
	{
	    err = 66;		/* error cell dimensions too small */
	    goto unlock_and_return;
	}

	if (ysign > 0)
	    y = pc.y;
	else
	    y = pc.y + inc2.y + 1;

	for (j = 0; j < d2; j++, y += inc2.y)
	{

	    if (xsign > 0)
		x = pc.x;
	    else
		x = pc.x + inc1.x + 1;

	    for (i = 0; i < d1; i++, x += inc1.x)
	    {
		pw_write(vws->sunview.pw, x, y, xsize, ysize,
			 PIX_COLOR(*colorind++) | _cgi_pix_mode, 0, 0, 0);
	    }
	}
    }
    else if ((pc.x == rc.x) && (qc.y == rc.y))
    {				/* Rect; P and R both left or right: draw
				 * aligned rectangles vertically */
/* No attempt is made to "resample", finding elements over pixel-centers */
/* Merely replicate elements to identical little rectangles */
	register int    x, y, xsize, ysize;

	/* compute size of each rectangular element */
	xsign = (qc.x - pc.x) < 0 ? -1 : 1;	/* One pixel further toward Q
						 * in X */
	inc2.x = (qc.x - pc.x + xsign) / d2;	/* Final - Initial + One more */
	xsize = abs(inc2.x);
	ysign = (qc.y - pc.y) < 0 ? -1 : 1;	/* One pixel further toward Q
						 * in Y */
	inc1.y = (qc.y - pc.y + ysign) / d1;	/* Final - Initial + One more */
	ysize = abs(inc1.y);
	if (xsize == 0 || ysize == 0)
	{
	    err = 66;		/* error cell dimensions too small */
	    goto unlock_and_return;
	}

	if (xsign > 0)
	    x = pc.x;
	else
	    x = pc.x + inc2.x + 1;


	for (i = 0; i < d2; i++, x += inc2.x)
	{
	    if (ysign > 0)
		y = pc.y;
	    else
		y = pc.y + inc1.y + 1;

	    for (j = 0; j < d1; j++, y += inc1.y)
	    {
		pw_write(vws->sunview.pw, x, y, xsize, ysize,
			 PIX_COLOR(*colorind++) | _cgi_pix_mode, 0, 0, 0);
	    }
	}
    }
    else
    {				/* draw using parallelogram */
/* Each cell element rendered as identical little parallelogram. */
/* No attempt is made to "resample", finding elements over pixel-centers */
	Ccoor           fourpts[4];
	int             np = 4;	/* number of points in polygon boundry */

/* element parallelogram sizes -- No correction made for fragmentation */
/* start incs as displacements between the array's corners */
	inc1.x = (rc.x - pc.x);
	xsign = (inc1.x < 0) ? -1 : (inc1.x > 0) ? 1 : 0;
	inc1.x = (inc1.x + xsign) / d1;

	inc1.y = (rc.y - pc.y);
	ysign = (inc1.y < 0) ? -1 : (inc1.y > 0) ? 1 : 0;
	inc1.y = (inc1.y + ysign) / d1;

	inc2.x = (qc.x - rc.x);
	xsign = (inc2.x < 0) ? -1 : (inc2.x > 0) ? 1 : 0;
	inc2.x = (inc2.x + xsign) / d2;

	inc2.y = (qc.y - rc.y);
	ysign = (inc2.y < 0) ? -1 : (inc2.y > 0) ? 1 : 0;
	inc2.y = (inc2.y + ysign) / d2;
/* each element has inc1 pixel displacements from P to R; inc2 from R to Q. */

	/* Correct polygon rendering that ignores rightmost & bottommost edges. */
	if ((qc.x - pc.x) < 0)
	    pc.x++;
	if ((qc.y - pc.y) < 0)
	    pc.y++;
	/*
	 * These lines don't completely fix the problem: moving P down & right
	 * does not necessarily cause the original P to be rendered:
	 * 
	 * Moving P to p doesn't necessarily cause P to be rendered. o*****+
	 * o****+ o***+ o**+ o*+ P+ p Pixels marked "+" are added to polygon,
	 * but not rendered (on right). All those pixels marked "o" (& P & p)
	 * are still on the bottom and will therefore still not be rendered.
	 */

	for (j = 0; j < d2; j++)
	{
/* fourpts[0] on P corner; [1] is R corner; [2] is Q corner; [3] other corner */
	    fourpts[0].x = pc.x;
	    fourpts[0].y = pc.y;
	    fourpts[1].x = fourpts[0].x + inc1.x;	/* R */
	    fourpts[1].y = fourpts[0].y + inc1.y;
	    fourpts[2].x = fourpts[1].x + inc2.x;	/* Q */
	    fourpts[2].y = fourpts[1].y + inc2.y;
	    fourpts[3].x = fourpts[0].x + inc2.x;	/* Other */
	    fourpts[3].y = fourpts[0].y + inc2.y;

	    for (i = 0; i < d1; i++)
	    {			/* draw inner d1 elements of cell "row" from P
				 * to R */
		pw_polygon_2(vws->sunview.pw, 0, 0, 1, &np,
			     (struct pr_pos *) fourpts,
			     PIX_COLOR(*colorind++) | _cgi_pix_mode,
			     (Pixrect *) 0, 0, 0);
		/* Move cell element parallelogram along d1 axis */
		fourpts[0] = fourpts[1];	/* [0](p) to [1](r) */
		fourpts[3] = fourpts[2];	/* [3](other) to [2](q) */
		fourpts[1].x = fourpts[0].x + inc1.x;	/* move R forward */
		fourpts[1].y = fourpts[0].y + inc1.y;
		fourpts[2].x = fourpts[3].x + inc1.x;	/* move Q forward */
		fourpts[2].y = fourpts[3].y + inc1.y;
	    }

	    /* draw outer d2 elements of cell "column", moving P towards Q */
	    pc.x += inc2.x;
	    pc.y += inc2.y;
	}
    }
unlock_and_return:
    pw_unlock(vws->sunview.pw);
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pixel_array	 					    */
/*                                                                          */
/*		draws array colorind starting at point pcell.		    */
/*              M and N define the size of the array.                       */
/****************************************************************************/

Cerror          pixel_array(pcell, m, n, colorind)
Ccoor          *pcell;		/* base of array in VDC space */
Cint            m, n;		/* dimensions of color array */
Cint           *colorind;	/* array of color values */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_pixel_array(_cgi_vws, pcell, m, n, colorind);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_pixel_array 					    */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_pixel_array(desc, pcell, m, n, colorind)
Ccgiwin        *desc;
Ccoor          *pcell;		/* base of array in VDC space */
Cint            m, n;		/* dimensions of color array */
Cint           *colorind;	/* array of color values */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_pixel_array(desc->vws, pcell, m, n, colorind);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pixel_array 					    */
/*                                                                          */
/*		draws array colorind starting at point pcell.		    */
/*              M and N define the size of the array.                       */
/*              uses _cgi_att, assumes context loaded			    */
/*                                                                          */
/*		Now returns error 69 if any portion of pixel array is	    */
/*		outside the window, but draws the pixel array ANYWAY.	    */
/****************************************************************************/

Cerror          _cgi_pixel_array(vws, pcell, m, n, colorind)
View_surface   *vws;
Ccoor          *pcell;		/* base of array in VDC space */
Cint            m, n;		/* dimensions of color array */
register Cint  *colorind;	/* array of color values */
{
    register int    i, j;
    int             x, y, incx, rop;
    u_char         *stx;
    short          *stx1;
    Pixrect        *mprect;
    short           jmod;

    _cgi_errno = NO_ERROR;
    _cgi_devscale(pcell->x, pcell->y, x, y);
    _cgi_errno = _cgi_err_check_69(x, y);
    if (!_cgi_errno)
	_cgi_errno = _cgi_err_check_69(x + m, y + n);
    /* WDS 860829: Draw (with clipping) the array ANYWAY -- even if hanging off */
    /* if (!err) */
    {
	mprect = mem_create(m, n, vws->sunview.depth);
	if (vws->sunview.depth == 1)
	{
	    register short *xp1;

	    stx1 = mprd_addr(mpr_d(mprect), 0, 0);
	    incx = (mpr_mdlinebytes(mprect) + 1) >> 1;	/* Words */
	    for (i = 0; i < n; i++)
	    {
		xp1 = stx1;
		for (j = 0; j < m; j++)
		{
		    jmod = j % 16;
		    if (jmod == 0 && j != 0)
			xp1++;
		    if (*colorind++ != 0)
#ifdef	i386
			*xp1 |= 0x8000 >> (15 - jmod);
#else	!i386
			*xp1 |= 1 << (15 - jmod);
#endif	i386
		}
		stx1 += incx;
	    }
	}
	else
	{
	    register u_char *xp;
	    stx = mprd8_addr(mpr_d(mprect), 0, 0, vws->sunview.depth);
	    incx = mpr_mdlinebytes(mprect);	/* Bytes */
	    for (i = 0; i < n; i++)
	    {
		xp = stx;
		for (j = 0; j < m; j++)
		{
		    *xp++ = *colorind++;
		}
		stx += incx;
	    }
	}
	if (_cgi_errno)
	    rop = _cgi_pix_mode & ~(PIX_DONTCLIP);	/* Turn CLIPPING ON */
	else
	    rop = _cgi_pix_mode;/* No error: leave CLIPPING alone */
	pw_write(vws->sunview.pw, x, y, m, n, rop, mprect, 0, 0);
	(void) pr_destroy(mprect);
    }
    return (_cgi_errno != 69 ? _cgi_errno : 0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_cell_array 					    */
/*                                                                          */
/*	Points p,q,r define a parellogram with pq as the diagonal where	    */
/*	p is the lower left-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtual size of the array  which is   */
/*	mapped onto the area defined by p,q, and r.			    */
/*                                                                          */
/*	Now allows P to be any corner: sampling is at corner of cell	    */
/*	element closest to point P, which is lower-left if P is lower-left. */
/*                                                                          */
/*	See discussion under cgipw_cell_array about the limitations caused  */
/*	by not really resampling.  Note that this function is not in the    */
/*	1986 CGI standard at all, since sampling is an application function.*/
/****************************************************************************/

inquire_cell_array(name, pcell, qcell, rcell, d1, d2, colorind)
Cint            name;
Ccoor          *pcell, *qcell, *rcell;	/* points of parallelogram */
Cint            d1, d2;		/* size of color array in first & second axes */
Cint           *colorind;	/* array of color values */
{
    Ccoor           pc, qc, rc;	/* DC corners of parallelogram */
    short           xsign, ysign;
    Ccoor           inc1, inc2;	/* inc1 x&y is for d1 steps from P to R.                                         *
				 * inc2 x&y is for d2 steps from R to Q.                                         */
    register int    i, j;	/* Loop counters. */
    register int    x, y;	/* Loop counters. */
    int             err = 0;	/* initialize error condition */

    err = _cgi_context(name);
    if (!err)
	if (d1 < 0 || d2 < 0)
	    err = 67;
    if (!err)
    {
	_cgi_devscale(pcell->x, pcell->y, pc.x, pc.y);
	_cgi_devscale(qcell->x, qcell->y, qc.x, qc.y);
	_cgi_devscale(rcell->x, rcell->y, rc.x, rc.y);
/* element parallelogram sizes -- No correction made for fragmentation */
/* start incs as displacements between the array's corners */
	inc1.x = (rc.x - pc.x);
	xsign = (inc1.x < 0) ? -1 : (inc1.x > 0) ? 1 : 0;
	inc1.x = (inc1.x + xsign) / d1;

	inc1.y = (rc.y - pc.y);
	ysign = (inc1.y < 0) ? -1 : (inc1.y > 0) ? 1 : 0;
	inc1.y = (inc1.y + ysign) / d1;

	inc2.x = (qc.x - rc.x);
	xsign = (inc2.x < 0) ? -1 : (inc2.x > 0) ? 1 : 0;
	inc2.x = (inc2.x + xsign) / d2;

	inc2.y = (qc.y - rc.y);
	ysign = (inc2.y < 0) ? -1 : (inc2.y > 0) ? 1 : 0;
	inc2.y = (inc2.y + ysign) / d2;
/* each element has inc1 pixel displacements from P to R; inc2 from R to Q. */

	/* error cell dimensions too small */
	if ((inc1.x == 0 && inc1.y == 0) || (inc2.x == 0 && inc2.y == 0))
	    err = 66;		/* error cell dimensions too small */
	if (!err)
	{
	    pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
	    for (j = 0; j < d2; j++)
	    {
		x = pc.x;
		y = pc.y;	/* Start at P */
		for (i = 0; i < d1; i++)
		{
		    *colorind++ = pw_get(_cgi_vws->sunview.pw, x, y);
		    x += inc1.x;
		    y += inc1.y;/* Move towards R */
		}
		pc.x += inc2.x;
		pc.y += inc2.y;	/* Move P towards Q */
	    }
	    pw_unlock(_cgi_vws->sunview.pw);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_pixel_array	 				    */
/*                                                                          */
/*		reads pixel array into array colorind,                      */
/*		starting at point pcell.				    */
/*              M and N define the size of the array.                       */
/****************************************************************************/

inquire_pixel_array(pcell, m, n, colorind, name)
Ccoor          *pcell;		/* base of array in VDC space */
Cint            m, n;		/* dimensions of color array */
Cint           *colorind;	/* array of color values */
Cint            name;
{
    int             x, y;
    Pixrect        *mprect;

    _cgi_errno = _cgi_context(name);
    if (!_cgi_errno)
    {
	_cgi_devscale(pcell->x, pcell->y, x, y);	/* start point */
	_cgi_errno = _cgi_err_check_69(x, y);
	if (!_cgi_errno)
	    _cgi_errno = _cgi_err_check_69(x + m, y + n);
	/*
	 * WDS 860829: Draw (with clipping) the array ANYWAY -- even if hanging
	 * off
	 */
	/* if (!err) */
	{
	    mprect = mem_create(m, n, _cgi_vws->sunview.depth);
	    pw_read(mprect, 0, 0, m, n, PIX_SRC, _cgi_vws->sunview.pw, x, y);

	    if (_cgi_vws->sunview.depth == 1)
	    {
#define WORD_SIZE	16	/* Number of bits in a u_short */
		register u_short *prpw;	/* PixRect Pointer walks by Words */
		register short  bit;	/* Bit within depth=1 pixrect word */
		u_short        *stxw;	/* 16-bit word pointer: start of line */
		int             incxw;	/* 16-bit words per line of pixrect */
		register int    i, j;	/* Count Y up to n, X up to m */

		stxw = (u_short *) mprd_addr(mpr_d(mprect), 0, 0);	/* 1 bit deep */
		incxw = (mpr_mdlinebytes(mprect) + 1) >> 1;	/* Words */
		for (i = 0; i < n; i++)
		{
		    prpw = stxw;
		    bit = WORD_SIZE - 1;	/* Count down from 15 to 0 */
		    for (j = 0; j < m; j++)
		    {		/* Color values returned necessarily either 1
				 * or 0. */
			*colorind++ = (*prpw & (1 << bit)) ? 1 : 0;
			if (--bit < 0)	/* Bump X bit-shift counter */
			{	/* Bit count rollover */
			    bit = WORD_SIZE - 1;	/* Leftmost bit of word */
			    prpw++;	/* Move on to next word */
			}
		    }
		    stxw += incxw;
		}
	    }
	    else		/* Non depth 1 pixrect: assume 8 */
	    {
		register u_char *prp;	/* PixRect Pointer walks by Bytes */
		u_char         *stx;	/* byte pointer: start of line */
		int             incx;	/* bytes per line of pixrect */
		register int    i, j;	/* Count Y up to n, X up to m */
		register unsigned colormap_segment_mask;
		/*
		 * Frame buffer's high-order bits are set to the address of the
		 * colormap segment in use.  The low-order bits are application
		 * data. We will have to mask off the high-order bits so the
		 * application will see the low-order bits it but there.
		 */
		{
#define	NO_PLANE_PTR	((int *)0)
		    struct colormapseg cms;
		    int             test = 1;
		    pw_getcmsdata(_cgi_vws->sunview.pw, &cms, NO_PLANE_PTR);
		    /* cms.cms_size is the colormap segment size of this window */
		    while (test < cms.cms_size)
			test <<= 1;
		    /*
		     * test is a power of two (a single bit) >= cms.cms_size,
		     * e.g., 8
		     */
		    colormap_segment_mask = test - 1;	/* e.g., 7 = bin 0111 */
		}


		stx = mprd8_addr(mpr_d(mprect), 0, 0, _cgi_vws->sunview.depth);
		incx = mpr_mdlinebytes(mprect);
		for (i = 0; i < n; i++)
		{
		    prp = stx;
		    for (j = 0; j < m; j++)	/* Process one row */
		    {
			*colorind++ = (*prp++) & colormap_segment_mask;
			/*
			 * high-order bits depends on which colormap segment is
			 * used.
			 */
		    }
		    stx += incx;
		}
	    }
	    (void) pr_destroy(mprect);
	}			/* if (!err) from _cgi_err_check_69 */
    }				/* if (!err) from _cgi_context */
    return (_cgi_errhand(_cgi_errno != 69 ? _cgi_errno : 0));
}
