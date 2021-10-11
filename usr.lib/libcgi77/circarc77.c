#ifndef lint
static char	sccsid[] = "@(#)circarc77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI circular arc functions
 */

/*
circular_arc
circular_arc_close
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcircarccent		 				    */
/*                                                                          */
/*                                                                          */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/

int     cfcircarccent_ (c1x, c1y, c2x, c2y, c3x, c3y, rad)
int     *c1x,*c1y,*c2x, *c2y, *c3x, *c3y;	/* endpoints */
int   *rad;			/* radius */
{
    Ccoor c1;			/* center */

    ASSIGN_COOR(&c1, *c1x, *c1y);
    return (circular_arc_center (&c1, (Cint) *c2x, (Cint) *c2y,
		(Cint) *c3x, (Cint) *c3y, (Cint) *rad));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcircarcthree		 				    */
/*                                                                          */
/*                                                                          */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/

int     cfcircarcthree_ (c1x, c1y,  c2x, c2y, c3x, c3y)
int *c1x, *c1y,  *c2x, *c2y, *c3x, *c3y;
{
    Ccoor c1, c2, c3;		/* starting, intermediate and ending
				   points */
    ASSIGN_COOR(&c1, *c1x, *c1y);
    ASSIGN_COOR(&c2, *c2x, *c2y);
    ASSIGN_COOR(&c3, *c3x, *c3y);
    return (circular_arc_3pt (&c1, &c2, &c3));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcircarccentcl 					    */
/*                                                                          */
/*                                                                          */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/****************************************************************************/

int     cfcircarccentcl_ (c1x, c1y, c2x, c2y, c3x, c3y, rad, close)
int     *c1x, *c1y, *c2x, *c2y, *c3x, *c3y;	/* endpoints */
int   *rad;			/* radius */
int *close;		/* PIE or CHORD */

{
Ccoor c1;			/* center */

    ASSIGN_COOR(&c1, *c1x, *c1y);
    return (circular_arc_center_close (&c1, (Cint) *c2x, (Cint) *c2y,
		(Cint) *c3x, (Cint) *c3y, (Cint) *rad,
		(Cclosetype) *close));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcircarcthreecl & cfcircarcthreecl_			    */
/*                                                                          */
/*                                                                          */
/*              points c2,c3. Arc is closed with either PIE or CHORD. 	    */
/*              NOTE: underbar '_' is missing because FORTRAN uses 16       */
/*                    characters from name and follows it with no '_'.      */
/*		Note2: This was fixed in Fortran 1.1, so added a new entry  */
/****************************************************************************/

int     cfcircarcthreecl (c1x, c1y,  c2x, c2y, c3x, c3y,close)
int *c1x, *c1y,  *c2x, *c2y, *c3x, *c3y;
int *close;		/* PIE or CHORD */
{
    Ccoor c1, c2, c3;		/* starting, intermediate and ending
				   points */
    ASSIGN_COOR(&c1, *c1x, *c1y);
    ASSIGN_COOR(&c2, *c2x, *c2y);
    ASSIGN_COOR(&c3, *c3x, *c3y);
    return (circular_arc_3pt_close (&c1, &c2, &c3, (Cclosetype) *close));
}

int     cfcircarcthreecl_ (c1x, c1y,  c2x, c2y, c3x, c3y,close)
int *c1x, *c1y,  *c2x, *c2y, *c3x, *c3y;
int *close;		/* PIE or CHORD */
{
    Ccoor c1, c2, c3;		/* starting, intermediate and ending
				   points */
    ASSIGN_COOR(&c1, *c1x, *c1y);
    ASSIGN_COOR(&c2, *c2x, *c2y);
    ASSIGN_COOR(&c3, *c3x, *c3y);
    return (circular_arc_3pt_close (&c1, &c2, &c3, (Cclosetype) *close));
}

