/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)deleteln.c 1.1 92/07/30 SMI"; /* from S5R3 1.3.1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine deletes a line from the screen.  It leaves
 * (_cury,_curx) unchanged.
 *
 */
wdeleteln(win)
register WINDOW	*win;
{
	register chtype	*temp;
	register int	y;

	temp = win->_y[win->_cury];
	win->_need_idl = TRUE;
	/* change line ptrs to point to next line;mark all lines as changed */
	for (y = win->_cury; y < win->_maxy-1; y++) {
		win->_y[y] = win->_y[y+1];
	}

	/* fill new last line w/ spaces */
	win->_y[win->_maxy-1] = temp;
	memSset (temp, (chtype) ' ', win->_maxx);
	win->_firstch[win->_maxy-1] = 0;
	win->_lastch[win->_maxy-1] = win->_maxx - 1;
	touchwin(win);
}
