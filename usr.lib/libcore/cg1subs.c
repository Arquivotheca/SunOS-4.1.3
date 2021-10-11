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
static char sccsid[] = "@(#)cg1subs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sunwindow/window_hs.h>
#include "coretypes.h"
#include "corevars.h"
#include "cgdd.h"
#include <stdio.h>

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _core_csimline                                             */
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
    static int rop, color;
    static struct pixrect *destpr;
    static struct pixrect *zbpr, *cutpr;

/*-------------------------------------*/
_core_csimline( x0,y0,x1,y1,op,wind,width,style)
    int x0,y0,x1,y1;		/* NDC coords  */
    struct windowstruct *wind;
    int op, width, style;	/* NDC width */
{
    int dx,dy;				/* vector components */
    int factor, tl;
    short i, dxprime, dyprime, p0x, p0y, p1x, p1y;
    short minax, majax, pos_slope, error;
    int diag_square, cur_error, old_error;
    int maj_count, maj_square, min_count, min_square;

    rop = op;
    destpr = wind->pixwin->pw_pixrect;
    color = PIX_OPCOLOR( op);
    dx = x1 - x0;
    dy = y1 - y0;

    vecl = _core_jsqrt((unsigned int)(dx*dx + dy*dy));
    if (style) {			/* if not solid fat line */
	if (vecl==0) {
	    if (wind->needlock)
                pw_lock( wind->pixwin, (Rect *)&wind->rect);
	    circle(x0, y0, (width)/2);
	    if (wind->needlock)
                pw_unlock( wind->pixwin);
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
	    if (wind->needlock)
                pw_lock( wind->pixwin, (Rect *)&wind->rect);
	    plotline(x0,y0,x1,y1);
	    if (wind->needlock)
                pw_unlock( wind->pixwin);
	    return(0);
	    }
	}
    else pdl[0] = 0xfffff;

    if(vecl) {

        pos_slope = ((dx>0)^(dy>0)) ? FALSE : TRUE;
        dyprime = (width*abs(dx)/vecl)>>1;
        dxprime = (width*abs(dy)/vecl)>>1;

	if (wind->needlock)
            pw_lock( wind->pixwin, (Rect *)&wind->rect);

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
    if (wind->needlock)
        pw_unlock( wind->pixwin);
    return(0);
}

/*-----------------------------------*/
static plotline( x0,y0,x1,y1)
short x0,y0,x1,y1;
{
    register int xb,yb,xe,ye;
    int totl,i;
    
    xb = xe = x0 << 10;			/* begin and end piece */
    yb = ye = y0 << 10;
    i = 0;
    totl = pdl[i];			/* total line length so far */
    while ( (totl>>10) < vecl) {		/* draw one textured line */
	xe += pdx[i]; ye += pdy[i];
	if ((i&1)==0)
		pr_vector(destpr, xb>>10, yb>>10, xe>>10, ye>>10, rop, color);
	totl += pdl[i++];
        i &= 3;				/* four part pattern cycle */
	xb = xe; yb = ye;
	}
    					/* finish remainder of line */
    pr_vector( destpr, xb>>10, yb>>10, x1, y1, rop, color); 
}
    
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
/*------------------------------------*/
static circ_pts( x0,y0,x,y) short x0,y0,x,y;
{					/* draw symmetry points of circle */
	short a1,a2,a3,a4,b1,b2,b3,b4;
	a1 = x0+x; a2 = x0+y; a3 = x0-x; a4 = x0-y;
	b1 = y0+x; b2 = y0+y; b3 = y0-x; b4 = y0-y;
        pr_vector( destpr, a3, b2, a1, b2, rop, color);		/* solid */
        pr_vector( destpr, a4, b1, a2, b1, rop, color); 
        pr_vector( destpr, a4, b3, a2, b3, rop, color); 
        pr_vector( destpr, a3, b4, a1, b4, rop, color); 
}

#include "colorshader.h"
static int _core_hidden;
/*------------------------------------*/
	typedef struct edge3_type {	/* edge type */
	    int ymn, ymx;		/* lower, upper y of edge */
	    int vx, dx;			/* x val, delta for edge */
	    int vz, dz;			/* z val, delta for edge */
	    int nx,ny,nz, dnx,dny,dnz;	/* normal vector, normal deltas */
	    struct edge3_type *nxt;	/* int is 15 bits rt of decimal pt */
	    } edge3;
	static int pistyle;
/*------------------------------------*/
_core_cregion3( vlist, npts, op, wind, zbuffer, cutaway, style, hidden)
	vtxtype *vlist; int npts;
	struct windowstruct *wind;
	int op, style, hidden;
	struct pixrect *zbuffer, *cutaway;
{
	/*------------------------------------------------------*/
	/* fill a region defined by a list of 2-D points, where */
	/* npts is the number of points in the lists pointed to */
	/* by x and y.  This routine joins the last point and   */
	/* the first point to close the boundary.  See Foley and*/
	/* Van Dam FUNDAMENTALS OF INTERACTIVE COMPUTER GRAPHICS*/
	/* pg 459 for algorithm description.			*/
	/*------------------------------------------------------*/

	edge3 aet[1];		/* active edge table */
	int i, edy, swap, cury;
	int symx, symn;
	edge3 *et;			/* edge table */
	vtxtype *vp, *vp1;
	register edge3 *p1, *p2, *pet, *paet;

	destpr = wind->pixwin->pw_pixrect;
	zbpr = zbuffer;
	cutpr = cutaway;
	_core_hidden = hidden;
	rop = op;
	color = PIX_OPCOLOR( op);
	pistyle = style;
	vp = vlist;			/* save vertex list start */
	if (npts < 3) return(1);	/* need 3 pts for boundary */
	et = (edge3 *)malloc( (unsigned int)((npts+1)*sizeof( *et)));	/* alloc edge table */
	if (!et) { 
	    fprintf(stderr, "_core_cregion3 cannot allocate memory.");
	    return( 1);
	    }

	pet = et;  pet->nxt = &et[1];	/* dummy 1st entry points to nxt */
	for (i=1; i<npts; i++){		/* fill the edge table */
	    vp1 = vlist+1;
	    if (vlist->y != vp1->y) {	/* horiz edges don't count */
	        pet++;
		pet->ymn = vlist->y;  pet->ymx = vp1->y;	/* edges */
		pet->vx = vlist->x;   pet->dx = vp1->x;
		pet->vz = vlist->z;   pet->dz = vp1->z;
		pet->nx = vlist->dx;  pet->dnx = vp1->dx;	/* normals */
		pet->ny = vlist->dy;  pet->dny = vp1->dy;
		pet->nz = vlist->dz;  pet->dnz = vp1->dz;
		pet->nxt = pet+1;
		}
	    vlist = vp1;
	    }
	if (vlist->y != vp->y) {
	    pet++;			/* wrap around to first point   */
	    pet->ymn = vlist->y;  pet->ymx = vp->y;
	    pet->vx = vlist->x;   pet->dx = vp->x;
	    pet->vz = vlist->z;   pet->dz = vp->z;
	    pet->nx = vlist->dx;  pet->dnx = vp->dx;	/* normals */
	    pet->ny = vlist->dy;  pet->dny = vp->dy;
	    pet->nz = vlist->dz;  pet->dnz = vp->dz;
	    }
	pet->nxt = 0;

	p1 = et[0].nxt;				/* initialize edge info */
	while (p1) {
	    if (p1->ymn < p1->ymx) {
	        edy = p1->ymx - p1->ymn;
		p1->dx = ((p1->dx - p1->vx)<<15)/edy;
		p1->vx = p1->vx << 15;
		p1->dz = ((p1->dz - p1->vz)<<15)/edy;
		p1->vz = p1->vz << 15;
		p1->dnx = (p1->dnx - p1->nx)/edy;	/* unit normals */
		p1->dny = (p1->dny - p1->ny)/edy;
		p1->dnz = (p1->dnz - p1->nz)/edy;
		}
	    else if (p1->ymn > p1->ymx){
	        edy = p1->ymn - p1->ymx;
		i = p1->ymn;  p1->ymn = p1->ymx; p1->ymx = i;
		i = p1->dx;
		p1->dx = ((p1->vx - p1->dx)<<15)/edy; p1->vx = i<<15;
		i = p1->dz;
		p1->dz = ((p1->vz - p1->dz)<<15)/edy; p1->vz = i<<15;
		i = p1->dnx;					/* normals */
		p1->dnx = (p1->nx - p1->dnx)/edy; p1->nx = i;
		i = p1->dny;
		p1->dny = (p1->ny - p1->dny)/edy; p1->ny = i;
		i = p1->dnz;
		p1->dnz = (p1->nz - p1->dnz)/edy; p1->nz = i;
		}
	    else {
		p1->dx = (p1->vx - p1->dx) << 15;   p1->vx <<= 15;
		p1->dz = (p1->vz - p1->dz) << 15;   p1->vz <<= 15;
		p1->dnx = (p1->nx - p1->dnx);		/* nrmls*/
		p1->dny = (p1->ny - p1->dny);
		p1->dnz = (p1->nz - p1->dnz);
		}
	    p1 = p1->nxt;
	    }

	if (p1 = et[0].nxt) {
	    symn = p1->ymn;  symx = p1->ymx;
	    }
	else return(0);
	while (p1->nxt) {			/* fix double edge endpoints */
	    if (p1->ymx == p1->nxt->ymn) p1->ymx--;
	    else if (p1->ymn == p1->nxt->ymx) {
		p1->ymn++; 
		p1->vx += p1->dx; p1->vz += p1->dz;
		p1->nx += p1->dnx;		/* normals */
		p1->ny += p1->dny; p1->nz += p1->dnz;
		}
	    p1 = p1->nxt;
	    }
	if (p1->ymx == symn) p1->ymx--;
	else if (p1->ymn == symx) {
		p1->ymn++; 
		p1->vx += p1->dx; p1->vz += p1->dz;
		p1->nx += p1->dnx;		/* normals */
		p1->ny += p1->dny; p1->nz += p1->dnz;
		}

	do {					/* sort edge table on > y */
	    swap = FALSE;
	    pet = et;  p1 = pet->nxt;		/* bubble sort for now */
	    do {
		if (p1->ymn > p1->nxt->ymn) {
		    paet = p1->nxt;		/* swap to sort y */
		    p1->nxt = p1->nxt->nxt;
		    paet->nxt = pet->nxt;
		    pet->nxt = paet;
		    swap = TRUE;
		    }
		else if(p1->ymn == p1->nxt->ymn)/* sort on > x, within y */
		    if (p1->vx > p1->nxt->vx) {
			paet = p1->nxt;		/* swap to sort x */
			p1->nxt = p1->nxt->nxt;
			paet->nxt = pet->nxt;
			pet->nxt = paet;
			swap = TRUE;
			}
		pet = pet->nxt;
		p1 = pet->nxt;
		}
	    while (p1->nxt);
	    }
	while (swap);

	pet = et[0].nxt;	/* Ready to draw the segments */
	paet = aet;
	cury = pet->ymn;		/* set y to 1st non-empty bucket */
	paet->nxt = 0;			/* init aet to empty */
	if (wind->needlock)
	    pw_lock(wind->pixwin, (Rect *)&wind->rect);
	do {				/* repeat til aet & et are empty */
	    p1 = paet;			/* merge current ymn et with aet */
	    while (pet && cury == pet->ymn) {
		while ( p1->nxt && (p1->nxt->vx < pet->vx))
		    p1 = p1->nxt;
		p2 = p1->nxt;
		p1->nxt = pet;
		pet = pet->nxt;
		p1->nxt->nxt = p2;
		}
		
	    p1 = paet->nxt;		/* paint current scanline segments */
	    while (p1 && p1->nxt) {
		shadeseg( cury, p1, p1->nxt);
		p1 = p1->nxt->nxt;
		}
	    p1 = paet;			/* remove active edges whose ymx */
	    while (p1->nxt)		/* = cury */
		if (p1->nxt->ymx == cury)/* paet->nxt -> p1->ymn */
		    p1->nxt = p1->nxt->nxt;
		else p1 = p1->nxt;
	    p1 = paet;			/* update edge values in aet */
	    while (p1->nxt) {
		p1 = p1->nxt;
		p1->vx += p1->dx;
		p1->vz += p1->dz;
		p1->nx += p1->dnx;		/* normals */
		p1->ny += p1->dny;
		p1->nz += p1->dnz;
		}

	    p1 = paet;
	    if (p1->nxt)
	        do {		/* resort on > vx because previous */
		    p1 = paet; p2 = p1->nxt;/* step may have crossed edges */
		    swap = FALSE;
		    while (p2->nxt)
		        if (p2->vx > p2->nxt->vx) {
			    p1->nxt = p2->nxt;
			    p2->nxt = p2->nxt->nxt;
			    p1->nxt->nxt = p2;
			    p1 = p1->nxt;
			    swap = TRUE;
			    }
		        else {
			    p1 = p1->nxt; p2 = p1->nxt;
			    }
		    }
	        while (swap);
	    cury++;			/* step to next scan line */
	    } while (paet->nxt || pet);
	    if (wind->needlock)
		pw_unlock(wind->pixwin);
	free((char *)et);
	return (0);
}
/*---------------------------------------------------------*/
#define one	32768
static shadeseg( y, lep, rep) int y; edge3 *lep, *rep;
{		/* paint the segment between left edge and right edge */
    int x1, x2, dx, vz,dz;
    int i, w, shade, dshade;
    register shading_parameter_structure *sp;
    short *zbptr, *zbcut;

    sp = &_core_shading_parameters;
    x1 = lep->vx + 16384 >> 15;
    x2 = rep->vx + 16384 >> 15;
    if (_core_hidden) {				/* get the piece of zbuf */
	    _core_pwzbuffer_ptr( x1, y, &zbptr, &zbcut, zbpr, cutpr);
	    					/* for this y into mem */
	    dx = x2-x1; dx = (dx<=0) ? 1 : dx;
	    vz = lep->vz;
	    dz = (rep->vz - lep->vz) / dx;
    }
    if ( pistyle == PLAIN || sp->shadestyle == WARNOCK) {
	    if (_core_hidden) {
		    for (i=x1; i<=x2; i++) {
			if (zbvisible( *zbptr, *zbcut, vz>>15)) {
			    pr_put( destpr, i, y, color);
			    *zbptr = vz>>15;
			    }
			vz += dz; zbptr++; zbcut++;
			}
		    }
	    else
		    (void)pr_rop( destpr, x1, y, x2-x1+1, 1, rop, 
			(struct pixrect *)0,0,0);
	    }
    else if (sp->shadestyle == GOURAUD) {	/* interpolate intensities */
	dx = x2-x1;
	dx = (dx<=0) ? 1 : dx;
	shade = lep->nx;			/* intensity stored in nx */
	dshade = (rep->nx - lep->nx)/dx;
	if (_core_hidden) {
	    for (i=x1; i<=x2; i++) {
		if (zbvisible( *zbptr, *zbcut, vz>>15)) {
		    *zbptr = vz>>15;
		    switch (sp->hue) {
			case 0: w = shade>>7; break;
			case 1: w = (shade>>9)  +1; break;
			case 2: w = (shade>>9) +64; break;
			case 3: w = (shade>>9)+128; break;
			case 4: w = (shade>>9)+192; break;
			default: break;
			}
		    pr_put( destpr, i, y, w);
		    }
		vz += dz; zbptr++; zbcut++;
		shade += dshade;
		}
	    }
	else {
	    for (i=x1; i<=x2; i++) {
		switch (sp->hue) {
		    case 0: w = shade>>7; break;
		    case 1: w = (shade>>9)  +1; break;
		    case 2: w = (shade>>9) +64; break;
		    case 3: w = (shade>>9)+128; break;
		    case 4: w = (shade>>9)+192; break;
		    default: break;
		    }
		pr_put( destpr, i, y, w);
		shade += dshade;
		}
	    }
    } else {					/* 'PHONG' interp normals */
        int length,lnx,lny,lnz,rnx,rny,rnz,vis;
        int da, T1mPQ, spec, Aspec, dAspec;
	register int A,dA,dB,ddB;			/* diff eqn for LdotN */
	register int B;
        int Bsqrt;

	/* convert normal vectors to unit vectors */
	length = _core_jsqrt((unsigned int)(((lep->nx*lep->nx+16384>>15)
		+(lep->ny*lep->ny+16384>>15)
		+ (lep->nz*lep->nz+16384>>15))   <<15));
	if (length) {
		lnx = (lep->nx<<15)/length; lny = (lep->ny<<15)/length;
		lnz = (lep->nz<<15)/length;
		}
	length = _core_jsqrt((unsigned int)(((rep->nx*rep->nx+16384>>15)
		+(rep->ny*rep->ny+16384>>15)
		+ (rep->nz*rep->nz+16384>>15))   <<15));
	if (length) {
		rnx = (rep->nx<<15)/length; rny = (rep->ny<<15)/length;
		rnz = (rep->nz<<15)/length;
		}
	/*
	 *  Difference eqns for Light dot SurfaceNormal
	 *  see Tom Duff, Computer Graphics 13:2, Siggraph 79
	 */
	dx = x2-x1;
	dx = (dx<=0) ? 1 : dx;
	da = one/dx;
        A =(lnx*sp->lx+16384>>15)+(lny*sp->ly+16384>>15)+(lnz*sp->lz+16384>>15);
        B =(rnx*sp->lx+16384>>15)+(rny*sp->ly+16384>>15)+(rnz*sp->lz+16384>>15);
	dA = (da*(B-A)+16384)>>15;
	B = one;
        dB = (lnx*rnx+16384>>15)+(lny*rny+16384>>15)+(lnz*rnz+16384>>15);
	T1mPQ = (da*(one-dB)+8192)>>14;		/* 2 * (1- P dot Q) */
	ddB = (da*T1mPQ+8192)>>14;
	dB = ((da-one)*T1mPQ+16384)>>15;

	if (sp->specularamt) {		/* L dot H needed for hilites */
	    Aspec =(lnx*sp->hx+16384>>15)+(lny*sp->hy+16384>>15)+
		(lnz*sp->hz+16384>>15);
	    T1mPQ =(rnx*sp->hx+16384>>15)+(rny*sp->hy+16384>>15)+
		(rnz*sp->hz+16384>>15);
	    dAspec = (da*(T1mPQ-Aspec)+16384)>>15;
	    }
	for (i=x1; i<=x2; i++) {
	    vis = TRUE;
	    if (_core_hidden){
		vis = zbvisible( *zbptr, *zbcut, vz>>15);
		if (vis) *zbptr = vz>>15;
	        vz += dz; zbptr++; zbcut++;
		}
	    if (vis) {
		Bsqrt = isqrt((unsigned int)(B<<15));		/* update diffuse shade */
		da = (A<<15)/Bsqrt;
		if (da <     0) da = -(da>>1);
		if (da > 32767) da = 32767;
		shade = sp->ambient+(da*sp->diffuseamt>>15);
		if (sp->floodamt) {
		    shade -= vz*sp->floodamt>>15; vz += dz; }
		if (sp->specularamt) {		/* update specular shade */
		    da = (Aspec<<15)/Bsqrt;
		    if (da <     0) da = 0;
		    if (da > 32767) da = 32767;
		    switch ((sp->bump>9)?9:sp->bump) {  /* bump = spec power */
			case 9: da = da*da>>15;
			case 8: da = da*da>>15;
			case 7: da = da*da>>15;
			case 6: da = da*da>>15;
			case 5: da = da*da>>15;
			case 4: da = da*da>>15;
			case 3: da = da*da>>15;
			default: da = da*da>>15;
			}
		    spec = da * sp->specularamt >> 15;
		    shade += spec;
		    }
	        switch (sp->hue) {
		    case 0: w = shade>>7;       break;
		    case 1: w = (shade>>9)  +1; break;
		    case 2: w = (shade>>9) +64; break;
		    case 3: w = (shade>>9)+128; break;
		    case 4: w = (shade>>9)+192; break;
		    default: break;
		    }
		pr_put( destpr, i, y, w);
		}
	    if (sp->specularamt) Aspec += dAspec;
	    A += dA;
	    B += dB;   dB += ddB;
	    }
	}
}

static isqrt(n) register unsigned n;
{
	register unsigned j,k,x;
	j = n>>23;		/* index into table */
	k = (n>>15) & 0xFF;		/* fraction for interpolation */
	x = (k * _core_sqrttable[j+1] + (0xFF-k) * _core_sqrttable[j] + 128) >> 8;
	return (x);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _core_cprvectext                                           */
/*                                                                          */
/*		This routine provides for plotting characters on the
	view surface in two dimensions.  The fonts have fill vectors so
	that the characters will appear solid if plotted at standard size.  */	
/****************************************************************************/

_core_cprvectext( s, font, up, path, space, x0, y0, vwprt, op, windptr)
    char *s; short font;
    ipt_type *up, *path, *space;
    int x0, y0, op;
    porttype *vwprt;
    struct windowstruct *windptr;
{
    short xmin, xmax;
    short curx, cury, nxtx, nxty;
    int x1, y1;
    register int upx,upy,lfx,lfy;
    int spx,spy;
    short i, penup, npts;
    int refx, refy, endx, endy, dmx;
    short lastvisible, nextvisible;
    short pts[250];  char ch;
    ipt_type vpup, vppath, vpspace;

    destpr = windptr->pixwin->pw_pixrect;
    color = PIX_OPCOLOR( op);
    refx = x0;		/* start point for text string dev coords */
    refy = y0;
				/* direction vecs in device coords << 5 */
    vpup = *up;			/* char up vector << 5 */
    upx = vpup.x; upy = vpup.y;
    vppath = *path;		/* char path vector << 5 */
    lfx = vppath.x; lfy = vppath.y;
    vpspace = *space;		/* spacing between chars in dev coords << 5*/
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
	if (windptr->needlock)
	    pw_lock(windptr->pixwin, (Rect *)&windptr->rect);
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
			    cury = refy + ((x1 * lfy + y1 * upy) >> 9);
			    }
			else {			/* else draw to the point */
			    x1 -= xmin;
			    nxtx = refx + ((x1 * lfx + y1 * upx) >> 9 );
			    nxty = refy + ((x1 * lfy + y1 * upy) >> 9 );
			    pr_vector( destpr,curx,cury,nxtx,nxty,op,color);
			    curx = nxtx; cury = nxty;
			    }
			penup = FALSE;
			}
		    }
	    refx = endx;
	    refy = endy;
	    lastvisible = nextvisible;
	    }
    if (windptr->needlock)
	pw_unlock(windptr->pixwin);
    return(0);
    }

