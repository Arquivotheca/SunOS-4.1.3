#ifndef lint
static	char sccsid[] = "@(#)clreolinln.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_clearhl();
extern	int	_dellines();
extern	int	_inslines();
extern	int	_outch();
extern	int	_pos();
extern	int	_setwind();
extern	int	clreol();
extern	char	*tparm();
extern	int	tputs();

_clreol()
{
	register int col;
	register struct line *cb;

#ifdef DEBUG
	if(outf) fprintf(outf, "_clreol().\n");
#endif
	if (!move_standout_mode)
		_clearhl ();
	if (clr_eol) {
		tputs (clr_eol, columns-SP->phys_x, _outch);
	} else if (delete_line && insert_line && SP->phys_x == 0) {
		_dellines (1);
		_inslines (1);
	} else {
		/*
		 * Should consider using delete/insert line here, too,
		 * if the part that would have to be redrawn is short,
		 * or if we have to get rid of some cemented highlights.
		 * This might win on the DTC 382 or the Teleray 1061.
		 */
		cb = SP->cur_body[SP->phys_y+1];
		for (col=SP->phys_x+1; col<cb->length; col++)
			if (cb->body[col] != ' ') {
				_pos(SP->phys_y, col);
				_blanks(1);
			}
		/*
		 * Could save and restore SP->curptr position, but since we
		 * keep track of what _blanks does to it, it turns out
		 * that this is wasted.  Not backspacing over the stuff
		 * cleared out is a real win on a super dumb terminal.
		 */
	}
}

_inslines (n)
{
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "_inslines(%d).\n", n);
#endif
	if (!move_standout_mode && SP->phys_gr)
		_clearhl();
	if (SP->phys_y + n >= lines && clr_eos) {
		/*
		 * Really quick -- clear to end of screen.
		 */
		tputs(clr_eos, lines-SP->phys_y, _outch);
		return;
	}

	/*
	 * We are about to _shove something off the bottom of the screen.
	 * Terminals with extra memory keep this around, and it might
	 * show up again to haunt us later if we do a delete line or
	 * scroll into it, or when we exit.  So we clobber it now.  We
	 * might get better performance by somehow remembering that this
	 * stuff is down there and worrying about it if/when we have to.
	 * In particular, having to do this extra pair of SP->curptr motions
	 * is a lose.
	 */
	if (memory_below) {
		int save;

		save = SP->phys_y;
		if (save_cursor && restore_cursor)
			tputs(save_cursor, 1, _outch);
		if (clr_eos) {
			_pos(lines-n, 0);
			tputs(clr_eos, n, _outch);
		} else {
			for (i=0; i<n; i++) {
				_pos(lines-n+i, 0);
				_clreol();
			}
		}
		if (save_cursor && restore_cursor)
			tputs(restore_cursor, 1, _outch);
		else
			_pos(save, 0);
	}

	/* Do the physical line deletion */
	if (scroll_reverse && SP->phys_y == SP->des_top_mgn /* &&costSR<costAL */) {
		/*
		 * Use reverse scroll mode of the terminal, at
		 * the top of the window.  Reverse linefeed works
		 * too, since we only use it from top line of window.
		 */
		_setwind();
		for (i = n; i > 0; i--) {
			_pos(SP->phys_y, 0);
			tputs(scroll_reverse, 1, _outch);
			if (SP->ml_above > 0)
				SP->ml_above--;
			/*
			 * If we are at the top of the screen, and the
			 * terminal retains display above, then we
			 * should try to clear to end of line.
			 * Have to use CE since we don't remember what is
			 * actually on the line.
			 */
			if (clr_eol && memory_above)
				tputs(clr_eol, 1, _outch);
		}
	} else if (parm_insert_line && (n>1 || *insert_line==0)) {
		tputs(tparm(parm_insert_line, n, SP->phys_y), lines-SP->phys_y, _outch);
	} else if (change_scroll_region && *insert_line==0) {
		/* vt100 change scrolling region to fake AL */
		tputs(save_cursor, 1, _outch);
		tputs(	tparm(change_scroll_region,
			SP->phys_y, SP->des_bot_mgn), 1, _outch);
		/* change_scroll_region homes stupid cursor */
		tputs(restore_cursor, 1, _outch);
		for (i=n; i>0; i--)
			tputs(scroll_reverse, 1, _outch);/* should do @'s */
		/* restore scrolling region */
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);/* Once again put it back */
		SP->phys_top_mgn = SP->des_top_mgn;
		SP->phys_bot_mgn = SP->des_bot_mgn;
	} else {
		tputs(insert_line, lines - SP->phys_y, _outch);
		for (i = n - 1; i > 0; i--) {
			tputs(insert_line, lines - SP->phys_y, _outch);
		}
	}
}
