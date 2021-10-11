/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)mvprintw.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3.1.4 */
#endif

# include	"curses_inc.h"
# include	<varargs.h>

/*
 * implement the mvprintw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Sigh....
 *
 */

/*VARARGS*/
mvprintw(va_alist)
va_dcl
{
	register int	y, x;
	register char * fmt;
	va_list ap;

	va_start(ap);
	y = va_arg(ap, int);
	x = va_arg(ap, int);
	fmt = va_arg(ap, char *);
	return move(y, x) == OK ? vwprintw(stdscr, fmt, ap) : ERR;
}
