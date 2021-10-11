/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scroll.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

# include	"curses.ext"

/*
 *	This routine scrolls the window up a line.
 *
 */
scroll(win)
WINDOW *win;
{
	_tscroll(win, 1);
}

int
_tscroll(win, musttouch)
register WINDOW	*win;
int musttouch;
{
	register int	i;
	register chtype	*temp;
	register int	top, bot;
	extern void memSset();

#ifdef DEBUG
	if (win == stdscr)
		if(outf) fprintf(outf, "scroll(stdscr, %d)\n", musttouch);
	else if (win == curscr)
		if(outf) fprintf(outf, "scroll(curscr, %d)\n", musttouch);
	else
		if(outf) fprintf(outf, "scroll(%x, %d)\n", win, musttouch);
#endif
	if (!win->_scroll)
		return ERR;

	/* scroll the window lines themselves up */
	top = win->_tmarg;
	bot = win->_bmarg;
	temp = win->_y[top];
	for (i = top; i < bot; i++)
		win->_y[i] = win->_y[i+1];

	/* Put a blank line in the opened up space */
	memSset(temp, (chtype)' ', win->_maxx);
	win->_y[bot] = temp;
	if (win->_cury > 0)
		win->_cury--;
	win->_need_idl = TRUE;
# ifdef DEBUG
	if(outf) fprintf(outf, "SCROLL: win [0%o], curscr [0%o], top %d, bot %d\n",win,curscr, top, bot);
# endif
	if (win->_flags&_FULLWIN &&
	    win->_tmarg == 0 &&
	    win->_bmarg == win->_maxx - 1)
		_scrdown();
	else
		musttouch = 1;
	if (musttouch)
		touchwin(win);
	return OK;
}
