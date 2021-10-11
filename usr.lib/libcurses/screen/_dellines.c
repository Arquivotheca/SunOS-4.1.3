#ifndef lint
static	char sccsid[] = "@(#)_dellines.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char *tparm();

extern	int	_outch();

_dellines (n)
{
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "_dellines(%d).\n", n);
#endif
	if (lines - SP->phys_y <= n && (clr_eol && n == 1 || clr_eos)) {
		tputs(clr_eos, n, _outch);
	} else
	if (scroll_forward && SP->phys_y == SP->des_top_mgn /* &&costSF<costDL */) {
		/*
		 * Use forward scroll mode of the terminal, at
		 * the bottom of the window.  Linefeed works
		 * too, since we only use it from the bottom line.
		 */
		_setwind();
		for (i = n; i > 0; i--) {
			_pos(SP->des_bot_mgn, 0);
			tputs(scroll_forward, 1, _outch);
			SP->ml_above++;
		}
		if (SP->ml_above + lines > lines_of_memory)
			SP->ml_above = lines_of_memory - lines;
	} else if (parm_delete_line && (n>1 || *delete_line==0)) {
		tputs(tparm(parm_delete_line, n, SP->phys_y), lines-SP->phys_y, _outch);
	}
	else if (change_scroll_region && *delete_line==0) {
		/* vt100: fake delete_line by changing scrolling region */
		/* Save since change_scroll_region homes cursor */
		tputs(save_cursor, 1, _outch);
		tputs(tparm(change_scroll_region,
			SP->phys_y, SP->des_bot_mgn), 1, _outch);
		/* go to bottom left corner.. */
		tputs(tparm(cursor_address, SP->des_bot_mgn, 0), 1, _outch);
		for (i=0; i<n; i++)	/* .. and scroll n times */
			tputs(scroll_forward, 1, _outch);
		/* restore scrolling region */
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);	/* put SP->curptr back */
		SP->phys_top_mgn = SP->des_top_mgn;
		SP->phys_bot_mgn = SP->des_bot_mgn;
	}
	else {
		for (i = 0; i < n; i++)
			tputs(delete_line, lines-SP->phys_y, _outch);
	}
}
