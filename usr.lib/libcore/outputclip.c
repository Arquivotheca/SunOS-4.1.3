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
static char sccsid[] = "@(#)outputclip.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
/* Output clipping routines clip to viewport of the segment being drawn */
/*----------------------------------------------------------------------*/
#include "coretypes.h"
#include "corevars.h"
#include <sys/types.h>

/*----------------------------------------------------------------------*/
static mkwec(v, wec, outcode, vwprt)
ipt_type *v;
int wec[], *outcode;
porttype *vwprt;
	{
	*outcode = 0;
	if ((wec[0] = v->x - vwprt->xmin) < 0) *outcode |= 1;
	if ((wec[1] = vwprt->xmax - v->x) < 0) *outcode |= 2;
	if ((wec[2] = v->y - vwprt->ymin) < 0) *outcode |= 4;
	if ((wec[3] = vwprt->ymax - v->y) < 0) *outcode |= 8;
	}


int _core_oclpvec2(v1, v2, vwprt)
ipt_type *v1, *v2;
porttype *vwprt;
	{		/* 2-D Clipper. Clips vector from v1 to v2 to
			   viewport in x and y.  Z is interpolated.
			   Cohen-Sutherland algorithm with window-edge
			   coordinates adapted from Newman & Sproull (2nd Ed.).
			*/

	int wec1[4], wec2[4], outcodes, outcode1, outcode2;
	u_int t1, t2, t;
	int i, j, n, d;
	int dx, dy, dz;
	int shiftx, shifty, shiftz;
	short sdx, sdy, sdz;

	mkwec(v1, wec1, &outcode1, vwprt);
	mkwec(v2, wec2, &outcode2, vwprt);
	if (!(outcodes = outcode1 | outcode2))
		return(TRUE);
	if (outcode1 & outcode2)
		return(FALSE);
	t1 = 0;
	t2 = 65536;
	j = 1;
	for (i = 0; i < 4; i++)
		{
		if (outcodes & j)
			{
				/* The following several lines accomplish the
				   computation t = wec1[i] / (wec1[i] - wec2[i])
				   where t is a fixed-point number with the
				   bottom 16 bits the fractional part.  Thus
				   the code maintains adequate precision and
				   guards against overflow.
				   Note that the numerator and denominator have
				   the same sign and that abs(n) <= abs(d).
				*/
			if ((n = wec1[i]) < 0)
				{
				n = -n;
				d = wec2[i] - wec1[i];
				}
			else
				d = wec1[i] - wec2[i];
			while (n > 65535)
				{
				n >>= 1;
				d >>= 1;
				}
			t = ((u_int) n * 65536) / (u_int) d;
			if (outcode1 & j)
				{
				if (t > t1)
					t1 = t;
				}
			else
				{
				if (t < t2)
					t2 = t;
				}
			}
		j <<= 1;
		}
	if (t2 < t1)
		return(FALSE);
					/* Compute magnitudes and signs of
					   deltas in preparation for computing
					   clipped endpoints below.
					   Also scale deltas to prevent overflow
					   below.
					*/
	if ((dx = v2->x - v1->x) < 0)
		{
		dx = -dx;
		sdx = 1;
		}
	else 
		sdx = 0;
	shiftx = 16;
	while (dx > 65535)
		{
		dx >>= 1;
		shiftx--;
		}
	if ((dy = v2->y - v1->y) < 0)
		{
		dy = -dy;
		sdy = 1;
		}
	else 
		sdy = 0;
	shifty = 16;
	while (dy > 65535)
		{
		dy >>= 1;
		shifty--;
		}
	if ((dz = v2->z - v1->z) < 0)
		{
		dz = -dz;
		sdz = 1;
		}
	else 
		sdz = 0;
	shiftz = 16;
	while (dz > 65535)
		{
		dz >>= 1;
		shiftz--;
		}

	if (t2 != 65536)
		{
		register int d;
					/* The following lines compute
					   v2->x = v1->x + ((t2 * dx) >> 16)
					   The scaling done above prevents
					   overflow for large dx.
					*/
		(u_int) d = (t2 * (u_int) dx) >> shiftx;
		if (sdx)
			d = -d;
		v2->x = v1->x + d;

					/* v2->y = v1->y + ((t2 * dy) >> 16) */
		(u_int) d = (t2 * (u_int) dy) >> shifty;
		if (sdy)
			d = -d;
		v2->y = v1->y + d;

					/* v2->z = v1->z + ((t2 * dz) >> 16) */
		(u_int) d = (t2 * (u_int) dz) >> shiftz;
		if (sdz)
			d = -d;
		v2->z = v1->z + d;

					/* Now clamp clipped coords to viewport
					   to prevent horrible effects from
					   roundoff error.
					*/
		if (v2->x < vwprt->xmin)
			v2->x = vwprt->xmin;
		else if (v2->x > vwprt->xmax)
			v2->x = vwprt->xmax;
		
		if (v2->y < vwprt->ymin)
			v2->y = vwprt->ymin;
		else if (v2->y > vwprt->ymax)
			v2->y = vwprt->ymax;
		}
	if (t1 != 0)
		{
		register int d;

					/* v1->x += (t1 * dx) >> 16 */
		(u_int) d = (t1 * (u_int) dx) >> shiftx;
		if (sdx)
			d = -d;
		v1->x += d;

					/* v1->y += (t1 * dy) >> 16 */
		(u_int) d = (t1 * (u_int) dy) >> shifty;
		if (sdy)
			d = -d;
		v1->y += d;

					/* v1->z += (t1 * dz) >> 16 */
		(u_int) d = (t1 * (u_int) dz) >> shiftz;
		if (sdz)
			d = -d;
		v1->z += d;

					/* Now clamp clipped coords to viewport
					   to prevent horrible effects from
					   roundoff error.
					*/
		if (v1->x < vwprt->xmin)
			v1->x = vwprt->xmin;
		else if (v1->x > vwprt->xmax)
			v1->x = vwprt->xmax;
		
		if (v1->y < vwprt->ymin)
			v1->y = vwprt->ymin;
		else if (v1->y > vwprt->ymax)
			v1->y = vwprt->ymax;
		}
	_core_cpchang = TRUE;
	return(TRUE);
	}
