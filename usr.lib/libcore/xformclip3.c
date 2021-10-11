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
static char sccsid[] = "@(#)xformclip3.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
	Operation:
		Three-D viewing transform routines and clipping routines.
	Usage:
		typedef struct{ float x,y,z,w;
		    } pt_type;			' homogeneous point type 
		
			--- Transform utility operations ---

		_core_tranpt( p1, p2) pt_type *p1, *p2;
					' transform 3-D point using stack top,
					' p2 gets transformed point.
		_core_sizept( p1, p2) pt_type *p1, *p2;
					' scale and rotate 3-D point, no
					' translation.
		_core_clipvec( p1,p2) ipt_type *p1, *p2;
					' Clip 3-D vector to window, return true
					' if visible.
		_core_clippt3( x,y,z) int x,y,z;
					' true if point is in view window

*/
/*--------------------------------------------------------------------*/
/*			--- Transform Variables ---		      */
#include "coretypes.h"
#include "corevars.h"
#include <math.h>
#include <sys/types.h>

/*--------------------------------------------------------------------*/
/*			--- Transform Operations ---		      */
/*--------------------------------------------------------------------*/
/*-----------------------------------*/
_core_tranpt( p1, p2) float *p1, *p2;	/* transform pt using stack top */
{					/* p2[i] = Sum p1[k]*tran[k,i]  */
	register float *m, *q;		/*	    k			*/
	register int k,i;
	float	sum, *m1;
	ddargtype ddstruct;

	if (_core_xformvs) {
	    _core_critflag++;	/* critical section since slight chance of
				   view surface termination via SIGCHILD
				   during the call to the dd */
	    ddstruct.instance = _core_xformvs->instance;
	    if (!_core_ddxformset) {
	        ddstruct.opcode = SETVWXFORM32K; /* send the matrix to the dd */
	        ddstruct.ptr1 = (char*)&_core_TStack[_core_TSp][0];
	        (*_core_xformvs->dd)(&ddstruct);
		_core_ddxformset = TRUE;
	        }
	    ddstruct.opcode = VIEWXFORM3;	/* transform a point */
	    ddstruct.ptr1 = (char*)p1;
	    ddstruct.ptr2 = (char*)p2;
	    (*_core_xformvs->dd)(&ddstruct);
	    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
		(*_core_sighandle)();
	    }
	else {
	    m1 = &_core_TStack[_core_TSp][0];
	    for (i=0; i<4; i++) {		/* for each output coord */
		q = p1;  m = m1 + i;
		sum = 0.0;
		for (k=0; k<4; k++) {	/* point times matrix column*/
		    sum += *q++ * *m;
		    m += 4;
		    }
		*p2++ = sum;
		}
	    }
	}
/*-----------------------------------*/
_core_sizept( p1, p2) float *p1, *p2;	/* scale, rotate pt using stack top */
{					/* p2[i] = Sum p1[k]*tran[k,i]  */
	register float *m, *q;		/*	    k			*/
	register int k,i;
	float	sum, *m1;

	m1 = &_core_TStack[_core_TSp][0];
	for (i=0; i<3; i++) {		/* for each output coord */
	    q = p1;  m = m1 + i;
	    sum = 0.0;
	    for (k=0; k<3; k++) {	/* point times matrix column*/
		sum += *q++ * *m;
		m += 4;
		}
	    *p2++ = sum;
	    }
        m = m1+3;
	*p2 = 0.0;
        for (k=0; k<4; k++) {
	    *p2 += *m;
	    m += 4;
	    }
	}
/*---------------------------------------*/
_core_pt3cnvrt( fp, ip)
register float *fp;
register int *ip;
{					/* convert 3-D float point to int pt */
    *ip++ = _core_roundf(fp);
    fp++;
    *ip++ = _core_roundf(fp);
    fp++;
    *ip++ = _core_roundf(fp);
    fp++;
    *ip = _core_roundf(fp);
    }
