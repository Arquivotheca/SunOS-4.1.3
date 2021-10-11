/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)winsnstr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

/*
 * Insert to 'win' at most n chars of a string
 * starting at (cury, curx). However, if n <= 0,
 * insert the entire string.
 * \n, \t, \r, \b are treated in the same way
 * as other control chars.
 */

winsnstr(win, sp, n)
register	WINDOW	*win;
char	*sp;
int	n;
{
    register	chtype	*wcp;
    register	int	x, cury, endx, maxx, len;

    if (n < 0)
	n = MAXINT;

    /* compute total length of the insertion */
    endx = win->_curx;
    maxx = win->_maxx;
    for (x = 0; sp[x] != '\0' && x < n && endx < maxx; ++x)
    {
	endx += (sp[x] < ' '|| sp[x] == _CTRL('?')) ? 2 : 1;
	if (endx > maxx)
	{
	    endx -= 2;
	    break;
	}
    }

    /* length of chars to be shifted */
    if ((len = endx - win->_curx) <= 0)
	return (ERR);

    /* number of chars insertible */
    n = x;

    /* shift data */
    cury = win->_cury;
    x = maxx - 1;
    wcp = win->_y[cury] + x;
    for ( ; x >= endx; --x, --wcp)
	*wcp = *(wcp - len);

    /* insert new data */
    wcp = win->_y[cury] + win->_curx;
    for ( ; n > 0; --n, ++wcp, ++sp)
    {
	if (*sp < ' ' || *sp == _CTRL('?'))
	{
	    *wcp++ = _CHAR('^') | win->_attrs;
	    *wcp = _CHAR(_UNCTRL(*sp)) | win->_attrs;
	}
	else
	    *wcp = _CHAR(*sp) | win->_attrs;
    }

    /* update the change structure */
    if (win->_firstch[cury] > win->_curx)
	win->_firstch[cury] = win->_curx;
    win->_lastch[cury] = maxx - 1;

    win->_flags |= _WINCHANGED;

    if (win->_sync)
	wsyncup(win);
    return (win->_immed ? wrefresh(win) : OK);
}
