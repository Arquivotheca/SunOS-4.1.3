/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wscanw.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7 */
#endif

/*
 * scanw and friends
 *
 */

# include	"curses_inc.h"
# include	<varargs.h>

/*
 *	This routine implements a scanf on the given window.
 */
/*VARARGS*/
wscanw(va_alist)
va_dcl
{
	register WINDOW	*win;
	register char	*fmt;
	va_list	ap;

	va_start(ap);
	win = va_arg(ap, WINDOW *);
	fmt = va_arg(ap, char *);
	return vwscanw(win, fmt, ap);
}
