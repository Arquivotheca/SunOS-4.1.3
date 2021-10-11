#ifndef lint
static char	sccsid[] = "@(#)pixels277.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfbtblsouarr                                               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfbtblsouarr_ (bitsource, xo, yo, xe, ye, bittarget, xt, yt, name)
int **bitsource, **bittarget;		/* source and target bitmaps */
int  *xo, *yo, *xe, *ye, *xt, *yt;	/* coordinates of bitmaps */
int  *name;				/* view surface name */
{
    return( bitblt_source_array (*bitsource, *xo, *yo, *xe, *ye,
			    *bittarget, *xt, *yt, *name));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfbtblpatarr                                               */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfbtblpatarr_ (pixpat, px, py, pixtarget, rx, ry, ox, oy, dx, dy, name)
int    **pixpat;			/* pattern source array */
int     *px, *py;			/* pattern extent */
int    **pixtarget;			/* destination pattern array */
int     *rx, *ry;			/* pattern reference point */
int     *ox, *oy;			/* destination origin */
int     *dx, *dy;			/* destination extent */
int	*name;				/* view surface name */
{
	return( bitblt_pattern_array (*pixpat, *px, *py, *pixtarget,
				    *rx, *ry, *ox, *oy, *dx, *dy, *name));
}

/****************************************************************************/
/*                                                                          */
/****************************************************************************/
int cfbtblpatsouarr_ (pixpat, px, py, pixsource, sx, sy, 
			pixtarget, rx, ry, ox, oy, dx, dy, name)
int **pixpat;			/* pattern source array */
int     *px, *py;			/* pattern extent */
int **pixsource;		/* source array */
int     *sx, *sy;			/* source origin */
int **pixtarget;		/* destination pattern array */
int     *rx, *ry;			/* pattern reference point */
int     *ox, *oy;			/* destination origin */
int     *dx, *dy;			/* destination extent */
int	*name;			/* view surface name */
{
    return( bitblt_patterned_source_array (*pixpat, *px, *py,
				*pixsource, *sx, *sy,
				*pixtarget, *rx, *ry, *ox, *oy, *dx, *dy,
				*name));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsgldrawmode                                              */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfsgldrawmode_ (combination)
int *combination;		/* combination rules */
{
    return(set_global_drawing_mode (*combination));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsdrawmode                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfsdrawmode_(visibility, source, destination, combination)
int *visibility;		/* transparent or opaque */
int *source;		/* NOT source bits */
int *destination;		/* NOT destination bits */
int *combination;		/* combination rules */
{
    return (set_drawing_mode (*visibility, *source, *destination, *combination));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqdrawmode                                                */
/*                                                                          */
/****************************************************************************/

int cfqdrawmode_ (visibility, source, destination, combination)
int * visibility;		/* transparent or opaque */
int * source;		/* NOT source bits */
int * destination;	/* NOT destination bits */
int * combination;		/* combination rules */
{
    return (inquire_drawing_mode((Cbmode *)visibility, (Cbitmaptype *)source,
		(Cbitmaptype *)destination, (Ccombtype *)combination));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqdevbtmp                                                 */
/*                                                                          */
/****************************************************************************/

int cfqdevbtmp_  (name,map)
int	*name;		/* view surface name */
int	**map;		/* return value for pixrect pointer */
{
    *map = (int*) (inquire_device_bitmap (*name));
    if (*map == NULL)	return(EVSIDINV);
    else		return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqbtblalign                                               */
/*                                                                          */
/****************************************************************************/

int     cfqbtblalign_(base, width, px, py, maxpx, maxpy, name)
int    *base;		/* bitmap base alignment */
int    *width;		/* width alignment */
int    *px, *py;		/* pattern extent alignment */
int    *maxpx, *maxpy;	/* maximum pattern size */
int	*name;
{
    return (inquire_bitblt_alignments (base, width, px, py, maxpx, maxpy,
					*name));
}