/*--------------------------------------*/
_core_imxfrm3( segptr, p1, p2) register ipt_type *p1, *p2; segstruc *segptr;
{			/* image transform integer point in NDC coordinates */
    int p1x, p1y, p1z, shiftx, shifty, shiftz;

    if (segptr->type == XLATE2) {
	p2->x = p1->x + segptr->imxform[3][0];
	p2->y = p1->y + segptr->imxform[3][1];
	p2->z = p1->z + segptr->imxform[3][2];
    } else if(segptr->type == XFORM2) {
	p1x = p1->x;
	shiftx = 15;
	while (p1x > 65535 || p1x < -65535)
	    {
	    p1x >>= 1;
	    shiftx--;
	    }
	p1y = p1->y;
	shifty = 15;
	while (p1y > 65535 || p1y < -65535)
	    {
	    p1y >>= 1;
	    shifty--;
	    }
	p1z = p1->z;
	shiftz = 15;
	while (p1z > 65535 || p1z < -65535)
	    {
	    p1z >>= 1;
	    shiftz--;
	    }
	p2->x = p1->x * (segptr->imxform[0][0] >> 16) +
		(p1x * (short)(segptr->imxform[0][0] & 0xFFFF) >> shiftx) +
		p1->y * (segptr->imxform[1][0] >> 16) +
		(p1y * (short)(segptr->imxform[1][0] & 0xFFFF) >> shifty) +
		p1->z * (segptr->imxform[2][0] >> 16) +
		(p1z * (short)(segptr->imxform[2][0] & 0xFFFF) >> shiftz) +
		segptr->imxform[3][0];
	p2->y = p1->x * (segptr->imxform[0][1] >> 16) +
		(p1x * (short)(segptr->imxform[0][1] & 0xFFFF) >> shiftx) +
		p1->y * (segptr->imxform[1][1] >> 16) +
		(p1y * (short)(segptr->imxform[1][1] & 0xFFFF) >> shifty) +
		p1->z * (segptr->imxform[2][1] >> 16) +
		(p1z * (short)(segptr->imxform[2][1] & 0xFFFF) >> shiftz) +
		segptr->imxform[3][1];

	p2->z = p1->z;
	    /* We don't compute p2->z here since the only visible effect
	       would be for view surfaces doing hidden surface elimination
	       and our current z-buffer routines don't handle repainting 
	       with dynamic image transformations.  Note that this means
	       that even static image transformations may not yield a
	       correct result.
	    */
    }
}
/*--------------------------------------*/
_core_imszpt3( segptr, p1, p2) register ipt_type *p1, *p2; segstruc *segptr;
{			/* image transform integer point in NDC coordinates */
    _core_imxfrm3(segptr, p1, p2);
    p2->x -= segptr->imxform[3][0];
    p2->y -= segptr->imxform[3][1];
    }

/*---------------------------------------*/

static mkwec(v, wec, outcode)
ipt_type *v;
int wec[], *outcode;
	{
	*outcode = 0;
	if ((wec[0] = v->w + v->x) < 0) *outcode |= 1;
	if ((wec[1] = v->w - v->x) < 0) *outcode |= 2;
	if ((wec[2] = v->w + v->y) < 0) *outcode |= 4;
	if ((wec[3] = v->w - v->y) < 0) *outcode |= 8;
	if ((wec[4] = v->z)        < 0) *outcode |= 16;
	if ((wec[5] = v->w - v->z) < 0) *outcode |= 32;
	*outcode &= _core_wclipplanes;
	}


