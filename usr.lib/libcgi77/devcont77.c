#ifndef lint
static char	sccsid[] = "@(#)devcont77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Device control functions
 */

/*
hard_reset
reset_to_defaults
clear_view_surface
clear_control
set_error_warning_mask
*/

#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfhardrst	 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfhardrst_() 
{
return (hard_reset ());
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfrsttodefs 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfrsttodefs_ () 
{
    return (reset_to_defaults ());
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfclrvws					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfclrvws_ (name,defflag,color)
int *name; /* name assigned to cgi view surface */
int *defflag;	/* default color flag */
int *color;
{
    return (clear_view_surface(*name,(Cflag)*defflag, (Cint) *color));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfclrcont 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfclrcont_ (soft, hard, intern, extent)
int *soft, *hard;	/* soft-copy action, hard-copy action */
int *intern;			/* internal action  */
int *extent;			/* clear extent */
{
    return (clear_control ((Cacttype) *soft, (Cacttype) *hard,
    (Cacttype) *intern, (Cexttype) *extent));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfserrwarnmk				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfserrwarnmk_  (action)
int *action;		/* No action, poll, or interrupt */
{
    return (set_error_warning_mask ((Cerrtype)  *action));
}
