#ifndef lint
static char	sccsid[] = "@(#)cgiwindow77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI window functions
 */


/*
vdc_extent
device_viewport
clip_indicator
clip_rectangle
*/

#include "cgidefs.h"
#include "cf77.h"
#include <sunwindow/window_hs.h>

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfvdcext                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfvdcext_ (xbot,ybot,xtop,ytop)
int 	*xbot,*ybot,*xtop,*ytop;
{
    Ccoor c1, c2;

    ASSIGN_COOR(&c1, *xbot, *ybot);
    ASSIGN_COOR(&c2, *xtop, *ytop);
    return (vdc_extent(&c1,&c2));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdevvpt                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfdevvpt_ (name,xbot,ybot,xtop,ytop)
int *name;	
int 	*xbot,*ybot,*xtop,*ytop;
{
    Ccoor c1, c2;

    ASSIGN_COOR(&c1, *xbot, *ybot);
    ASSIGN_COOR(&c2, *xtop, *ytop);
    return (device_viewport (*name,&c1,&c2));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfclipind                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfclipind_(flag)
int *flag;
{
    return (clip_indicator ((Cclip) *flag));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcliprect                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfcliprect_ (xmin, xmax, ymin, ymax)
int *xmin, *xmax, *ymin, *ymax;
{
    return ( clip_rectangle ((Cint)*xmin, (Cint)*xmax,
		(Cint)*ymin, (Cint)*ymax));
}