int _core_clipvec(v1, v2)
ipt_type *v1, *v2;
	{		/* 3-D Clipper. Clips vector from v1 to v2 to view
			   volume. Operates in homogeneous coordinates.
			   Cohen-Sutherland algorithm with window-edge
			   coordinates adapted from Newman & Sproull (2nd Ed.).
			   Lines with endpoints with negative w are not
			   treated properly. This can be changed later if
			   necessary. */

	int wec1[6], wec2[6], outcodes, outcode1, outcode2;
	u_int t1, t2, t;
	int i, j, n, d;
	int dx, dy, dz, dw;
	int shiftx, shifty, shiftz, shiftw;
	int clipcoord, clipcoord2;
	short sdx, sdy, sdz, sdw;

	mkwec(v1, wec1, &outcode1);
	mkwec(v2, wec2, &outcode2);
	if (!(outcodes = outcode1 | outcode2))
		return(TRUE);
	if (outcode1 & outcode2)
		return(FALSE);
	t1 = 0;
	t2 = 65536;
	j = 1;
	for (i = 0; i < 6; i++)
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
	if ((dw = v2->w - v1->w) < 0)
		{
		dw = -dw;
		sdw = 1;
		}
	else 
		sdw = 0;
	shiftw = 16;
	while (dw > 65535)
		{
		dw >>= 1;
		shiftw--;
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

					/* v2->w = v1->w + ((t2 * dw) >> 16) */
		(u_int) d = (t2 * (u_int) dw) >> shiftw;
		if (sdw)
			d = -d;
		v2->w = v1->w + d;

					/* Now clamp clipped coords to view
					   volume to prevent horrible effects
					   from roundoff error.
					*/
		clipcoord = v2->w;
		clipcoord2 = clipcoord << 1;

		if ((_core_wclipplanes & 0x3) &&
		    ((u_int) (v2->x + clipcoord) > clipcoord2))
			if (v2->x < 0)
				v2->x = -clipcoord;
			else
				v2->x = clipcoord;

		if ((_core_wclipplanes & 0xC) &&
		    ((u_int) (v2->y + clipcoord) > clipcoord2))
			if (v2->y < 0)
				v2->y = -clipcoord;
			else
				v2->y = clipcoord;

		if ((_core_wclipplanes & 0x10) && (v2->z < 0))
			v2->z = 0;

		if ((_core_wclipplanes & 0x20) && (v2->z > clipcoord))
			v2->z = clipcoord;
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

					/* v1->w += (t1 * dw) >> 16 */
		(u_int) d = (t1 * (u_int) dw) >> shiftw;
		if (sdw)
			d = -d;
		v1->w += d;

					/* Now clamp clipped coords to view
					   volume to prevent horrible effects
					   from roundoff error.
					*/
		clipcoord = v1->w;
		clipcoord2 = clipcoord << 1;

		if ((_core_wclipplanes & 0x3) &&
		    ((u_int) (v1->x + clipcoord) > clipcoord2))
			if (v1->x < 0)
				v1->x = -clipcoord;
			else
				v1->x = clipcoord;

		if ((_core_wclipplanes & 0xC) &&
		    ((u_int) (v1->y + clipcoord) > clipcoord2))
			if (v1->y < 0)
				v1->y = -clipcoord;
			else
				v1->y = clipcoord;

		if ((_core_wclipplanes & 0x10) && (v1->z < 0))
			v1->z = 0;

		if ((_core_wclipplanes & 0x20) && (v1->z > clipcoord))
			v1->z = clipcoord;
		}
	return(TRUE);
	}

/*--------------------------------------*/

_core_clippt3(x,y,z) int x,y,z;
{			/* return true if point is in view window */

    if ((_core_wndwclip &&
	 	((x < -MAX_CLIP_COORD) || (x > MAX_CLIP_COORD) ||
          	 (y < -MAX_CLIP_COORD) || (y > MAX_CLIP_COORD)))
        || (_core_frontclip && (z < 0))
	|| (_core_backclip && (z > MAX_CLIP_COORD)))

		return(FALSE);

    else
		return(TRUE);
    }

