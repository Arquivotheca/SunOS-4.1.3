/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)scanw.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/06/07 */
#endif not lint

# include	<varargs.h>

/*
 * scanw and friends
 *
 */

# include	"curses.ext"

/*
 *	This routine implements a scanf on the standard screen.
 */
/*VARARGS1*/
scanw(fmt, va_alist)
char	*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return _sscans(stdscr, fmt, ap);
}
/*
 *	This routine implements a scanf on the given window.
 */
/*VARARGS2*/
wscanw(win, fmt, va_alist)
WINDOW	*win;
char	*fmt;
va_dcl	{
	va_list ap;

	va_start(ap);
	return _sscans(win, fmt, ap);
}
/*
 *	This routine actually executes the scanf from the window.
 *
 *	This is really a modified version of "sscanf".  As such,
 * it assumes that sscanf interfaces with the other scanf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sscanf" uses.
 */
_sscans(win, fmt, ap)
WINDOW	*win;
char	*fmt;
va_list	ap; {

	char	buf[100];
	FILE	junk;

	junk._flag = _IOREAD|_IOSTRG;
	junk._base = junk._ptr = (unsigned char *)buf;
	if (wgetstr(win, buf) == ERR)
		return ERR;
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, ap);
}
