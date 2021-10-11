/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)V2.__sscans.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6 */
#endif

# include	"curses_inc.h"
# include	<varargs.h>
#ifdef _VR2_COMPAT_CODE
/*
	This file is provided for compatibility reasons only
	and will go away someday. Programs should reference
	vwscanw() instead.
 */


extern int vwscanw();
__sscans(win, fmt, ap)
WINDOW	*win;
char *fmt;
va_list	ap;
{
	return vwscanw(win, fmt, ap);
}
#endif
