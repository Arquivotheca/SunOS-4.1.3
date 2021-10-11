/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
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
#ifndef lint
static char sccsid[] = "@(#)pixwinsubs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <sunwindow/window_hs.h>
#include <stdio.h>

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _core_bwsimline                                            */
/*                                                                          */
/*     PURPOSE: SIMULATE A LINE OF THE CURRENT LINE STYLE AND WIDTH         */
/*                                                                          */
/****************************************************************************/
    static int pattern[4][5] = {0,0,0,0,0,		/* solid  */
			2500,5000,2500,5000,15000,	/* dotted */
		        10000,10000,10000,10000,40000,	/* dashed */
		        22500,7500,7500,7500,45000};	/* dash dot */
    static int pdx[4],pdy[4],pdl[4];		/* pattern length vector */    
    static int vecl;
    static int rop;
    static struct pixwin *destpw;

/*--------------------------------------------------------------------------*/
_core_bwsimline( x0,y0,x1,y1,wind,op,width,style)
    int x0,y0,x1,y1;		/* NDC coords  */
    struct windowstruct *wind;
    int op, width, style;	/* NDC width   */
{
    int dx,dy;				/* vector components */
    int factor, tl;
    short i, dxprime, dyprime, p0x, p0y, p1x, p1y;
    short minax, majax, pos_slope, error;
    int diag_square, cur_error, old_error;
    int maj_count, maj_square, min_count, min_square;

    rop = op;
    destpw = wind->pixwin;
    dx = x1 - x0;
    dy = y1 - y0;

    vecl = _core_jsqrt((unsigned int)(dx*dx + dy*dy));
    if (style) {			/* if not solid fat line */
	if (vecl==0) {
            pw_lock( destpw, (Rect *)&wind->rect);
	    circle(x0, y0, (width)/2);
    	    pw_unlock( destpw);
	    return(0);
	    }
	for (i=0; i<4; i++) {		/* make four part pattern cycle */
	    tl = pattern[style][i];
	    factor = tl / vecl;		/* 10 bits to right of binary pt */
	    pdx[i] = (dx * factor);	/* length vecs for 4 pieces */
	    pdy[i] = (dy * factor);	/* 2 bits rt of binary pt */
	    pdl[i] = _core_jsqrt((unsigned int)(pdx[i]*pdx[i] + pdy[i]*pdy[i]));
	    }
	if (width <= 1) {
    	    pw_lock(destpw, (Rect *)&wind->rect);
	    plotline(x0,y0,x1,y1);
    	    pw_unlock( destpw);
	    return(0);
	    }
	}
    else pdl[0] = 0xfffff;

    if(vecl) {

        pos_slope = ((dx>0)^(dy>0)) ? FALSE : TRUE;
        dyprime = (width*abs(dx)/vecl)>>1;
        dxprime = (width*abs(dy)/vecl)>>1;

        pw_lock( destpw, (Rect *)&wind->rect);

	diag_square = width*width;
	maj_count = 0;
	min_count = 0;
	maj_square = 1;
	min_square = 1;
	cur_error = diag_square - (maj_square + min_square);

        if (dxprime > dyprime) {
            minax=2*dyprime;
            majax=2*dxprime;
            error=2*dyprime - dxprime;
            p0x=x0-dxprime;
	    p1x=x1-dxprime;
	    if (pos_slope) {
		p0y = y0+dyprime;
		p1y = y1+dyprime;
	    } else {
		p0y = y0-dyprime;
		p1y = y1-dyprime;
	    }
    	    do {
                plotline(p0x,p0y,p1x,p1y);
                p0x++; p1x++;
	        maj_count++;
	        maj_square += (maj_count<<1) + 1;
	        error += minax;
                if (error > 0) {
                    plotline(p0x,p0y,p1x,p1y);
		    if (pos_slope) {
		        p0y--;
		        p1y--;
		    } else {
			p0y++;
			p1y++;
		    }
		    min_count++;
		    min_square += (min_count<<1) + 1;
		    error -= majax;
	        }
	        old_error = cur_error;
	        cur_error = diag_square - (maj_square + min_square);;
            } while ((abs(cur_error)) <= (abs(old_error)));

        } else {
            minax=2*dxprime;
            majax=2*dyprime;
            error=2*dxprime - dyprime;
            p0y=y0-dyprime;
	    p1y=y1-dyprime;
	    if (pos_slope) {
		p0x = x0+dxprime;
		p1x = x1+dxprime;
	    } else {
		p0x = x0-dxprime;
		p1x = x1-dxprime;
	    }
    	    do {
                plotline(p0x,p0y,p1x,p1y);
                p0y++; p1y++;
	        maj_count++;
	        maj_square += (maj_count<<1) + 1;
	        error += minax;
                if (error > 0) {
                    plotline(p0x,p0y,p1x,p1y);
		    if (pos_slope) {
		        p0x--;
		        p1x--;
		    } else {
			p0x++;
			p1x++;
		    }
		    min_count++;
		    min_square += (min_count<<1) + 1;
		    error -= majax;
		}
		old_error = cur_error;
		cur_error = diag_square - (maj_square + min_square);
            } while ((abs(cur_error)) <= (abs(old_error)));
        }
    }
    circle(x0, y0, (width-1)/2);
    circle(x1, y1, (width-1)/2);
    pw_unlock( destpw);
    return(0);
}

