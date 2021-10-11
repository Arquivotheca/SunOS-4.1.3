#ifndef lint
static	char sccsid[] = "@(#)_init_cost.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Figure out (roughly) how much each of these capabilities costs.
 * In the parameterized cases, we just take a typical case and
 * use that value.  This is done only once at startup, since it
 * would be too expensive for intensive use.
 */
_init_costs()
{
	int c_il0, c_il100;
	char *tparm();

	/*
	 * Insert line costs.  These are a mess, they do not take into
	 * account parameterized insert line, but rather assume 1 at a time.
	 * Cost is # chars to insert one line k lines from bottom of screen.
	 */
	if (insert_line) {
#ifdef DEBUG
		if(outf) fprintf(outf, "real insert line\n");
#endif
		c_il0 = _cost_fn(insert_line,0);
		c_il100 = _cost_fn(insert_line,100);
	} else if (change_scroll_region && save_cursor && restore_cursor) {
#ifdef DEBUG
		if(outf) fprintf(outf, "use scrolling region\n");
#endif
		c_il0 = 2*_cost_fn(change_scroll_region,lines-1) +
			2*_cost_fn(restore_cursor,0) +
			_cost_fn(save_cursor,0) + _cost_fn(scroll_reverse,0);
		c_il100 = c_il0;
	} else {
#ifdef DEBUG
		if(outf) fprintf(outf, "no insert line\n");
#endif
		c_il0 = c_il100 = INFINITY;
	}
	_cost(ilfixed) = c_il0;
	_cost(ilvar) = ((long)(c_il100 - c_il0)<<32) / 100 ;
#ifdef DEBUG
	if(outf) fprintf(outf,"_init_costs, ilfixed %d, ilvar %d/32,\
 c_il0 %d, c_il100 %d\n", _cost(ilfixed), _cost(ilvar), c_il0, c_il100);
#endif

	/* This is also a botch: treated as _cost to insert k characters */
	_cost(icvar) = _cost(icfixed) = 0;
	if (enter_insert_mode && exit_insert_mode)
		_cost(icfixed) += _cost_fn(enter_insert_mode,0) +
			       _cost_fn(exit_insert_mode,0);
	if (parm_ich)
		_cost(icfixed) = _cost_fn(tparm(parm_ich, 10), 10);
	else if (insert_character)
		_cost(icfixed) = 0;
	else if (_cost(icfixed) == 0)
		_cost(icfixed) = INFINITY;
	_cost(icvar) = 1<<5;	/* for the character itself */
	if (!parm_ich) {
		if (insert_character)
			_cost(icvar) += _cost_fn(insert_character,1)<<5;
		if (insert_padding)
			_cost(icvar) += _cost_fn(insert_padding,1)<<5;
	}
#ifdef DEBUG
	if (outf) fprintf(outf, "icfixed %d=%d+%d, icvar=%d/32\n",
		_cost(icfixed), _cost_fn(enter_insert_mode,0),
		_cost_fn(exit_insert_mode,0), _cost(icvar));
	if (outf) fprintf(outf, "from ich1 %x '%s' %d\n",
		insert_character, insert_character,
		_cost_fn(insert_character,1));
	if (outf) fprintf(outf, "ip %x '%s' %d\n", insert_padding,
		insert_padding, _cost_fn(insert_padding));
#endif

	_cost(Cursor_address)	= _cost_fn(tparm(cursor_address,8,10),1);
	_cost(Cursor_home)	= _cost_fn(cursor_home,1);
	_cost(Carriage_return)	= _cost_fn(carriage_return,1);
	_cost(Tab)		= _cost_fn(tab,1);
	_cost(Back_tab)		= _cost_fn(back_tab,1);
	_cost(Cursor_left)	= _cost_fn(cursor_left,1);
	_cost(Cursor_right)	= _cost_fn(cursor_right,1);
	_cost(Right_base)	= _cost(Cursor_right);
	_cost(Cursor_down)	= _cost_fn(cursor_down,1);
	_cost(Cursor_up)	= _cost_fn(cursor_up,1);
	_cost(Parm_left_cursor)	= _cost_fn(tparm(parm_left_cursor, 10),1);
	_cost(Parm_right_cursor)= _cost_fn(tparm(parm_right_cursor, 10),1);
	_cost(Parm_up_cursor)	= _cost_fn(tparm(parm_up_cursor, 10),1);
	_cost(Parm_down_cursor)	= _cost_fn(tparm(parm_down_cursor, 10),1);
	_cost(Column_address)	= _cost_fn(tparm(column_address, 10),1);
	_cost(Row_address)	= _cost_fn(tparm(row_address, 8),1);
}
