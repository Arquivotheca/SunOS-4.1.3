#ifndef lint
static	char sccsid[] = "@(#)_tscroll.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"

_tscroll( win )
register WINDOW	*win;
{

	register chtype	*sp;
	register int		i;
	register chtype	*temp;
	register int	top, bot;

#ifdef DEBUG
	if( win == stdscr )
	{
		if(outf) fprintf( outf, "scroll( stdscr )\n" );
	}
	else
	{
		if( win == curscr)
		{
			if(outf) fprintf( outf, "scroll( curscr )\n" );
		}
		else
		{
			if(outf) fprintf( outf, "scroll( %x )\n", win );
		}
	}
#endif
	if( !win->_scroll )
	{
		return ERR;
	}
	/* scroll the window lines themselves up */
	top = win->_tmarg;
	bot = win->_bmarg;
	temp = win->_y[top];
#ifdef	DEBUG
	if(outf)
	{
		fprintf( outf, "top = %d, bot = %d\n", top, bot );
	}
#endif	DEBUG
	for (i = top; i < bot; i++)
	{
		win->_y[i] = win->_y[i+1];
	}
	/* Put a blank line in the opened up space */
	for (sp = temp; sp - temp < win->_maxx; )
		*sp++ = ' ';
	win->_y[bot] = temp;
#ifdef	DEBUG
	if(outf)
	{
		fprintf(outf,"SCROLL win [0%o], curscr [0%o], top %d, bot %d\n",
				win, curscr, top, bot);
		fprintf( outf, "Doing --> touchwin( 0%o )\n", win );
	}
#endif	DEBUG
	if (win->_cury > 0)
		win->_cury--;
/*
**	This section taken out because it wasn't allowing the scrolling of a
**	region smaller than the full screen.  Taken out on 10/12/83.  The 
**	function call touchwin(win) is done all the time now, & seems to do
**	the correct thing when it comes to scrolling with a smaller than screen
**	sized region, & a screen sized region.
**
**	if( win->_flags&_FULLWIN )
**	{
**		_scrdown();
**	}
**	else
**	{
**		musttouch = 1;
**	}
**	if( musttouch )
**		touchwin(win);
*/
	touchwin(win);
	return OK;
}
