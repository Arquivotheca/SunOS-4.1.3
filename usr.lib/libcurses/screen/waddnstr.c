/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)waddnstr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

/* This routine adds a string starting at (_cury, _curx) */

waddnstr(win, str, i)
register	WINDOW	*win; 
register	char	*str;
int		i;
{
    register	chtype	ch;
    register	int	maxx_1 = win->_maxx - 1, cury = win->_cury,
			curx = win->_curx;
    register	chtype	**_y = win->_y;
    int			savimmed = win->_immed, savsync = win->_sync;
    int			rv = OK;

#ifdef	DEBUG
    if (outf)
    {
	if (win == stdscr)
	    fprintf(outf, "waddnstr(stdscr, ");
	else
	    fprintf(outf, "waddnstr(%o, ", win);
	fprintf(outf, "\"%s\")\n", str);
    }
#endif	/* DEBUG */

    win->_immed = win->_sync = FALSE;

    if (i < 0)
	i = MAXINT;

    while ((ch = *str++) && (i-- > 0))
    {
	/* do normal characters while not next to edge */
	if ((ch >= ' ') && (ch != _CTRL('?')) && (curx < maxx_1))
	{
	    if (curx < win->_firstch[cury])
		win->_firstch[cury] = curx;
	    if (curx > win->_lastch[cury])
		win->_lastch[cury] = curx;
	    ch = _WCHAR(win, ch);
	    _y[cury][curx] = ch;
#ifdef	_VR3_COMPAT_CODE
	    if (_y16update)
		win->_y16[cury][curx] = _TO_OCHTYPE (ch);
#endif	/* _VR3_COMPAT_CODE */
	    curx++;
	}
	else
	{
	    win->_curx = curx;
	    /* found a char that is too tough to handle above */
	    if (waddch(win, ch) == ERR)
	    {
		rv = ERR;
		break;
	    }
	    cury = win->_cury;
	    curx = win->_curx;
	}
    }

    win->_curx = curx;
    win->_flags |= _WINCHANGED;
    win->_sync = savsync;
    if (win->_sync)
	wsyncup(win);

    return ((win->_immed = savimmed) ? wrefresh(win) : rv);
}
