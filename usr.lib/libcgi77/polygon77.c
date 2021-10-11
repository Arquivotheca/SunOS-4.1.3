#ifndef lint
static char	sccsid[] = "@(#)polygon77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Polygon functions
 */

/*
polygon
partial_polygon
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpolygon                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfpolygon_ (xcoors, ycoors, n)
int	*xcoors;
int	*ycoors;
int 		*n;
{
    Ccoorlist polycoors;
    int err;

    if ((err = ALLOC_COORLIST(&polycoors, xcoors, ycoors, *n)) == NO_ERROR)
    {
	err = polygon(&polycoors);
    }
    FREE_COORLIST(&polycoors);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfppolygon                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfppolygon_ (xcoors, ycoors, n, flag)
int	*xcoors;
int	*ycoors;
int 		*n;
int		*flag;
{
    Ccoorlist polycoors;
    int err;
    
    if ((err = ALLOC_COORLIST(&polycoors, xcoors, ycoors, *n)) == NO_ERROR)
    {
	err = partial_polygon(&polycoors, (Ccflag) *flag);
    }
    FREE_COORLIST(&polycoors);
    return(err);
}
