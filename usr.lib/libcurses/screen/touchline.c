/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)touchline.c 1.1 92/07/30 SMI"; /* from S5R3 1.1 */
#endif

# include	"curses.ext"

/*
 * make it look like a few lines in the window have been changed.
 */
touchline(win, firstline, numlines)
register WINDOW	*win;
int firstline, numlines;
{
	register int	y, maxy, maxx;

#ifdef DEBUG
	if (outf) fprintf(outf, "touchline(%x,%d,%d)\n", win, firstline, numlines);
#endif
	maxy = firstline+numlines;
	if (win->_maxy < maxy)
	    maxy = win->_maxy;
	maxx = win->_maxx - 1;
	for (y = firstline; y < maxy; y++) {
		win->_firstch[y] = 0;
		win->_lastch[y] = maxx;
	}
	win->_flags |= _WINCHANGED;
}