/*------------------------------------------------------------------------
	Polygon clipper for 3-D polygons.  Uses Sutherland Hodgeman algorithm
	in 'Reentrant Polygon Clipping', CACM 17:1, January 1974.
	Two function calls are included clipvertex( p) must be called for
	each vertex of a polygon and clipend() is called to terminate the
	polygon.  Output vertices are written to the global array 'vtxlist'.
	*/

/*------------globals---------------------*/
#define LEFTPLANE	0
#define RIGHTPLANE	1
#define BOTTOM	2
#define TOP	3
#define NEAR	4
#define FAR	5
#define VTXSIZE	7

static int procvtx(), polclos(), outvtx(), polout();

static struct clipplane {
    struct clipplane *nextplane;	/* ptr to data for next clip plane */
    int (*procvtx)();			/* vertex processing routine */
    int (*polclos)();			/* close-polygon-entry routine */
    short firstvtx;			/* TRUE if this is 1st vertex  */
    short planeno;			/* clip plane number for this plane */
    short nout;				/* no. vtxs output by this stage */
    vtxtype f,				/* 1st vertex of polygon */
	    s,				/* saved previous vtx of current pol */
	    i;				/* intersect of edge with clip plane */
    } planes[6]	= {	/* 6 clipping planes */
	    {&planes[1], procvtx, polclos, TRUE, LEFTPLANE},
	    {&planes[2], procvtx, polclos, TRUE, RIGHTPLANE},
	    {&planes[3], procvtx, polclos, TRUE, BOTTOM},
	    {&planes[4], procvtx, polclos, TRUE, TOP},
	    {&planes[5], procvtx, polclos, TRUE, NEAR},
	    {&planes[0], outvtx, polout,   TRUE, FAR}
	    };

static int pclipplanes = 0x3F;	  /* Format of pclipplanes and _core_wclipplanes
				     is    ...|F|N|T|B|R|L|   */

/*-------------------------------------------
static printplanes()
{
    int i;
    for (i=0; i<6; i++) {
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
/*------------------------------------*/
_core_clpvtx3( p) vtxtype *p;
{				/* hand this routine each vertex */
				/* This routine assumes that
				   _core_wclipplanes != 0, so it should
				   never be called unless at least one
				   plane is being clipped against. */
    int i, j;

    if ( _core_wclipplanes != pclipplanes)
	{
	j = 0;
	if (_core_wndwclip)
	    for (i = 0; i < 4; i++)
		{
		planes[i].nextplane = &planes[i + 1];
		planes[i].procvtx = procvtx;
		planes[i].polclos = polclos;
		planes[i].firstvtx = TRUE;
		planes[i].planeno = LEFTPLANE + i;
		j++;
		}
	if (_core_frontclip)
		{
		planes[j].nextplane = &planes[j + 1];
		planes[j].procvtx = procvtx;
		planes[j].polclos = polclos;
		planes[j].firstvtx = TRUE;
		planes[j].planeno = NEAR;
		j++;
		}
	if (_core_backclip)
		{
		planes[j].nextplane = &planes[0];
		planes[j].procvtx = outvtx;
		planes[j].polclos = polout;
		planes[j].firstvtx = TRUE;
		planes[j].planeno = FAR;
		}
	else
		{
		planes[--j].nextplane = &planes[0];
		planes[j].procvtx = outvtx;
		planes[j].polclos = polout;
		}
	pclipplanes = _core_wclipplanes;
	}
    if (_core_vwstate.projtype == PARALLEL)  p->w = MAX_CLIP_COORD;
    procvtx( p, &planes[0]);
    }
/*------------------------------------*/
_core_clpend3()			/* this routine terminates polygon */
{
    polclos(  &planes[0]);
    }

/*------------------------------------*/
static procvtx( p, plane) vtxtype *p; struct clipplane *plane;
{
    if (plane->firstvtx) {
	plane->f = *p;			/* f is first vertex of polygon */
	plane->s = *p;			/* s is previous 'saved' vertex */
	}
    else {
	if (linecross( plane, p)) {	/* if line sp crosses plane */
	    plane->s = *p;		/* compute intersect vtx i  */
	    (*(plane->procvtx))( &plane->i, plane->nextplane);
	    }
	else plane->s = *p;
	}
    if ( visible( plane)) {		/* if s on visible side of plane */
	(*(plane->procvtx))( &plane->s, plane->nextplane);
	plane->nout++;
	}
    plane->firstvtx = FALSE;
    }

