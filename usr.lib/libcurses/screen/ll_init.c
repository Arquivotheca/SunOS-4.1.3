/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_init.c 1.1 92/07/30 SMI"; /* from S5R3 1.5.1.1 */
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
	_cost(ilvar) = ((long)(c_il100 - c_il0)<<5) / 100 ;
#ifdef DEBUG
	if(outf) fprintf(outf,"_init_costs, ilfixed %d, ilvar %d/32,\
 c_il0 %d, c_il100 %d\n", _cost(ilfixed), _cost(ilvar), c_il0, c_il100);
#endif

	/* This is also a botch: treated as cost to insert k characters */
	_cost(icfixed) = 0;
	if (enter_insert_mode && exit_insert_mode)
		_cost(icfixed) += _cost_fn(enter_insert_mode,0) +
			       _cost_fn(exit_insert_mode,0);
	if (parm_ich)
		_cost(icfixed) = _cost_fn(tparm(parm_ich, 10), 10);
	else if (insert_character)
		_cost(icfixed) = 0;
	else if (_cost(icfixed) == 0)
		_cost(icfixed) = INFINITY;
	_cost(icvar) = 1;	/* for the character itself */
	if (!parm_ich) {
		if (insert_character)
			_cost(icvar) += _cost_fn(insert_character,1);
		if (insert_padding)
			_cost(icvar) += _cost_fn(insert_padding,1);
	}

	if (parm_dch) {
		_cost(dcfixed) = _cost_fn(tparm(parm_dch, 10), 1);
		_cost(dcvar) = 0;
	} else if (delete_character) {
		_cost(dcfixed) = 0;
		_cost(dcvar) = _cost_fn(delete_character,0);
	} else {
		_cost(dcfixed) = INFINITY;
		_cost(dcvar) = INFINITY;
	}
	if (enter_delete_mode)
		_cost(dcfixed) += _cost_fn(enter_delete_mode,0);
	if (exit_delete_mode)
		_cost(dcfixed) += _cost_fn(exit_delete_mode,0);

#ifdef DEBUG
	if (outf) {
		fprintf(outf, "icfixed %d=%d+%d, icvar=%d\n",
			_cost(icfixed), _cost_fn(enter_insert_mode,0),
			_cost_fn(exit_insert_mode,0), _cost(icvar));
		fprintf(outf, "from ich1 %x '%s' %d\n",
			insert_character, insert_character,
			_cost_fn(insert_character,1));
		fprintf(outf, "ip %x '%s' %d\n", insert_padding,
			insert_padding, _cost_fn(insert_padding,1));
		fprintf(outf, "dcfixed %d, dcvar %d\n", _cost(dcfixed), _cost(dcvar));
	}
#endif

	_cost(Cursor_address)	= _cost_fn(tparm(cursor_address,8,10),1);
	_cost(Cursor_home)	= _cost_fn(cursor_home,1);
	_cost(Carriage_return)	= _cost_fn(carriage_return,1);
	_cost(Tab)		= _cost_fn(tab,1);
	_cost(Back_tab)		= _cost_fn(back_tab,1);
	_cost(Cursor_left)	= _cost_fn(cursor_left,1);
	_cost(Cursor_right)	= _cost_fn(cursor_right,1);
	_cost(Cursor_down)	= _cost_fn(cursor_down,1);
	_cost(Cursor_up)	= _cost_fn(cursor_up,1);
	_cost(Clr_eol)		= _cost_fn(clr_eol,1);
	_cost(Clr_bol)		= _cost_fn(clr_bol,1);
	_cost(Parm_left_cursor)	= _cost_fn(tparm(parm_left_cursor, 10),1);
	_cost(Parm_right_cursor)= _cost_fn(tparm(parm_right_cursor, 10),1);
	_cost(Parm_up_cursor)	= _cost_fn(tparm(parm_up_cursor, 10),1);
	_cost(Parm_down_cursor)	= _cost_fn(tparm(parm_down_cursor, 10),1);
	_cost(Erase_chars)	= _cost_fn(tparm(erase_chars, 10),1);
	_cost(Column_address)	= _cost_fn(tparm(column_address, 10),1);
	_cost(Row_address)	= _cost_fn(tparm(row_address, 8),1);
	_cost(Clr_eol)		= _cost_fn(tparm(clr_eol, 8),1);
}

