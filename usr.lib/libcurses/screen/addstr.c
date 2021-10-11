/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)addstr.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

# include	"curses.ext"

/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 */
waddstr(win,str)
register WINDOW	*win; 
register char	*str;
{
	register chtype ch, attrs;
	register int maxx_1, cury, curx;
	register chtype **_y;

# ifdef DEBUG
	if(outf) {
		if (win == stdscr)
			fprintf(outf, "WADDSTR(stdscr, ");
		else
			fprintf(outf, "WADDSTR(%o, ", win);
		fprintf(outf, "\"%s\")\n", str);
	}
# endif

	if (!win)
		return ERR;

	maxx_1 = win->_maxx - 1;
	_y = win->_y;
	attrs = win->_attrs;
	while (ch = *str) {
		cury = win->_cury;
		curx = win->_curx;
		/* do normal characters while not next to edge */
		for ( ; (ch >= ' ') && (ch <= '~') &&
			(curx < maxx_1) ; ch = *++str) {
			if (win->_firstch[cury] == _NOCHANGE)
				win->_firstch[cury] =
					win->_lastch[cury] = curx;
			else if (curx < win->_firstch[cury])
				win->_firstch[cury] = curx;
			else if (curx > win->_lastch[cury])
				win->_lastch[cury] = curx;
			_y[cury][curx++] = ch | attrs;
		}
		win->_curx = curx;
		/* found a char that is too tough to handle above */
		if (ch)
			if (waddch(win, ch) == ERR)
				return ERR;
			else
				str++;
	}
	win->_flags |= _WINCHANGED;
	return OK;
}
