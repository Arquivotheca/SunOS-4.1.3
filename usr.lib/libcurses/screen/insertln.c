/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)insertln.c 1.1 92/07/30 SMI"; /* from S5R3 1.3.1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine performs an insert-line on the _window, leaving
 * (_cury,_curx) unchanged.
 *
 */
winsertln(win)
register WINDOW	*win;
{
	register chtype	*temp;
	register int	y;

	temp = win->_y[win->_maxy-1];
	win->_firstch[win->_cury] = 0;
	win->_lastch[win->_cury] = win->_maxx - 1;
	win->_need_idl = TRUE;
	win->_flags |= _WINCHANGED;
	for (y = win->_maxy - 1; y > win->_cury; --y) {
		win->_y[y] = win->_y[y-1];
	}

	win->_y[win->_cury] = temp;
	memSset(temp, (chtype) ' ', win->_maxx);

	if (win->_cury == lines - 1 && win->_y[lines-1][COLS-1] != ' ')
		if (win->_scroll && !(win->_flags&_ISPAD)) {
			wrefresh(win);
			scroll(win);
			win->_cury--;
		}
		else
			return ERR;
	touchwin(win);
	return OK;
}
