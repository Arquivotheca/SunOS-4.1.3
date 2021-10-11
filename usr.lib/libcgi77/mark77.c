#ifndef lint
static char	sccsid[] = "@(#)mark77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI marker attribute functions
 */

/*
marker_type
marker_size
marker_color 
marker_size_specification_mode
*/
#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfmktype                                          	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfmktype_ (ttyp)
int *ttyp;		/* style of marker */
{
return (marker_type (*ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfmksize                                          	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int   cfmksize_	(index)
float   *index;			/* marker size */
{
	return (  marker_size (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfmkcolor                                         	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfmkcolor_ (index)
int     *index;			/* marker color */
{
	return (marker_color (*index));
}

/****************************************************************************/
/*                          						    */
/*     FUNCTION: cfmksizespecmode & cfmksizespecmode_			    */
/*                                                                          */
/*              NOTE: underbar '_' is missing because FORTRAN uses 16       */
/*                    characters from name and follows it with no '_'.      */
/*		Note2: This was fixed in Fortran 1.1, so added a new entry  */
/*                                                                          */
/****************************************************************************/

int cfmksizespecmode (mode)
int *mode;		/* pixels or percent */
{
	return(marker_size_specification_mode (*mode));
}

int cfmksizespecmode_ (mode)
int *mode;		/* pixels or percent */
{
	return(marker_size_specification_mode (*mode));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfmkspecmode						    */
/*                                                                          */
/*              NOTE: Both cfmkspecmode and cfmksizespecmode are	    */
/*                    included because cfqoutcap() lists the name as	    */
/*                    cfmkspecmode, which matches post-3.0 documentation,   */
/*                    while pre-3.0 documentation listed the name as	    */
/*                    cfmksizespecmode, which is the correct name.	    */
/*                                                                          */
/****************************************************************************/

int cfmkspecmode_ (mode)
int *mode;		/* pixels or percent */
{
	return(marker_size_specification_mode (*mode));
}
