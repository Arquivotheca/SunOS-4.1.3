#ifndef lint
static char	sccsid[] = "@(#)patsubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Pattern fill functions
 * Nonstandard functionality added 850815 wds
 */

#include "cgipriv.h"
#include <stdio.h>

Cint           *_cgi_pattern_table[MAXNUMPATS];	/* pattern table */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
short           _cgi_pattern_with_fill_color = 0;

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pattern_with_fill_color				    */
/*                                                                          */
/*	Non-standard CGI routine to change the interpretation of PATTERNs   */
/*	This routine sets ON or OFF a flag which controls PATTERN fills.    */
/*                                                                          */
/*	If the flag is OFF (the default), PATTERN fills perform as the CGI  */
/*	standard says they should:  pattern values from the entry in the    */
/*	pattern table are used verbatim (though scaled and translated).	    */
/*                                                                          */
/*	If the flag is ON, PATTERN fills are performed with only two colors:*/
/*	Color zero (where pattern value is zero),			    */
/*	and the current fill color (where pattern value is nonzero).	    */
/*                                                                          */
/*	Note that HATCH fill is not affected by this attribute flag.	    */
/****************************************************************************/
Cerror          pattern_with_fill_color(flag)
Cflag           flag;
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	_cgi_pattern_with_fill_color = (short) flag;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pattern	 					    */
/*                                                                          */
/*	_cgi_pattern creates one copy of the pattern which is used for      */
/*	pattern and hatch fill. Hatch fill is not scalable whereas          */
/*	pattern fill is scalable					    */
/****************************************************************************/
struct pixrect *_cgi_pattern(index, flag)
short           index, flag;	/* pattern number */
{
    register int   *pdp;	/* pattern pointer walks thru pattern defn */
    register short  j;		/* count X up to mscale pixels in patrect */
    register short  xctr;	/* counter for X within m x n block */
    register short  mxctr;	/* counter for X in mscale x nscale pixrect */
    register short  i;		/* count Y up to nscale pixels in patrect */
    register short  yctr;	/* counter for Y within m x n block */
    register short  myctr;	/* counter for Y in mscale x nscale pixrect */

    struct pixrect *patrect;
    int             incx;	/* number of bytes per line of pixrect */
    u_char         *stx;	/* Start of pixrect line */
    u_short        *stxw;	/* 16-bit word pointer: start of line */
    int             incxw;	/* 16-bit words per line of pixrect */
#define WORD_SIZE	16	/* Number of bits in a u_short */

    int             m, n;
    int             mscale, nscale;
    int             xscale, yscale;
    int             xoff, yoff;
    int            *pattern_defn = _cgi_pattern_table[index];	/* wds added 850823 */

    _cgi_vws->sunview.depth = _cgi_vws->sunview.pw->pw_pixrect->pr_depth;
    m = pattern_defn[1];	/* pattern or hatch size */
    n = pattern_defn[2];
    if (!flag)
    {				/* HATCH -- no scaling required */
	patrect = mem_create(m, n, _cgi_vws->sunview.depth);	/* create memory pixrect */
#ifdef	i386
	mpr_d(patrect)->md_flags &= ~MP_I386;
#endif	i386
	incx = mpr_mdlinebytes(patrect);	/* Num bytes in pixrect row */
	pdp = &pattern_defn[3];	/* starting point (not selectable) */

	if (_cgi_vws->sunview.depth == 1)
	{			/* black and white HATCH -- no scaling */
	    /* pixrect pointer walks thru patrect pixrect */
	    register u_short *prpw;	/* PixRect Pointer walks by Words */
	    register short  bit;/* Bit within depth=1 pixrect word */

	    stxw = (u_short *) mprd_addr(mpr_d(patrect), 0, 0);	/* 1-bit deep */
	    incxw = incx / 2 + incx % 2;
	    for (i = 0; i < n; i++)
	    {
		prpw = stxw;
		bit = WORD_SIZE - 1;	/* count down from 15 to 0 */
		for (j = 0; j < m; j++)
		{		/* put "colors" in bits " */
		    /* if color is nonzero, it's black */
		    if (*pdp++ != 0)
			*prpw |= 1 << bit;	/* 1 << 15, ..., 1 << 0 */
		    if (--bit < 0)	/* Bump X bit-shift counter */
		    {		/* Bit count rollover */
			bit = WORD_SIZE - 1;	/* Leftmost bit of word  */
			prpw++;	/* Next word in pixrect */
		    }
		}
		stxw += incxw;
	    }
	}
	else
	{			/* color HATCH -- no scaling */
	    /* pixrect pointer walks thru patrect pixrect */
	    register u_char *prp;	/* PixRect Pointer walks (by bytes) */

	    stx = mprd8_addr(mpr_d(patrect), 0, 0, _cgi_vws->sunview.depth);
	    for (i = 0; i < n; i++)
	    {
		prp = stx;
		for (j = 0; j < m; j++)	/* fill row */
		{
		    *prp++ = *pdp++;
		}
		stx += incx;
	    }
	}
    }
    else
    {				/* PATTERN may be scaled */
	/* calculate size of individual element */
	_cgi_devscalen(_cgi_att->pattern.dx, xscale);
	_cgi_devscalen(_cgi_att->pattern.dy, yscale);
	if (xscale < 1)
	    xscale = 1;
	if (yscale < 1)
	    yscale = 1;
	mscale = xscale * m;	/* pattern size in pixels */
	nscale = yscale * n;
	/* wds 850812: arbitrarily "clipping" [mn]scale won't work. */
	patrect = mem_create(mscale, nscale, _cgi_vws->sunview.depth);
	if (patrect == (struct pixrect *) NULL)
	{
	    (void) _cgi_errhand(EMEMSPAC);
	    (void) fprintf(stderr, "Pattern scaling failed.  CGI cannot continue.\n");
	    return (0);
	}
	incx = mpr_mdlinebytes(patrect);	/* Num bytes in pixrect row */
	_cgi_devscale(_cgi_att->pattern.point->x,
		      _cgi_att->pattern.point->y,
		      xoff, yoff);
	xoff %= mscale;
	yoff %= nscale;
	pdp = &pattern_defn[3 + (xoff / xscale) + (yoff / yscale) * m];

	if (_cgi_vws->sunview.depth == 1)
	{			/* black and white PATTERN */
	    /* pixrect pointer walks thru patrect pixrect */
	    register u_short *prpw;	/* PixRect Pointer walks by Words */
	    register short  bit;/* Bit within depth=1 pixrect word */

	    xctr = (xoff % xscale);
	    yctr = (yoff % yscale);
	    mxctr = xoff;
	    myctr = yoff;
	    incxw = incx / 2 + incx % 2;	/* Round up: num of u_shorts */
	    stxw = (u_short *) mprd_addr(mpr_d(patrect), 0, 0);	/* 1-bit deep */

	    for (i = 1; i <= nscale;)
	    {
		prpw = (u_short *) stxw;
		bit = WORD_SIZE - 1;	/* count down from 15 to 0 */
		for (j = 1; j <= mscale; j++)
		{
		    if (*pdp != 0)
			*prpw |= 1 << bit;	/* 1 << 15, ..., 1 << 0 */

		    if (++xctr >= xscale)	/* Bump x block counter */
		    {
			pdp++;	/* Go to next block */
			xctr = 0;	/* Left edge of that block */
		    }
		    if (--bit < 0)	/* Bump X bit-shift counter */
		    {		/* Bit count rollover */
			bit = WORD_SIZE - 1;	/* Leftmost bit of word  */
			prpw++;	/* Next word in pixrect */
		    }

		    if (++mxctr >= mscale)	/* Bump x counter */
		    {		/* Registration-caused wrap. */
			pdp -= m;	/* Wrap to start of row. */
			mxctr = 0;	/* Left edge of pixrect  */
		    }
		    /* pdp == &pattern_defn[idx] */
		    /* (pdp - &pattern_defn[0]) == idx */
		}

		pdp += m;	/* Next, use next defn row */
		++i, ++yctr, ++myctr;	/* One more line done */
		stxw += incxw;	/* Start of next line to do */

		/* Bump i, y block counter and y counter for each line done */
		while ((i <= nscale) && (yctr < yscale) && (myctr < nscale))
		{
		    /* make next patrect line identical to the previous line */
		    /* bcopy FROM_BYTE  TO_BYTE LENGTH_IN_BYTES, not words */
		    bcopy((char *) (stxw - incxw), (char *) stxw, incx);
		    stxw += incxw;	/* Start of next line to do */
		    ++i, ++yctr, ++myctr;	/* One more line done */
		}

		if (yctr >= yscale)
		    yctr = 0;	/* Top of next block */
		if (myctr >= nscale)	/* Already bumped y counter */
		{
		    myctr = 0;	/* Top edge of pixrect  */
		    /* Top row, proper X */
		    pdp = &pattern_defn[3 + (xoff / xscale)];
		}
	    }
	}
	else
	{			/* color PATTERN */
	    /* pixrect pointer walks thru patrect pixrect */
	    register u_char *prp;	/* PixRect Pointer walks (by bytes) */

	    xctr = (xoff % xscale);
	    yctr = (yoff % yscale);
	    mxctr = xoff;
	    myctr = yoff;
	    stx = mprd8_addr(mpr_d(patrect), 0, 0, _cgi_vws->sunview.depth);

	    for (i = 1; i <= nscale;)
	    {
		prp = stx;

		/* Nonstandard functionality added 850815 wds */
		if (_cgi_pattern_with_fill_color)
		{
		    /* Nonzero pattern values rendered with fill color */
		    for (j = 1; j <= mscale; j++)
		    {
			*prp++ = (*pdp) ? _cgi_att->fill.color : 0;
			if (++xctr >= xscale)	/* Bump x block counter */
			{
			    pdp++;	/* Go to next block */
			    xctr = 0;	/* Left edge of that block */
			}
			if (++mxctr >= mscale)	/* Bump x counter */
			{	/* Registration-caused wrap. */
			    pdp -= m;	/* Wrap to start of row. */
			    mxctr = 0;	/* Left edge of pixrect  */
			}
			/* pdp == &pattern_defn[idx] */
			/* (pdp - &pattern_defn[0]) == idx */
		    }
		}
		else		/* ! _cgi_pattern_with_fill_color */
		{
		    /* pattern values are used verbatim */
		    for (j = 1; j <= mscale; j++)
		    {
			*prp++ = (*pdp);
			if (++xctr >= xscale)	/* Bump x block counter */
			{
			    pdp++;	/* Go to next block */
			    xctr = 0;	/* Left edge of that block */
			}
			if (++mxctr >= mscale)	/* Bump x counter */
			{	/* Registration-caused wrap. */
			    pdp -= m;	/* Wrap to start of row. */
			    mxctr = 0;	/* Left edge of pixrect  */
			}
		    }
		}
		/* Note if it is helpful: when here, (mxctr == xoff) */

		pdp += m;	/* Next, use next defn row */
		++i, ++yctr, ++myctr;	/* One more line done */
		stx += incx;	/* Start of next line to do */

		/* Bump i, y block counter and y counter for each line done */
		while ((i <= nscale) && (yctr < yscale) && (myctr < nscale))
		{
		    /* make next patrect line identical to the previous line */
		    /* bcopy FROM_BYTE  TO_BYTE LENGTH_IN_BYTES */
		    bcopy((char *) (stx - incx), (char *) stx, incx);
		    stx += incx;/* Start of next line to do */
		    ++i, ++yctr, ++myctr;	/* One more line done */
		}

		if (yctr >= yscale)
		    yctr = 0;	/* Top of next block */
		if (myctr >= nscale)	/* Already bumped y counter */
		{
		    myctr = 0;	/* Top edge of pixrect  */
		    /* Top row, proper X */
		    pdp = &pattern_defn[3 + (xoff / xscale)];
		}
	    }
	}
    }
/*      pw_write (_cgi_vws->sunview.pw, 0, 0, m, n, _cgi_pix_mode, patrect, 0, 0);  */
    return (patrect);
}