/*--------------------------------------*/
_core_oclippt2(x,y,vwprt) register int x,y; register porttype *vwprt;
{			/* return true if point is in view window */

    if (  (x < vwprt->xmin) || (x >= vwprt->xmax)
        ||(y < vwprt->ymin) || (y >= vwprt->ymax))
        return(FALSE);
    else return(TRUE);
    }

/*-----------------------------------------------------------------------
	Polygon clipper for 2-D polygons.  Uses Sutherland Hodgeman algorithm
	in 'Reentrant Polygon Clipping', CACM 17:1, January 1974.
	Two function calls are included _core_oclpvtx2( p) must be called for
	each vertex of a polygon and _core_oclpend2() is called to terminate the
	polygon.  Output vertices are written to the global array 'vtxlist'.
	*/

/*------------globals---------------------*/
#define LEFTPLANE	0
#define RIGHTPLANE	1
#define BOTTOM		2
#define TOP		3
#define VTXSIZE		7

static int oprocvtx(), opolclos(), ooutvtx(), opolout();

static struct clipplane {
    struct clipplane *nextplane;	/* ptr to data for next clip plane */
    int (*oprocvtx)();			/* vertex processing routine */
    int (*opolclos)();			/* close-polygon-entry routine */
    short firstvtx;			/* TRUE if this is 1st vertex  */
    short planeno;			/* clip plane number for this plane */
    short nout;				/* no. vtxs output by this stage */
    vtxtype f,				/* 1st vertex of polygon */
	    s,				/* saved previous vtx of current pol */
	    i;				/* intersect of edge with clip plane */
    } planes[4]	= {	/* 4 clipping planes */
	    {&planes[1], oprocvtx, opolclos, TRUE, LEFTPLANE},
	    {&planes[2], oprocvtx, opolclos, TRUE, RIGHTPLANE},
	    {&planes[3], oprocvtx, opolclos, TRUE, BOTTOM},
	    {&planes[0], ooutvtx, opolout, TRUE, TOP}
	    };

/*-------------------------------------------
printplanes()
{
    int i;
    for (i=0; i<4; i++) {
	fprintf(stderr, " %d %d %d\n", planes[i].firstvtx, planes[i].planeno,
	planes[i].nout);
	fprintf(stderr, " %d %d %d %d\n", planes[i].f.x,planes[i].f.y,
	planes[i].f.z,planes[i].f.w);
	fprintf(stderr, " %d %d %d %d\n", planes[i].s.x,planes[i].s.y,
	planes[i].s.z,planes[i].s.w);
	fprintf(stderr, " %d %d %d %d\n", planes[i].i.x,planes[i].i.y,
	planes[i].i.z,planes[i].i.w);
	}
    }
---------------------------------------------*/
static porttype *vwprt;
/*------------------------------------*/
_core_oclpvtx2( p,vwp) vtxtype *p; porttype *vwp;
{				/* hand this routine each vertex */
    vwprt = vwp;
    oprocvtx( p, &planes[0]);
    }
/*------------------------------------*/
_core_oclpend2()			/* this routine terminates polygon */
{
    opolclos(  &planes[0]);
    }