/*-----------------------------------*/
static plotline( x0,y0,x1,y1)
short x0,y0,x1,y1;
{
    register int xb,yb,xe,ye,i;
    int totl;
    
    xb = xe = x0 << 10;			/* begin and end piece */
    yb = ye = y0 << 10;
    i = 0;
    totl = pdl[i];			/* total line length so far */
    while ( (totl>>10) < vecl) {	/* draw one textured line */
	xe += pdx[i]; ye += pdy[i];
	if ((i&1)==0)
		(void)pw_vector(destpw, xb>>10, yb>>10, xe>>10, ye>>10,
		rop, 1);
	totl += pdl[i++];
        i &= 3;				/* four part pattern cycle */
	xb = xe; yb = ye;
	}
    (void)pw_vector(destpw, xb>>10, yb>>10, x1, y1,rop,1);
    /* do rest of line */
}
    
/*-------------------------------*/
static circle( x0,y0,rad) short x0,y0,rad;
{
	int x,y,d;

	x = 0;  y = rad;
	d = 3 - 2 * rad;
	while ( x<y) {
	    circ_pts( x0,y0,x,y);
	    if (d<0) d = d + 4*x + 6;
	    else {
		d = d + 4 *(x-y) + 10;
		y = y - 1;
		}
	    x += 1;
	    }
	if (x=y) circ_pts( x0,y0,x,y);
}
/*------------------------------------------------------------------------*/
static circ_pts( x0,y0,x,y) short x0,y0,x,y;
{					/* draw symmetry points of circle */
	short a1,a2,a3,a4,b1,b2,b3,b4;
	a1 = x0+x; a2 = x0+y; a3 = x0-x; a4 = x0-y;
	b1 = y0+x; b2 = y0+y; b3 = y0-x; b4 = y0-y;
        (void)pw_vector( destpw, a4, b1, a2, b1, rop, 1);
        (void)pw_vector( destpw, a3, b2, a1, b2, rop, 1);
        (void)pw_vector( destpw, a4, b3, a2, b3, rop, 1);
        (void)pw_vector( destpw, a3, b4, a1, b4, rop, 1);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _core_pwvectext                                           */
/*                                                                          */
/*		This routine provides for plotting characters on the
	view surface in two dimensions.  The 5 character fonts available
	are vector fonts.						    */	
/****************************************************************************/
_core_pwvectext( s, font, up, path, space, x0, y0, vwprt, op, windptr)
    char *s; short font;
    ipt_type *up, *path, *space;
    int x0, y0, op;
    porttype *vwprt;
    struct windowstruct *windptr;
{
    short xmin, xmax;
    short refx, refy, curx, cury, nxtx, nxty;
    short x1, y1;
    register int upx,upy,lfx,lfy;
    int spx,spy;
    short i, penup, npts, endx, endy, dmx;
    short lastvisible, nextvisible;
    short pts[250];  char ch;
    ipt_type vpup, vppath, vpspace;

    destpw = windptr->pixwin;
    refx = x0;		/* start point for text string dev coords */
    refy = y0;
				/* direction vecs in device coords << 5 */
    vpup = *up;			/* char up vector << 5 */
    upx = vpup.x; upy = vpup.y;
    vppath = *path;		/* char path vector << 5 */
    lfx = vppath.x; lfy = vppath.y;
    vpspace = *space;		/* spacing between chars in dev coords << 5 */
    spx = vpspace.x; spy = vpspace.y;

    /*
     *  Process all chars in string drawing in device coords
     */
	lastvisible = TRUE;
	nextvisible = TRUE;
	if (_core_wndwclip || _core_outpclip) {
	    lastvisible = !(refx < (int)(vwprt->xmin) ||
	    refy < (int)(vwprt->ymin) ||
	    refx > (int)(vwprt->xmax) ||
	    refy > (int)(vwprt->ymax));
	    }
	(void)pw_lock(destpw, (Rect *)&windptr->rect);
	while (ch = *s++) {		/* for all chars in string */
	    if (font < 4) _core_scribe( font, ch, &xmin, &xmax, &npts, pts);
	    else _core_sfont( font-4, ch, &xmin, &xmax, &npts, pts);
	    dmx = xmax - xmin;
	    endx = refx + ((dmx * lfx >> 4) + spx + 16 >> 5);
	    endy = refy + ((dmx * lfy >> 4) + spy + 16 >> 5);
	    penup = TRUE;
	    if (_core_wndwclip || _core_outpclip) {
		nextvisible = !(endx < (int)(vwprt->xmin) ||
	        endy < (int)(vwprt->ymin) ||
		endx > (int)(vwprt->xmax) ||
		endy > (int)(vwprt->ymax));
		}
	    if (lastvisible && nextvisible)
		for (i=0; i<npts; i += 2) {	/* for vectors in char */
		    x1 = pts[i];
		    if (x1 == 31) penup = TRUE;
		    else {			/* 31 is penup command */
			y1 = pts[i+1];
			if (penup) {
			    x1 -= xmin;
			    curx = refx + ((x1 * lfx + y1 * upx) >> 9);
			    cury = refy + ((x1 * lfy + y1 * upy) >> 9 );
			    }
			else {			/* else draw to the point */
			    x1 -= xmin;
			    nxtx = refx + ((x1 * lfx + y1 * upx) >> 9 );
			    nxty = refy + ((x1 * lfy + y1 * upy) >> 9 );
			    (void)pw_vector( destpw,curx,cury,nxtx,nxty,op,1);
			    curx = nxtx; cury = nxty;
			    }
			penup = FALSE;
			}
		    }
	    refx = endx;
	    refy = endy;
	    lastvisible = nextvisible;
	    }
	(void)pw_unlock( destpw);
    return(0);
    }

