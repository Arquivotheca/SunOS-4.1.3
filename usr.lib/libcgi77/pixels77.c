#ifndef lint
static char	sccsid[] = "@(#)pixels77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Raster Output Primitives
 */

/*
cell_array
pixel_array
inquire_cell_array
inquire_pixel_array
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcellarr 					    */
/*                                                                          */
/*                                                                          */
/*	p is the lower left-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtural size of the array  which is  */
/*	mapped onto the area defined by p,q, and r.			    */
/****************************************************************************/

int cfcellarr_ (px, qx, rx, py, qy, ry, dx, dy, colorind)
int *px,*py;
int *qx,*qy;
int *rx,*ry;
int *dx, *dy;			/* size of color array */
int colorind[];		/* array of color values */
{
    Ccoor pcell, qcell, rcell;

    ASSIGN_COOR(&pcell, *px, *py);
    ASSIGN_COOR(&rcell, *rx, *ry);
    ASSIGN_COOR(&qcell, *qx, *qy);
    return (cell_array (&pcell, &qcell, &rcell, *dx, *dy, colorind));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpixarr 					    */
/*                                                                          */
/*                                                                          */
/*              M and N define the size of the array.                       */
/****************************************************************************/

int cfpixarr_ (px,py, m, n, colorind)
int *px,*py;
int *m, *n;			/* dimensions of color array */
int colorind[];		/* array of color values */

{
    Ccoor pcell;		/* base of array in VDC space */

    ASSIGN_COOR(&pcell, *px, *py);
    return (pixel_array (&pcell, *m, *n, colorind));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqcellarr					    */
/*                                                                          */
/*                                                                          */
/*	p is the lower left-hand corner. r is one of the remaining two 	    */
/*	corners. Dx and dy define the virtural size of the array  which is  */
/*	mapped onto the area defined by p,q, and r.			    */
/****************************************************************************/

int cfqcellarr_ (name, px, qx, rx, py, qy, ry, dx, dy, colorind)
int *name;
int *px,*py;
int *qx,*qy;
int *rx,*ry;
int *dx, *dy;			/* size of color array */
int colorind[];		/* array of color values */
{
	Ccoor pcell, qcell, rcell;	/* points of parallelogram */

    ASSIGN_COOR(&pcell, *px, *py);
    ASSIGN_COOR(&rcell, *rx, *ry);
    ASSIGN_COOR(&qcell, *qx, *qy);
    return(inquire_cell_array (*name, &pcell, &qcell, &rcell,
		*dx, *dy, colorind));
}

/****************************************************************************/
/*     FUNCTION: cfqpixarr                                                  */
/*                                                                          */
/*              M and N define the size of the array.                       */
/****************************************************************************/

int cfqpixarr_ (px,py, m, n, colorind,name)
int *px,*py;
int *m, *n;			/* dimensions of color array */
int colorind[];		/* array of color values */
int *name;
{
    Ccoor pcell;		/* base of array in VDC space */

    ASSIGN_COOR(&pcell, *px, *py);
    return(inquire_pixel_array (&pcell, *m, *n, colorind, *name));
}