static counter = 0;
/* ARGSUSED */
_countchar(ch)
char ch;
{
	counter++;
}

/*
 * Figure out the _cost in characters to print this string.
 * Due to padding, we can't just use strlen, so instead we
 * feed it through tputs and trap the results.
 * Even if the terminal uses xon/xoff handshaking, count the
 * pad chars here since they estimate the real time to do the
 * operation, useful in calculating costs.
 */
static
_cost_fn(str, affcnt)
char *str;
{
	int save_xflag = xon_xoff;

	if (str == NULL)
		return INFINITY;
	counter = 0;
	xon_xoff = 0;
	tputs(str, affcnt, _countchar);
	xon_xoff = save_xflag;
	return counter;
}

_init_acs()
{
	register int i;
	register chtype *nacsmap = SP->acsmap;
	register char *cp;
	register int to_get, must_output;

	/* Default acs chars for regular ASCII terminals */
	for (i=0; i<0400; i++)
		nacsmap[i] = '+';

	/* There are a few we'll hardwire defaults in. */
#define unmap(x)	(&(x) - &acs_map[0])
						    /* Dec VT100 symbols */
	nacsmap[unmap(ACS_DIAMOND)] = '*';		/* ` */
	nacsmap[unmap(ACS_CKBOARD)] = ':';		/* a */
	nacsmap[unmap(ACS_DEGREE)] = '\'';		/* f */
	nacsmap[unmap(ACS_PLMINUS)] = '#';		/* g */
	/* nacsmap[unmap(ACS_LRCORNER)] = '+'; */	/* j */
	/* nacsmap[unmap(ACS_URCORNER)] = '+'; */	/* k */
	/* nacsmap[unmap(ACS_ULCORNER)] = '+'; */	/* l */
	/* nacsmap[unmap(ACS_LLCORNER)] = '+'; */	/* m */
	/* nacsmap[unmap(ACS_PLUS)] = '+'; */	/* n */
	nacsmap[unmap(ACS_S1)] = '-';		/* o */
	nacsmap[unmap(ACS_HLINE)] = '-';		/* q */
	nacsmap[unmap(ACS_S9)] = '_';		/* s */
	/* nacsmap[unmap(ACS_LTEE)] = '+'; */	/* t */
	/* nacsmap[unmap(ACS_RTEE)] = '+'; */	/* u */
	/* nacsmap[unmap(ACS_BTEE)] = '+'; */	/* v */
	/* nacsmap[unmap(ACS_TTEE)] = '+'; */	/* w */
	nacsmap[unmap(ACS_VLINE)] = '|';		/* x */
	nacsmap[unmap(ACS_BULLET)] = 'o';		/* ~ */
	
						/* Teletype 5420 symbols */
	nacsmap[unmap(ACS_LARROW)] = '<';		/* , */
	nacsmap[unmap(ACS_RARROW)] = '>';		/* + */
	nacsmap[unmap(ACS_DARROW)] = 'v';		/* . */
	nacsmap[unmap(ACS_UARROW)] = '^';		/* - */
	nacsmap[unmap(ACS_BOARD)] = '#';		/* h */
	nacsmap[unmap(ACS_BLOCK)] = '#';		/* 0 */
	nacsmap[unmap(ACS_LANTERN)] = '#';		/* i */

	/* Now do mapping for terminals own ACS, if any */
	if (acs_chars)
		for (cp=acs_chars; *cp; ) {
			to_get = *cp++;		/* to get this ... */
			must_output = *cp++;	/* must output this ... */
#ifdef DEBUG
			if (outf) fprintf(outf, "acs %d, was %d, now %d\n",
						to_get, nacsmap[to_get], must_output);
#endif
			nacsmap[to_get] = must_output | A_ALTCHARSET;
		}
	acs_map = nacsmap;
}
