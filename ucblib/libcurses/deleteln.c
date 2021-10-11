/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)deleteln.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/06/07 */
#endif not lint

# include	"curses.ext"

/*
 *	This routine deletes a line from the screen.  It leaves
 * (_cury,_curx) unchanged.
 *
 */
wdeleteln(win)
reg WINDOW	*win;
{
	reg char	*temp;
	reg int		y;
	reg char	*end;
	reg int		x;

# ifdef DEBUG
	fprintf(outf, "DELETELN(%0.2o)\n", win);
# endif
	temp = win->_y[win->_cury];
	for (y = win->_cury; y < win->_maxy - 1; y++) {
		if (win->_orig == NULL)
			win->_y[y] = win->_y[y + 1];
		else
			bcopy(win->_y[y + 1], win->_y[y], win->_maxx);
		touchline(win, y, 0, win->_maxx - 1);
	}
	if (win->_orig == NULL)
		win->_y[y] = temp;
	else
		temp = win->_y[y];
	for (end = &temp[win->_maxx]; temp < end; )
		*temp++ = ' ';
	touchline(win, y, 0, win->_maxx - 1);
	if (win->_orig == NULL)
		_id_subwins(win);
	return OK;
}
