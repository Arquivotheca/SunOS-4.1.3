#ifndef lint
static char	sccsid[] = "@(#)metainit77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI initialization functions
 */

/*
int open_cgi
int close_cgi 
*/

#include "cgidefs.h"
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfactvws 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfactvws_ (name)
int *name; /* view surface name */
{
    return (activate_vws (*name));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdeactvws 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfdeactvws_	 (name)
int *name; /* view surface name */

{
    return (deactivate_vws(*name));
}