/*------------------------------------*/
static oprocvtx( p, plane) vtxtype *p; struct clipplane *plane;
{
    if (plane->firstvtx) {
	plane->f = *p;			/* f is first vertex of polygon */
	plane->s = *p;			/* s is previous 'saved' vertex */
	}
    else {
	if (olinecross( plane, p)) {	/* if line sp crosses plane */
	    plane->s = *p;		/* compute intersect vtx i  */
	    (*(plane->oprocvtx))( &plane->i, plane->nextplane);
	    }
	else plane->s = *p;
	}
    if ( ovisible( plane)) {		/* if s on visible side of plane */
	(*(plane->oprocvtx))( &plane->s, plane->nextplane);
	plane->nout++;
	}
    plane->firstvtx = FALSE;
    }

/*-----------------------------------------*/
static opolclos( pl) register struct clipplane *pl;
{
    if (pl->nout ) {
	if (olinecross( pl, &pl->f)) {		/* handle last edge */
	    (*(pl->oprocvtx))( &pl->i, pl->nextplane);
	    }
	}
    pl->firstvtx = TRUE;
    pl->nout = 0;
    (*(pl->opolclos))( pl->nextplane);
    }

/*--------------------------------------------*/
static ooutvtx( p, pl) vtxtype *p; struct clipplane *pl;
{			/* output routine from last stage of clipper */
					/* Now clamp clipped coords to viewport
					   to prevent horrible effects from
					   roundoff error.
					*/
	if (p->x < vwprt->xmin)
		p->x = vwprt->xmin;
	else if (p->x > vwprt->xmax)
		p->x = vwprt->xmax;

	if (p->y < vwprt->ymin)
		p->y = vwprt->ymin;
	else if (p->y > vwprt->ymax)
		p->y = vwprt->ymax;
	pl->nout++;
	_core_vtxlist[_core_vtxcount++] = *p;
}
/*--------------------------------------------*/
static opolout( pl) struct clipplane *pl;
{
    if (pl->nout ) {	/* later may want to notify about end of polygon */
        }
    pl->nout = 0;
    }
	
/*----------------------------------------*/
static olinecross( pl, p) struct clipplane *pl; vtxtype *p;
{
    register int shift;
    register u_int alpha, n, d;
    int svis, pvis, i;
    int *ptri, *ptrs, *ptrp;
    short sd;

    switch( pl->planeno) {
	case LEFTPLANE:
	    svis = pl->s.x - vwprt->xmin;		/* x-(-w) */
	    pvis = p->x - vwprt->xmin;
	    break;
	case RIGHTPLANE:
	    svis = vwprt->xmax - pl->s.x;		/* -(x-w) */
	    pvis = vwprt->xmax - p->x;
	    break;
	case BOTTOM:
	    svis = pl->s.y - vwprt->ymin;		/* y-(-w) */
	    pvis = p->y - vwprt->ymin;
	    break;
	case TOP:
	    svis = vwprt->ymax - pl->s.y;		/* -(y-w) */
	    pvis = vwprt->ymax - p->y;
	    break;
	}
    if ((svis ^ pvis) & 0x80000000) {		/* if opposite signs then on */
	if (((int) n = svis) < 0)		/* opposite sides of plane */
		{
		(int) n = - (int) n;
		(int) d = pvis - svis;
		}
	else
		(int) d = svis - pvis;
	while (n > 65535)
		{
		n >>= 1;
		d >>= 1;
		}
	alpha = (n << 16) / d;
	ptri = &pl->i.x;
	ptrs = &pl->s.x;
	ptrp = &p->x;
	for (i=0; i<VTXSIZE; i++){		/* compute intersection vrtx */
	    if (((int) d = *ptrp++ - *ptrs) < 0)
		{
		(int) d = - (int) d;
		sd = 1;
		}
	    else
		sd = 0;
	    shift = 16;
	    while (d > 65535)
		{
		d >>= 1;
		shift--;
		}
	    d = (alpha * d) >> shift;
	    if (sd)
		(int) d = - (int) d;
	    *ptri++ = *ptrs++ + (int) d;
	    }
	return( TRUE);
	}
    else return( FALSE);
    }
/*-------------------------------------*/
static ovisible( pl) struct clipplane *pl;
{
    int svis;

    switch( pl->planeno) {
	case LEFTPLANE:
	    svis = pl->s.x - vwprt->xmin;		/* x-(-w) */
	    break;
	case RIGHTPLANE:
	    svis = vwprt->xmax - pl->s.x;		/* -(x-w) */
	    break;
	case BOTTOM:
	    svis = pl->s.y - vwprt->ymin;		/* y-(-w) */
	    break;
	case TOP:
	    svis = vwprt->ymax - pl->s.y;		/* -(y-w) */
	    break;
	}
    if (svis >= 0) return( TRUE);
    else return( FALSE);
    }
