/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)waddchnstr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

/*
 * Add ncols worth of data to win, using string as input.
 * Return the number of chtypes copied.
 */
waddchnstr(win, string, ncols)
register	WINDOW	*win;
chtype		*string;
int		ncols;
{
    int		my_x = win->_curx;
    int		my_y = win->_cury;
    int		my_maxx;
    int		counter;
    chtype	*ptr = &(win->_y[my_y][my_x]);

    /* If ncols == -1 then search for NULL and set ncols */

    if (ncols == -1)
    {
	chtype	*sptr = ptr;

	for (counter = win->_maxx - my_x; *string && counter--; )
	    *ptr++ = *string++;
	my_maxx = ptr - sptr;
    }
    else
    {
	/*
	 * Adjust ncols if it will exceed the maximum for the
	 * window being referenced
	 */
	if (my_x + ncols > win->_maxx)
	    ncols = win->_maxx - my_x;
    
	my_maxx = my_x + ncols;
    
	(void) memcpy((char *)ptr, (char *)string, (int) (ncols * sizeof(chtype)));
    }

    if (my_x < win->_firstch[my_y])
	win->_firstch[my_y] = my_x;

    if (my_maxx > win->_lastch[my_y])
	win->_lastch[my_y] = my_maxx;

    win->_flags |= _WINCHANGED;

    /* sync with ancestor structures */
    if (win->_sync)
	wsyncup(win);

    return (win->_immed ? wrefresh(win) : OK);
}
