/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)printw.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/06/07 */
#endif not lint

# include	<varargs.h>

/*
 * printw and friends
 *
 */

# include	"curses.ext"

/*
 *	This routine implements a printf on the standard screen.
 */
/* VARARGS1 */
printw(fmt, va_alist)
char	*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return _sprintw(stdscr, fmt, ap);
}

/*
 *	This routine implements a printf on the given window.
 */
/* VARARGS2 */
wprintw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return _sprintw(win, fmt, ap);
}
/*
 *	This routine actually executes the printf and adds it to the window
 *
 *	This code now uses the vsprintf routine, which portably digs
 *	into stdio.  We provide a vsprintf for older systems that don't
 *	have one.
 */
_sprintw(win, fmt, ap)
WINDOW	*win;
char	*fmt;
va_list	ap; {

	char	buf[512];

	vsprintf(buf, fmt, ap);
	return waddstr(win, buf);
}
