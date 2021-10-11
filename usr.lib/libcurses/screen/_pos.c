#ifndef lint
static	char sccsid[] = "@(#)_pos.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

/* Position the SP->curptr to (row, column) which start at 0. */
_pos(row, column)
int	row;
int	column;
{
#ifdef DEBUG
    if(outf) fprintf(outf, "_pos from row %d, col %d => row %d, col %d\n",
	    SP->phys_y, SP->phys_x, row, column);
#endif
	if( SP->phys_x == column && SP->phys_y == row )
	{
		return;	/* already there */
	}
	/*
	 * Many terminals can't move the cursor when in standout mode.
	 * We must be careful, however, because HP's and cookie terminals
	 * will drop a cookie when we do this.
	 */
	if( !move_standout_mode && SP->phys_gr && magic_cookie_glitch < 0 )
	{
		if( !ceol_standout_glitch )
		{
			_clearhl ();
		}
	}
	/* some terminals can't move in insert mode */
	if( SP->phys_irm == 1 && !move_insert_mode )
	{
		tputs(exit_insert_mode, 1, _outch);
		SP->phys_irm = 0;
	}
	/* If we try to move outside the scrolling region, widen it */
	if( row<SP->phys_top_mgn || row>SP->phys_bot_mgn )
	{
		_window(0, lines-1, 0, columns-1);
		_setwind();
	}
	mvcur(SP->phys_y, SP->phys_x, row, column);
	SP->phys_x = column;
	SP->phys_y = row;
}
