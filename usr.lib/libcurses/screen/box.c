/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)box.c 1.1 92/07/30 SMI"; /* from S5R3 1.3.1.1 */
#endif

# include	"curses.ext"

/*
 *	This routine draws a box around the given window with "vert"
 * as the vertical delimiting char, and "hor", as the horizontal one.
 *
 */

/* Defaults - might someday be terminal dependent using graphics chars */
#define DEFVERT	ACS_VLINE
#define DEFHOR  ACS_HLINE

box(win, vert, hor)
register WINDOW	*win;
chtype	vert, hor;
{
	register int	i;
	register int	endy, endx;
	register chtype	*fp, *lp;
	extern void memSset();

	if (vert == 0 || vert == '|')
		vert = DEFVERT;
	if (hor == 0 || hor == '-')
		hor = DEFHOR;

	vert |= win->_attrs;
	hor |= win->_attrs;

	endx = win->_maxx;
	endy = win->_maxy -  1;
	fp = win->_y[0];
	lp = win->_y[endy];
	memSset(&fp[0], hor, endx);
	memSset(&lp[0], hor, endx);
	endx--;
	for (i = 0; i <= endy; i++)
		win->_y[i][0] = (win->_y[i][endx] = vert);
	fp[0] =    ACS_ULCORNER | win->_attrs;
	fp[endx] = ACS_URCORNER | win->_attrs;
	lp[0] =    ACS_LLCORNER | win->_attrs;
	lp[endx] = ACS_LRCORNER | win->_attrs;
	touchwin(win);
}
