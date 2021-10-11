/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wvline.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

wvline(win, vertch, num_chars)
register	WINDOW	*win;
chtype	vertch;
int	num_chars;
{
    int     cury = win->_cury, curx = win->_curx;
    chtype  a, **fp = win->_y;
    short   *firstch = &(win->_firstch[cury]), *lastch = &(win->_lastch[cury]);

    if (vertch == 0)
	vertch = ACS_VLINE;
    a = _ATTR(vertch);
    vertch = _WCHAR(win, vertch) | a;
    for (num_chars += cury; cury < num_chars; cury++, firstch++, lastch++)
    {
	fp[cury][curx] = vertch;
	if (curx < *firstch)
	    *firstch = curx;
	if (curx > *lastch)
	    *lastch = curx;
    }
    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
