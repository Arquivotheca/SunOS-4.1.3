/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)whline.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

whline(win, horch, num_chars)
register	WINDOW	*win;
chtype		horch;
int		num_chars;
{
    int     cury = win->_cury, curx = win->_curx;
    chtype  a, *fp = &(win->_y[cury][curx]);

    if (horch == 0)
	horch = ACS_HLINE;
    a = _ATTR(horch);
    horch = _WCHAR(win, horch) | a;
    memSset (fp, horch | win->_attrs, num_chars);
    if (curx < win->_firstch[cury])
	win->_firstch[cury] = curx;
    if ((curx += (num_chars - 1)) > win->_lastch[cury])
	win->_lastch[cury] = curx;
    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
