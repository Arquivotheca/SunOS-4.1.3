/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)mvscanw.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/06/07 */
#endif not lint

# include	<varargs.h>

# include	"curses.ext"

/*
 * implement the mvscanw commands.  Due to the variable number of
 * arguments, they cannot be macros.  Another sigh....
 *
 */

/* VARARGS3 */
mvscanw(y, x, fmt, va_alist)
reg int		y, x;
char		*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return move(y, x) == OK ? _sscans(stdscr, fmt, ap) : ERR;
}

/*VARARGS4*/
mvwscanw(win, y, x, fmt, va_alist)
reg WINDOW	*win;
reg int		y, x;
char		*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return wmove(win, y, x) == OK ? _sscans(win, fmt, ap) : ERR;
}
