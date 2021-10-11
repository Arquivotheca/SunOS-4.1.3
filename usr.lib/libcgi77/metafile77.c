#ifndef lint
static char	sccsid[] = "@(#)metafile77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
open_cgi
close_cgi 
close_vws
activate_vws
deactivate_vws
set_up_sigwinch
*/
#include "cgidefs.h"
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfopencgi                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfopencgi_ () 
{
    return (open_cgi ());
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfclosecgi                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfclosecgi_ ()
{
    return (close_cgi ());
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfclosevws                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfclosevws_ (name)
int *name; /* view surface name */

{
    return (close_vws (*name));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsupsig                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfsupsig_ (name,sig_function)
int *name; /* view surface name */
int     (*sig_function) ();	/* signal handling function */
{
    return (set_up_sigwinch (*name, sig_function));
}
