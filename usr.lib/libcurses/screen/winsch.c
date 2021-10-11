/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)winsch.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"
#include	<ctype.h>

/* Insert a character at (curx, cury). */

winsch(win, c)
register	WINDOW	*win;
chtype	c;
{
    register	int	x, endx, n, curx = win->_curx, cury = win->_cury;
    register	chtype	*wcp, a;

    a = _ATTR(c);
    c &= A_CHARTEXT;

    /* let waddch() worry about these */
    if (c == '\r' || c == '\b')
	return (waddch(win, c));

    /* with \n, in contrast to waddch, we don't clear-to-eol */
    if (c == '\n')
    {
	if (cury >= (win->_maxy-1) || cury == win->_bmarg)
	    return (wscrl(win, 1));
	else
	{
	    win->_cury++;
	    win->_curx = 0;
	    return (OK);
	}
    }

    /* with tabs or control characters, we have to do more */
    if (c == '\t')
    {
	n = (TABSIZE - (curx % TABSIZE));
	if ((curx + n) >= win->_maxx)
	    n = win->_maxx - curx;
	c = ' ';
    }
    else
    {
	if (iscntrl((int) c))
	{
	    if (curx >= win->_maxx-1)
		return (ERR);
	    n = 2;
	}
	else
	    n = 1;
    }

    /* shift right */
    endx = curx + n;
    x = win->_maxx - 1;
    wcp = win->_y[cury] + x;
    for ( ; x >= endx; --x, --wcp)
	*wcp = *(wcp-n);

    /* insert new control character */
    if (c < ' ' || c == _CTRL('?'))
    {
	*(wcp-1) = '^' | win->_attrs | a;
	*wcp = _UNCTRL(c) | win->_attrs | a;
    }
    else
    {
	/* normal characters */
	c = _WCHAR(win, c) | a;
	for ( ; x >= curx; --x, --wcp)
	    *wcp = c;
    }

    if (curx < win->_firstch[cury])
	win->_firstch[cury] = curx;
    win->_lastch[cury] = win->_maxx-1;

    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
