/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wtouchln.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

/*
 * Make a number of lines look like they have/have not been changed.
 * y: the start line
 * n: the number of lines affected
 * changed:	1: changed
 * 		0: not changed
 * 		-1: changed. Called internally - In this mode
 * 		    even REDRAW lines are changed.
 */

wtouchln(win, y, n, changed)
WINDOW	*win;
int	y, n, changed;
{
    register	short	*firstch, *lastch;
    register	int	b, e, maxy = win->_maxy;

    if (y >= maxy)
	return (ERR);
    if (y < 0)
	y = 0;
    if ((y + n) > maxy)
	n = maxy - y;
    firstch = win->_firstch + y;
    lastch = win->_lastch + y;
    if (changed)
    {
	win->_flags |= _WINCHANGED;
	b = 0;
	e = win->_maxx - 1;
    }
    else
    {
	b = _INFINITY;
	e = -1;
	win->_flags &= ~_WINCHANGED;
    }

    for ( ; n-- > 0; firstch++, lastch++)
    {
	if (changed == -1 || *firstch != _REDRAW)
	    *firstch = b, *lastch = e;
    }

    if ((changed == 1) && win->_sync)
	wsyncup(win);

    return ((changed == 1) && win->_immed) ? wrefresh(win) : OK;
}
