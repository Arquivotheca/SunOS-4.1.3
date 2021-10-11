/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wbkgd.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

/* Change the background of a window.  nbkgd :	new background. */

wbkgd(win, nbkgd)
register	WINDOW	*win;
chtype	nbkgd;
{
    register	int	x, y, maxx;
    register	chtype	*wcp, obkgda, obkgdc, nbkgda, nbkgdc, c;
    register	short	*begch, *endch;

    if (nbkgd == win->_bkgd)
	return (OK);

    obkgdc = _CHAR(win->_bkgd);
    obkgda = _ATTR(win->_bkgd);

    nbkgdc = _CHAR(nbkgd);
    nbkgda = _ATTR(nbkgd);
    if (nbkgdc < ' ' || nbkgdc == _CTRL('?'))
	nbkgdc = obkgdc, nbkgd = nbkgdc|nbkgda;

    win->_bkgd = nbkgd;
    win->_attrs = _ATTR((win->_attrs & ~obkgda) | nbkgda);

    maxx = win->_maxx - 1;
    begch = win->_firstch;
    endch = win->_lastch;
    for (y = win->_maxy-1; y >= 0; --y, ++begch, ++endch)
    {
	for (x = maxx, wcp = win->_y[y]; x-- >= 0; ++wcp)
	{
	    if ((c = _CHAR(*wcp)) == obkgdc)
		c = nbkgdc;
	    *wcp = c | _ATTR((*wcp & ~obkgda) | nbkgda);
	}
	*begch = 0;
	*endch = maxx;
    }

    win->_flags |= _WINCHANGED;
    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
