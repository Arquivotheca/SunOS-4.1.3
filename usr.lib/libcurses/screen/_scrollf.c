#ifndef lint
static	char sccsid[] = "@(#)_scrollf.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

/*
 * Scroll the terminal forward n lines, bringing up blank lines from bottom.
 * This only affects the current scrolling region.
 */
_scrollf(n)
int n;
{
	register int i;

	if( scroll_forward )
	{
		_setwind();
		_pos( SP->des_bot_mgn, 0 );
		for( i=0; i<n; i++ )
		{
			tputs(scroll_forward, 1, _outch);
		}
		SP->ml_above += n;
		if( SP->ml_above + lines > lines_of_memory )
		{
			SP->ml_above = lines_of_memory - lines;
		}
	}
	else
	{
		/* If terminal can't do it, try delete line. */
		_pos(0, 0);
		_dellines(n);
	}
}
