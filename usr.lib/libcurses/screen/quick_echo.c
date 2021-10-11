/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)quick_echo.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.12 */
#endif

#include	"curses_inc.h"

extern	int	outchcount;

/*
 *  These routines short-circuit much of the innards of curses in order to get
 *  a single character output to the screen quickly! It is used by waddch().
 */

_quick_echo(win, ch)
register	WINDOW	*win;
chtype			ch;
{
    extern	int	_outch();
    int		y = win->_cury;
    register	int	SPy = y + win->_begy + win->_yoffset;
    register	int	SPx = (win->_curx - 1) + win->_begx;
    register	chtype	rawc = _CHAR(ch), rawattrs = _ATTR(ch);

    if ((curscr->_flags & _CANT_BE_IMMED) || (win->_flags & _WINCHANGED) ||
	(win->_clear) || (curscr->_clear) || (_virtscr->_flags & _WINCHANGED) ||
	(SPy > ((LINES + SP->Yabove) - 1)) || (SPx > (COLS - 1)) ||
	(SP->slk && (SP->slk->_changed == TRUE)))
    {
	win->_flags |= _WINCHANGED;
	return (wrefresh (win));
    }

    outchcount = 0;
    win->_firstch[y] = _INFINITY;
    win->_lastch[y] = -1;
    /* If the cursor is not in the right place, put it there! */
    if ((SPy != curscr->_cury) || (SPx != curscr->_curx))
    {
	(void) mvcur (curscr->_cury, curscr->_curx, SPy, SPx);
	curscr->_cury = SPy;
    }
    curscr->_curx = SPx + 1;
    _CURHASH[SPy] = _NOHASH;
    if (ch != ' ')
    {
	if (SPx > _ENDNS[SPy])
	    _ENDNS[SPy] = SPx;
	if (SPx < _BEGNS[SPy])
	    _BEGNS[SPy] = SPx;
    }
    _virtscr->_y[SPy][SPx] = curscr->_y[SPy][SPx] = ch;

    if (rawattrs != curscr->_attrs)
	_VIDS(rawattrs, curscr->_attrs);

    if (SP->phys_irm)
	_OFFINSERT();

    /* Write it out! */
    _outch((chtype) rawc);
    (void) fflush(SP->term_file);

    return (outchcount);
}
