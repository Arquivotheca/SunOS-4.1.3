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
static char sccsid[] = "@(#)clipvertex.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
	Polygon clipper for 3-D polygons.  Uses Sutherland Hodgeman algorithm
	in 'Reentrant Polygon Clipping', CACM 17:1, January 1974.
	Two function calls are included clipvertex( p) must be called for
	each vertex of a polygon and clipend() is called to terminate the
	polygon.  Output vertices are written to the global array 'vtxlist'.
	*/

/*------------globals---------------------*/
#define TRUE	1
#define FALSE	0
#define LEFT	0
#define RIGHT	1
#define BOTTOM	2
#define TOP	3
#define NEAR	4
#define FAR	5
#define VTXSIZE	7

static int procvtx(), polclos(), outvtx(), polout();

typedef struct {			/* vertex, NDC coordinates */
    int x,y,z,w;
    int dx,dy,dz,dw;			/* unit normal at vertex 20b.10b */
    } vtxtype;

extern vtxtype _core_vtxlist[];		/***** OUTPUT VERTEX LIST *****/
extern short _core_vtxcount;


static struct clipplane {
    struct clipplane *nextplane;	/* ptr to data for next clip plane */
    int (*procvtx)();			/* vertex processing routine */
    int (*polclos)();			/* close-polygon-entry routine */
    short firstvtx;			/* TRUE if this is 1st vertex  */
    short planeno;			/* clip plane number for this plane */
    short nout;				/* no. vtxs output by this stage */
    vtxtype f,			/* 1st vertex of polygon */
	    s,				/* saved previous vtx of current pol */
	    i;				/* intersect of edge with clip plane */
    } planes[6]	= {	/* 6 clipping planes */
	    &planes[1], procvtx, polclos, TRUE, LEFT, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    &planes[2], procvtx, polclos, TRUE, RIGHT, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    &planes[3], procvtx, polclos, TRUE, BOTTOM, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    &planes[4], procvtx, polclos, TRUE, TOP, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    &planes[5], procvtx, polclos, TRUE, NEAR, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    &planes[0], outvtx, polout,   TRUE, FAR, 0,
	    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	    };

/*-------------------------------------------
printplanes()
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
clipvertex( p) vtxtype *p;
{				/* hand this routine each vertex */
    procvtx( p, &planes[0]);
    }
/*------------------------------------*/
clipend()			/* this routine terminates polygon */
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
    int svis, pvis, diff, i;
    int *ptri, *ptrs, *ptrp;

    switch( pl->planeno) {
	case LEFT:
	    svis = pl->s.x + pl->s.w;		/* x-(-w) */
	    pvis = p->x + p->w;
	    break;
	case RIGHT:
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
	diff = svis - pvis;			/* opposite sides of plane */
	ptri = &pl->i.x;
	ptrs = &pl->s.x;
	ptrp = &p->x;
	for (i=0; i<VTXSIZE; i++){		/* compute intersection vrtx */
	    *ptri++ = ((*ptrp++ * svis) - (*ptrs++ * pvis)) / diff;
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
	case LEFT:
	    svis = pl->s.x + pl->s.w;		/* x-(-w) */
	    break;
	case RIGHT:
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
