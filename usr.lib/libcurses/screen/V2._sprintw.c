/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)V2._sprintw.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

# include	"curses_inc.h"
# include	<varargs.h>

#ifdef _VR2_COMPAT_CODE
/*
	This is only here for compatibility with SVR2 curses.
	It will go away someday. Programs should reference
	vwprintw() instead.
 */

_sprintw(win, fmt, ap)
register WINDOW	*win;
register char * fmt;
va_list ap;
{
	return vwprintw(win, fmt, ap);
}
#endif