/*-----------------------------------------*/
static polclos( pl) register struct clipplane *pl;
{
    if (pl->nout ) {
	if (linecross( pl, &pl->f)) {		/* handle last edge */
	    (*(pl->procvtx))( &pl->i, pl->nextplane);
	    }
	}
    pl->firstvtx = TRUE;
    pl->nout = 0;
    (*(pl->polclos))( pl->nextplane);
    }

/*--------------------------------------------*/
static outvtx( p, pl) vtxtype *p; struct clipplane *pl;
{			/* output routine from last stage of clipper */
					/* Now clamp clipped coords to view
					   volume to prevent horrible effects
					   from roundoff error.
					*/
	if ( (_core_wclipplanes & 0x3) && (u_int) (p->x + p->w) > (p->w << 1))
		if (p->x < 0)
			p->x = -p->w;
		else
			p->x = p->w;

	if ( (_core_wclipplanes & 0xC) && (u_int) (p->y + p->w) > (p->w <<1))
		if (p->y < 0)
			p->y = -p->w;
		else
			p->y = p->w;

	if ( (_core_wclipplanes &0x10) && p->z < 0) p->z = 0;
	if ( (_core_wclipplanes &0x20) && p->z > p->w) p->z = p->w;
	pl->nout++;
	_core_vtxlist[_core_vtxcount++] = *p;
}
/*--------------------------------------------*/
static polout( pl) struct clipplane *pl;
{
    if (pl->nout ) {	/* later may want to notify about end of polygon */
        }
    pl->nout = 0;
    }
	
/*----------------------------------------*/
static linecross( pl, p) struct clipplane *pl; vtxtype *p;
{
    register int shift;
    register u_int alpha, n, d;
    int svis, pvis, i;
    int *ptri, *ptrs, *ptrp;
    short sd;

    switch( pl->planeno) {
	case LEFTPLANE:
	    svis = pl->s.x + pl->s.w;		/* x-(-w) */
	    pvis = p->x + p->w;
	    break;
	case RIGHTPLANE:
	    svis = pl->s.w - pl->s.x;		/* -(x-w) */
	    pvis = p->w - p->x;
	    break;
	case BOTTOM:
	    svis = pl->s.y + pl->s.w;		/* y-(-w) */
	    pvis = p->y + p->w;
	    break;
	case TOP:
	    svis = pl->s.w - pl->s.y;		/* -(y-w) */
	    pvis = p->w - p->y;
	    break;
	case NEAR:
	    svis = pl->s.z;			/* (z-0) */
	    pvis = p->z;
	    break;
	case FAR:
	    svis = pl->s.w - pl->s.z;		/* -(z-w) */
	    pvis = p->w - p->z;
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
static visible( pl) struct clipplane *pl;
{
    int svis;

    switch( pl->planeno) {
	case LEFTPLANE:
	    svis = pl->s.x + pl->s.w;		/* x-(-w) */
	    break;
	case RIGHTPLANE:
	    svis = pl->s.w - pl->s.x;		/* -(x-w) */
	    break;
	case BOTTOM:
	    svis = pl->s.y + pl->s.w;		/* y-(-w) */
	    break;
	case TOP:
	    svis = pl->s.w - pl->s.y;		/* -(y-w) */
	    break;
	case NEAR:
	    svis = pl->s.z;			/* (z-0) */
	    break;
	case FAR:
	    svis = pl->s.w - pl->s.z;		/* -(z-w) */
	    break;
	}
    if (svis >= 0) return( TRUE);
    else return( FALSE);
    }
