#ifndef lint
static char     sccsid[] = "@(#)circfast.c 1.1 92/07/30 ";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
/*
 * CGI circular arc  functions
 */

/*
_cgi_fill_partial_pattern_circle
_cgi_textured_arc
_cgi_next_quad
_cgi_textured_quad
_cgi_new_arc_pts
*/
/*
bugs:
"off by one error" between quadrant definition and pr_curve semantics
floats
*/

/*
 *	**** THE ROUTINES IN THIS FILE ARE NO LONGER USED ****
 *
 * The special-case code for width 1 circular arcs was disabled to fix two
 * problems:  1) Bug 1002999, arc segments of width 1 were drawn in the
 * wrong direction (ie. from point 2 to point 1, rather than from point 1
 * to point 2), and 2) pr_curve() was not ported for the SPARC architecture,
 * so SYS4/3.4 needed this change to not use the stuff in circfast.c, which
 * used pr_curve.
 *      WCL     2/5/87
 */
#ifdef	DAVID_REMOVED_870205

#include "cgipriv.h"		/* defines types used in this file  */
#include "chain.h"
#ifndef window_hs_DEFINED
#include <sunwindow/window_hs.h>
#endif
View_surface   *_cgi_vws;	/* current view surface */
#include <math.h>
int             _cgi_pix_mode;
#ifdef	cgi_before_cleanup_850723
static int      ptc, ipat, convf, oconvf, angsum, angsum1;
#else	cgi_before_cleanup_850723
static int      ptc, ipat, oconvf, angsum, angsum1;
#endif	cgi_before_cleanup_850723
/* wds Exchanged for the copy without the "static" 850712 */
#ifdef  cgi_before_850405
int             cut;
int             ptc, ipat, convf, oconvf, angsum, angsum1;
/* "_cgi_abs" changed to use math-library's "abs" also */
#endif  cgi_before_850405
static int      pattern2a[6][9] =
{
 {
  180, 46, 0, 0, 0, 0, 0, 0, 0
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
/*     FUNCTION: _cgi_check_new_quad					    */
/*                                                                          */
/*            uses flipped definition of quadrants                          */
/****************************************************************************/

int             _cgi_check_new_quad(x1, y1, x2, y2)
int             x1, y1, x2, y2;
{
    int             yes;
    if (x2 > x1 && y2 <= y1)
	yes = 4;
    else
    {
	if (x2 >= x1 && y2 > y1)
	    yes = 1;
	else
	{
	    if (x2 < x1 && y2 >= y1)
		yes = 2;
	    else
	    {
		if (x2 <= x1 && y2 < y1)
		    yes = 3;
	    }
	}
    }
    return (yes);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_textured_circle				    */
/*                                                                          */
/*		draws circle with coors in screen space			    */
/****************************************************************************/
int             _cgi_textured_arc(x0, y0, x1, y1, x2, y2, rad, width, val, style)
short           x0, y0;
int             rad;
int             width;
short           val;
Clintype        style;
int             x1, y1, x2, y2;
{
    static int      xb, yb, xe, ye, next_quad, cur_quad;
    /* compare quadrants of starting and ending points */
    if (style != SOLID)
    {
	oconvf = _cgi_oct_circ_radius(rad + width);
	ipat = 0;
	angsum1 = angsum = (pattern2a[(int) style][0] << 10) / 2;
	ptc = pattern2a[(int) style][0] / 2;
    }
#ifdef cgi_before_wds_850225
    if (_cgi_check_new_quad(x0, y0, x1, y1) == _cgi_check_new_quad(x0, y0, x2, y2))
    {
	_cgi_textured_quad(x0, y0, x1, y1, x2, y2, rad, width,
			   val, style, next_quad);
    }
    else
#else
    cur_quad = next_quad = _cgi_check_new_quad(x0, y0, x1, y1);
    /* Before drawing clockwise, Check for clockwise in same quadrant */
    _cgi_next_quad(&next_quad, x0, y0, x1, y1, x2, y2, rad, &xe, &ye);
#endif cgi_before_wds_850225
    {				/* WDS */
#ifdef cgi_before_wds_850225
	next_quad = _cgi_check_new_quad(x0, y0, x1, y1);
	cur_quad = next_quad;
	_cgi_next_quad(&next_quad, x0, y0, x1, y1, x2, y2, rad, &xe, &ye);
	_cgi_textured_quad(x0, y0, x1, y1, xe, ye, rad, width,
			   val, style, cur_quad);
	xb = xe;
	yb = ye;
#else
	_cgi_textured_quad(x0, y0, x1, y1, xe, ye, rad, width,
			   val, style, cur_quad);
#endif cgi_before_wds_850225
	while (next_quad)
	{
	    xb = xe;
	    yb = ye;
	    cur_quad = next_quad;
	    _cgi_next_quad(&next_quad, x0, y0, xb, yb, x2, y2, rad, &xe, &ye);
	    _cgi_textured_quad(x0, y0, xb, yb, xe, ye, rad, width,
			       val, style, cur_quad);
	}
    }				/* WDS */
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_next_quad                                             */
/*                                                                          */
/*          Finds next quadrant in circular arc and endpoints               */
/*	in arc.                                                         */
/*                                                                          */
/****************************************************************************/
/* warning from wds 850220: x1 & y1 are unused */
int             _cgi_next_quad(cur_quad, x0, y0, x1, y1, x2, y2, rad, xe, ye)
int            *cur_quad, x0, y0, x1, y1, x2, y2, rad, *xe, *ye;
{
    /* find next quadrant */
    if ((*cur_quad == _cgi_check_new_quad(x0, y0, x2, y2))
	&& (((y1 - y0 >= 0) && (x1 >= x2))	/* clockwise in Q1 or 2 */
	    || ((y1 - y0 <= 0) && (x1 <= x2))	/* clockwise in Q3 or 4 */
	    )
	)
    {
	*cur_quad = 0;
	*xe = x2;
	*ye = y2;
    }
    else
    {
	switch (*cur_quad)
	{
	case 1:
	    *cur_quad = 2;
	    *xe = x0;
	    *ye = y0 + rad + 1;
	    break;
	case 2:
	    *cur_quad = 3;
	    *xe = x0 - (rad + 1);
	    *ye = y0;
	    break;
	case 3:
	    *cur_quad = 4;
	    *xe = x0;
	    *ye = y0 - (rad + 1);
	    break;
	case 4:
	    *cur_quad = 1;
	    *xe = x0 + rad + 1;
	    *ye = y0;
	    break;
	}
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_partial_textured_quad				    */
/*                                                                          */
/****************************************************************************/
/* warning from wds 850220: xe is unused */
#ifdef cgi_wds_removed_850220
int             _cgi_textured_quad(x0, y0, xb, yb, xe, ye, rad, width, val, style, iquad)
#else
_cgi_textured_quad(x0, y0, xb, yb, xe, ye, rad, width, val, style, iquad)
#endif cgi_wds_removed_850220
short           x0, y0;
int             rad;
int             width;
short           val;
Clintype        style;
int             xb, yb, xe, ye;
int             iquad;
{
    register int    x, y;
#ifdef cgi_wds_removed_850220
    int             x1, y1, d, d1;
    int             i, /* k, temp, */ totx;
#endif cgi_wds_removed_850220
    int /* rad1, */ rad2;
    int /* f, f1, */ cnt, numf;	/* note! comments below about f  */
    register short  chainc;
    static struct pr_curve bchain[200] /* , tchain[200] */ ;
    int             oldx, oldy, end_oct_xmajor;

    rad = rad + width;
    if (style == SOLID)
    {
	cnt = 1;
	chainc = 0;
	numf = 0;
	rad2 = rad * rad;
	switch (iquad)
	{
	case 1:
#ifdef cgi_wds_removed_850220
/*		    oldx = rad;
		    oldy = 0;
		    x = rad;
*/
#endif cgi_wds_removed_850220
	    oldx = x = xb - x0;
#ifdef cgi_before_wds_fixed_850319
	    oldy = yb - y0;
	    for (y = yb - y0; y <= ye - y0; y++)
#else
	    oldy = y = yb - y0;
#ifndef cgi_before_wds_850319
	    end_oct_xmajor = (xe - x0) <= (ye - y0);
#endif cgi_before_wds_850319
	    /* Moved y++ and x-- right before adding to chainc */
	    while ((!end_oct_xmajor) && (y < ye - y0)
		   || (end_oct_xmajor)	/* && (x > xe - x0) */
		)
#endif cgi_before_wds_fixed_850319
	    {
		while (abs((rad2) - ((x - 1) * (x - 1) + y * y)) <
		       abs((rad2) - ((y + 1) * (y + 1) + x * x)))
		{		/* bump x until error is positive */
		    x--;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
#ifdef cgi_before_wds_fixed_850319
		    if (x <= 0)
			goto endfor1;
#else
		    if ((end_oct_xmajor) && (x <= xe - x0))
			goto endfor1;
#endif cgi_before_wds_fixed_850319
		}
#ifdef cgi_before_wds_fixed_850226
/* 			if(cnt == 2 && numf && (bchain[numf-1].bits & 1)) { */
		/* wds added } for vi % function */
		/* kludge for 45  trash this */
		 /* B */ if (x == y)
		{
		    bchain[numf].x = -1;
		    bchain[numf].y = 0;
		    bchain[numf].bits = 1 << 15;
		    oldx++;
		    numf++;
		}
		else
		    chainc |= 0 << 16 - cnt;
/* e */
#endif cgi_before_wds_fixed_850226
		y++;
		cnt++;
#ifdef cgi_before_wds_850319
		chainc |= 0 << 16 - cnt;
#endif cgi_before_wds_850319
		if (cnt >= 16)
		{
		    cnt = 1;
		    bchain[numf].x = x - oldx;
		    bchain[numf].y = y - oldy;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    numf++;
		    chainc = 0;
		    oldx = x;
		    oldy = y;
		}
	    }			/* end for y */
    endfor1:
	    if (cnt > 1)
	    {
		bchain[numf].x = x - oldx;
		bchain[numf].y = y - oldy;
		bchain[numf].bits = chainc;
		numf++;
	    }
#ifdef cgi_before_wds_fixed_850226
	    if (x < 0)
	    {
		while (x < -15)
		{
		    numf--;
		    x += 15;
		}
		totx = 0;
		for (i = 0; i <= numf; i++)
		{
		    totx -= bchain[i].x;
		}
		bchain[numf].x += (totx - (xb - x0));
	    }
#endif cgi_before_wds_fixed_850226
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	case 2:
#ifdef cgi_before_wds_fixed_850226
	    oldx = 0;
	    x = x0 - xb;
	    oldy = yb - y0;
	    for (y = yb - y0; y >= ye - y0; y--)
#else
	    /* Changed sign of initial x to nonnegative! */
	    oldx = x = x0 - xb;
	    oldy = y = yb - y0;
#ifndef cgi_before_wds_850319
	    end_oct_xmajor = (x0 - xe) <= (ye - y0);
#endif cgi_before_wds_850319
	    /* Moved y-- and x++ right before adding to chainc */
	    while ((end_oct_xmajor)	/* && (x < x0 - xe) */
		   || (!end_oct_xmajor) && (y > ye - y0)
		)
#endif cgi_before_wds_fixed_850226
	    {
		while (abs((rad2) - ((x + 1) * (x + 1) + y * y)) <
		       abs((rad2) - ((y - 1) * (y - 1) + x * x)))
		{
		    x++;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
		}
#ifndef cgi_before_wds_fixed_850319
		if ((end_oct_xmajor) && (x >= x0 - xe))
		    goto endfor2;
#endif cgi_before_wds_fixed_850319
#ifdef cgi_before_wds_fixed_850226
/*			if(cnt == 2 && numf && (bchain[numf-1].bits & 1)) {
				bchain[numf].x = -1;
				bchain[numf].y = 0;
				bchain[numf].bits = 1<<14;
				oldx++;
				numf++;
			}
			else */
#endif cgi_before_wds_fixed_850226
		y--;
		cnt++;
#ifdef cgi_before_wds_850319
		chainc |= 0 << 16 - cnt;
#endif cgi_before_wds_850319
		if (cnt >= 16)
		{
		    cnt = 1;
		    bchain[numf].x = oldx - x;
		    bchain[numf].y = y - oldy;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    numf++;
		    chainc = 0;
		    oldx = x;
		    oldy = y;
		}
	    }			/* end for y */
    endfor2:
	    if (cnt > 1)
	    {
		bchain[numf].x = oldx - x;
		bchain[numf].y = y - oldy;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	case 3:
#ifdef cgi_wds_removed_850220
/*		    oldx = rad;
		    oldy = 0;
		    x = rad; */
#endif cgi_wds_removed_850220
	    oldx = x = x0 - xb;
#ifdef cgi_before_wds_fixed_850226
	    oldy = y0 - yb;
/* 		    for (y = 0; y <= rad; y++) */
	    for (y = y0 - yb; y <= y0 - ye; y++)
#else
	    oldy = y = y0 - yb;
#ifndef cgi_before_wds_850319
	    end_oct_xmajor = (x0 - xe) <= (y0 - ye);
#endif cgi_before_wds_850319
	    /* Moved y++ and x-- right before adding to chainc */
	    while ((!end_oct_xmajor) && (y < y0 - ye)
		   || (end_oct_xmajor)	/* && (x > x0 - xe) */
		)
#endif cgi_before_wds_fixed_850226
	    {
		while (abs((rad2) - ((x - 1) * (x - 1) + y * y)) <
		       abs((rad2) - ((y + 1) * (y + 1) + x * x)))
		{		/* increment f until error is positive */
		    x--;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
#ifdef cgi_before_wds_fixed_850319
		    if (x <= 0)
			goto endfor3;
#else
		    if ((end_oct_xmajor) && (x <= x0 - xe))
			goto endfor3;
#endif cgi_before_wds_fixed_850319
		}
#ifdef cgi_wds_removed_850220
/*			if(cnt == 2 && numf && (bchain[numf-1].bits & 1)) {
				bchain[numf].x = 0;
				bchain[numf].y = 1;
				bchain[numf].bits = 1<<15;
				oldx++;
				numf++;
			}
			else */
#endif cgi_wds_removed_850220
		y++;
		cnt++;
#ifdef cgi_before_wds_850319
		chainc |= 0 << 16 - cnt;
#endif cgi_before_wds_850319
		if (cnt >= 16)
		{
		    cnt = 1;
		    bchain[numf].x = oldx - x;
		    bchain[numf].y = oldy - y;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    numf++;
		    chainc = 0;
		    oldx = x;
		    oldy = y;
		}
	    }			/* end for y */
    endfor3:
	    if (cnt > 1)
	    {
		bchain[numf].x = oldx - x;
		bchain[numf].y = oldy - y;
		bchain[numf].bits = chainc;
		numf++;
	    }
#ifdef cgi_before_wds_fixed_850226
	    if (x < 0)
	    {
		while (x < -15)
		{
		    numf--;
		    x += 15;
		}
		totx = 0;
		for (i = 0; i <= numf; i++)
		{
		    totx += bchain[i].x;
		}
		bchain[numf].x -= (totx - (x0 - xb));
	    }
#endif cgi_before_wds_fixed_850226
	    pw_curve(_cgi_vws->sunview.pw, xb, yb,
		     bchain, numf, _cgi_pix_mode, val);
	    break;
	case 4:
#ifdef cgi_wds_removed_850220
/*		    oldx = 0;
		    oldy = rad;
		    x = 0;
		    */
#endif cgi_wds_removed_850220
	    oldx = x = xb - x0;
#ifdef cgi_before_wds_fixed_850226
	    oldy = y0 - yb;
	    for (y = y0 - yb; y >= y0 - ye; y--)
#else
	    oldy = y = y0 - yb;
#ifndef cgi_before_wds_850319
	    end_oct_xmajor = (xe - x0) <= (y0 - ye);
#endif cgi_before_wds_850319
	    /* Moved y-- and x++ right before adding to chainc */
	    while ((end_oct_xmajor)	/* && (x < xe - x0) */
		   || (!end_oct_xmajor) && (y > y0 - ye)
		)
#endif cgi_before_wds_fixed_850226
	    {
		while (abs((rad2) - ((x + 1) * (x + 1) + y * y)) <
		       abs((rad2) - ((y - 1) * (y - 1) + x * x)))
		{		/* increment f until error is positive */
		    x++;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
#ifndef cgi_before_wds_fixed_850319
		    if ((end_oct_xmajor) && (x >= xe - x0))
			goto endfor4;
#endif cgi_before_wds_fixed_850319
		}
#ifdef cgi_wds_removed_850220
/*			if(cnt == 2) {
			    if( numf && (bchain[numf-1].bits & 1)) {
				bchain[numf].x = 1;
				bchain[numf].y = 0;
				bchain[numf].bits = 1<<14;
				oldx++;
				numf++;
			    }
			}
			else { */
/*  			}  */
#endif cgi_wds_removed_850220
		y--;
		cnt++;
#ifdef cgi_before_wds_850319
		chainc |= 0 << 16 - cnt;
#endif cgi_before_wds_850319
		if (cnt >= 16)
		{
		    cnt = 1;
		    bchain[numf].x = x - oldx;
		    bchain[numf].y = oldy - y;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    numf++;
		    chainc = 0;
		    oldx = x;
		    oldy = y;
		}
	    }			/* end for y */
    endfor4:
	    if (cnt > 1)
	    {
		bchain[numf].x = x - oldx;
		bchain[numf].y = oldy - y;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	}			/* end switch (iquad) */
    }				/* end (style == SOLID) */
    else
    {
	cnt = 1;
	chainc = 0;
	numf = 0;
	rad2 = rad * rad;
	switch (iquad)
	{
	case 1:
	    end_oct_xmajor = (xe - x0) <= (ye - y0);
	    oldx = x = xb - x0;
	    oldy = y = yb - y0;
	    /* Moved y++ and x-- right before adding to chainc */
	    while ((!end_oct_xmajor) && (y < ye - y0)
		   || (end_oct_xmajor)	/* && (x > xe - x0) */
		)
	    {
		while (abs((rad2) - ((x - 1) * (x - 1) + y * y)) <
		       abs((rad2) - ((y + 1) * (y + 1) + x * x)))
		{		/* increment f until error is positive */
		    x--;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (ptc > pattern2a[(int) style][ipat])
		    {
			ipat++;
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			if ((ipat % 2) == 0)
			    chainc = 0;
			else
			    chainc = 1 << 15;
			numf++;
			oldx = x;
			oldy = y;
			ptc = 0;
			ipat &= 7;
		    }
		    else
		    {
			ptc++;
			if (cnt >= 16)
			{
			    cnt = 1;
			    bchain[numf].x = x - oldx;
			    bchain[numf].y = y - oldy;
			    bchain[numf].bits = chainc;
			    /* move to next word */
			    chainc = 0;
			    numf++;
			    oldx = x;
			    oldy = y;
			}
		    }
		    if ((end_oct_xmajor) && (x <= xe - x0))
			goto endpat1;
		}		/* end increment f until error is positive */
		y++;
		cnt++;
		/* chainc |= 0 << 16 - cnt; */
		if (ptc > pattern2a[(int) style][ipat])
		{
		    ipat++;
		    cnt = 1;
		    bchain[numf].x = x - oldx;
		    bchain[numf].y = y - oldy;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    if ((ipat % 2) == 0)
			chainc = 0;
		    else
			chainc = 1 << 15;
		    numf++;
		    oldx = x;
		    oldy = y;
		    ptc = 0;
		    ipat &= 7;
		}
		else
		{
		    ptc++;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
		}
	    }			/* end for y */
    endpat1:
	    if (cnt > 1)
	    {
		bchain[numf].x = x - oldx;
		bchain[numf].y = y - oldy;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	case 2:
	    end_oct_xmajor = (x0 - xe) <= (ye - y0);
	    oldx = x = x0 - xb;	/* wds reversed 850321 */
	    oldy = y = yb - y0;
	    /* Moved y-- and x++ right before adding to chainc */
	    while ((end_oct_xmajor)	/* && (x < x0 - xe) */
		   || (!end_oct_xmajor) && (y > ye - y0)
		)
	    {
		while (abs((rad2) - ((x + 1) * (x + 1) + y * y)) <
		       abs((rad2) - ((y - 1) * (y - 1) + x * x)))
		{
		    x++;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (ptc > pattern2a[(int) style][ipat])
		    {
			ipat++;
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			if ((ipat % 2) == 0)
			    chainc = 0;
			else
			    chainc = 1 << 15;
			numf++;
			oldx = x;
			oldy = y;
			ptc = 0;
			ipat &= 7;
		    }
		    else
		    {
			ptc++;
			if (cnt >= 16)
			{
			    cnt = 1;
			    bchain[numf].x = oldx - x;
			    bchain[numf].y = y - oldy;
			    bchain[numf].bits = chainc;
			    /* move to next word */
			    chainc = 0;
			    numf++;
			    oldx = x;
			    oldy = y;
			}
		    }
		    if ((end_oct_xmajor) && (x >= x0 - xe))
			goto endpat2;
		}
		y--;
		cnt++;
		/* chainc |= 0 << 16 - cnt; */
		if (ptc > pattern2a[(int) style][ipat])
		{
		    ipat++;
		    cnt = 1;
		    bchain[numf].x = oldx - x;
		    bchain[numf].y = y - oldy;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    if ((ipat % 2) == 0)
			chainc = 0;
		    else
			chainc = 1 << 15;
		    numf++;
		    oldx = x;
		    oldy = y;
		    ptc = 0;
		    ipat &= 7;
		}
		else
		{
		    ptc++;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = y - oldy;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
		}
	    }			/* end for y */
    endpat2:
	    if (cnt > 1)
	    {
		bchain[numf].x = oldx - x;
		bchain[numf].y = y - oldy;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	case 3:
	    end_oct_xmajor = (x0 - xe) <= (y0 - ye);
	    oldx = x = x0 - xb;
	    oldy = y = y0 - yb;
	    /* Moved y++ and x-- right before adding to chainc */
	    while ((!end_oct_xmajor) && (y < y0 - ye)
		   || (end_oct_xmajor)	/* && (x > x0 - xe) */
		)
	    {
		while (abs((rad2) - ((x - 1) * (x - 1) + y * y)) <
		       abs((rad2) - ((y + 1) * (y + 1) + x * x)))
		{		/* increment f until error is positive */
		    x--;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    /* if (x < 0) goto label2; */
		    if (ptc > pattern2a[(int) style][ipat])
		    {
			ipat++;
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			if ((ipat % 2) == 0)
			    chainc = 0;
			else
			    chainc = 1 << 15;
			numf++;
			oldx = x;
			oldy = y;
			ptc = 0;
			ipat &= 7;
		    }
		    else
		    {
			ptc++;
			if (cnt >= 16)
			{
			    cnt = 1;
			    bchain[numf].x = oldx - x;
			    bchain[numf].y = oldy - y;
			    bchain[numf].bits = chainc;
			    /* move to next word */
			    chainc = 0;
			    numf++;
			    oldx = x;
			    oldy = y;
			}
		    }
		    if ((end_oct_xmajor) && (x <= x0 - xe))
			goto endpat3;
		}
		y++;
		cnt++;
		/* chainc |= 0 << 16 - cnt; */
		if (ptc > pattern2a[(int) style][ipat])
		{
		    ipat++;
		    cnt = 1;
		    bchain[numf].x = oldx - x;
		    bchain[numf].y = oldy - y;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    if ((ipat % 2) == 0)
			chainc = 0;
		    else
			chainc = 1 << 15;
		    numf++;
		    oldx = x;
		    oldy = y;
		    ptc = 0;
		    ipat &= 7;
		}
		else
		{
		    ptc++;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = oldx - x;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
		}
	    }
    endpat3:
	    if (cnt > 1)
	    {
		bchain[numf].x = oldx - x;
		bchain[numf].y = oldy - y;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw, xb, yb,
		     bchain, numf, _cgi_pix_mode, val);
	    break;
	case 4:
	    end_oct_xmajor = (xe - x0) <= (y0 - ye);
	    oldx = x = xb - x0;
	    oldy = y = y0 - yb;
	    /* Moved y-- and x++ right before adding to chainc */
	    while ((end_oct_xmajor)	/* && (x < xe - x0) */
		   || (!end_oct_xmajor) && (y > y0 - ye)
		)
	    {
		while (abs((rad2) - ((x + 1) * (x + 1) + y * y)) <
		       abs((rad2) - ((y - 1) * (y - 1) + x * x)))
		{		/* increment f until error is positive */
		    x++;
		    cnt++;
		    chainc |= 1 << 16 - cnt;
		    if (ptc > pattern2a[(int) style][ipat])
		    {
			ipat++;
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			if ((ipat % 2) == 0)
			    chainc = 0;
			else
			    chainc = 1 << 15;
			numf++;
			oldx = x;
			oldy = y;
			ptc = 0;
			ipat &= 7;
		    }
		    else
		    {
			ptc++;
			if (cnt >= 16)
			{
			    cnt = 1;
			    bchain[numf].x = x - oldx;
			    bchain[numf].y = oldy - y;
			    bchain[numf].bits = chainc;
			    /* move to next word */
			    chainc = 0;
			    numf++;
			    oldx = x;
			    oldy = y;
			}
		    }
		    if ((end_oct_xmajor) && (x >= xe - x0))
			goto endpat4;
		}
		y--;
		cnt++;
		/* chainc |= 0 << 16 - cnt; */
		if (ptc > pattern2a[(int) style][ipat])
		{
		    ipat++;
		    cnt = 1;
		    bchain[numf].x = x - oldx;
		    bchain[numf].y = oldy - y;
		    bchain[numf].bits = chainc;
		    /* move to next word */
		    if ((ipat % 2) == 0)
			chainc = 0;
		    else
			chainc = 1 << 15;
		    numf++;
		    oldx = x;
		    oldy = y;
		    ptc = 0;
		    ipat &= 7;
		}
		else
		{
		    ptc++;
		    if (cnt >= 16)
		    {
			cnt = 1;
			bchain[numf].x = x - oldx;
			bchain[numf].y = oldy - y;
			bchain[numf].bits = chainc;
			/* move to next word */
			numf++;
			chainc = 0;
			oldx = x;
			oldy = y;
		    }
		}
	    }			/* end for y */
    endpat4:
	    if (cnt > 1)
	    {
		bchain[numf].x = x - oldx;
		bchain[numf].y = oldy - y;
		bchain[numf].bits = chainc;
		numf++;
	    }
	    pw_curve(_cgi_vws->sunview.pw,
		     xb, yb, bchain, numf, _cgi_pix_mode,
		     val);
	    break;
	}			/* end switch (iquad) */
    }				/* end (style != SOLID) */
#ifdef cgi_wds_removed_850220
    return (0);
#endif cgi_wds_removed_850220
}


#ifdef	UNUSED
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int             _cgi_new_arc_pts(x0, y0, x, y, x1, y1, color, quad)
short           x0, y0, x, y, x1, y1, color, quad;
{				/* draw symmetry points of circle */
    short           a1, /* a2, a3, a4, b1, */ b2 /* , b3, b4 */ ;
    short           ra1, /* ra2, ra3, ra4, rb1, */ rb2 /* , rb3, rb4 */ ;
    if (_cgi_texture[1] == 1)
    {
	switch (quad)
	{
	case 1:
	    a1 = x0 + x;
	    b2 = y0 + y;
	    ra1 = x0 + x1;
	    rb2 = y0 + y1;
	    break;
	case 2:
	    a1 = x0 - x;
	    b2 = y0 + y;
	    ra1 = x0 - x1;
	    rb2 = y0 + y1;
	    break;
	case 3:
	    a1 = x0 - x;
	    b2 = y0 - y;
	    ra1 = x0 - x1;
	    rb2 = y0 - y1;
	    break;
	case 4:
	    a1 = x0 + x;
	    b2 = y0 - y;
	    ra1 = x0 + x1;
	    rb2 = y0 - y1;
	    break;
	}
	pw_vector(_cgi_vws->sunview.pw, a1, b2, ra1, rb2, _cgi_pix_mode, color);
    }
}

#endif	UNUSED

#endif	DAVID_REMOVED_870205
