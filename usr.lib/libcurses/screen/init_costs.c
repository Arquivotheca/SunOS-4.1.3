/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)init_costs.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.10 */
#endif

#include	"curses_inc.h"

/*
 * Figure out (roughly) how much each of these capabilities costs.
 * In the parameterized cases, we just take a typical case and
 * use that value.  This is done only once at startup, since it
 * would be too expensive for intensive use.
 */

static	short	offsets[] =
		{
		    52,		/* insert_character, */
		    21,		/* delete_character, */
		    12,		/* cursor_home, */
		    18,		/* cursor_to_ll, */
		    14,		/* cursor_left, */
		    17,		/* cursor_right, */
		    11,		/* cursor_down, */
		    19,		/* cursor_up, */
		    2,		/* carriage_return, */
		    134,	/* tab, */
		    0,		/* back_tab, */
		    6,		/* clr_eol, */
		    269,	/* clr_bol, */
#define	FIRST_LOOP	13
		    108,	/* parm_ich, */
		    105,	/* parm_dch, */
		    111,	/* parm_left_cursor, */
		    114,	/* parm_up_cursor, */
		    107,	/* parm_down_cursor, */
		    112,	/* parm_right_cursor, */
#define	SECOND_LOOP	19
		};

void
_init_costs()
{
    register	short	*costptr = &(SP->term_costs.icfixed);
    register	char	**str_array = (char **) cur_strs;
    register	int	i = 0;
    int		save_xflag = xon_xoff;

    xon_xoff = 0;
/*
 * This next block of code is actually correct in that it takes into
 * account many things that wrefresh has to keep figuring in the function
 * _useidch.  Wrefresh MUST be changed (in the words of Tony Hansen) !!!
 *
 * Wrefresh has been changed (in my words -Phong Vo) !!!!
 */
    *costptr++ = ((enter_insert_mode) && (exit_insert_mode)) ?
	_cost_fn(enter_insert_mode, 0) + _cost_fn(exit_insert_mode, 0) : 0;

    *costptr++ = ((enter_delete_mode) && (exit_delete_mode)) ?
	_cost_fn(enter_delete_mode, 0) + _cost_fn(exit_delete_mode, 0) : 0;

    while (i < FIRST_LOOP)
	*costptr++ = _cost_fn(str_array[offsets[i++]], 1);

    while (i < SECOND_LOOP)
	*costptr++ = _cost_fn(tparm(str_array[offsets[i++]], 10), 1);

    *costptr++ = _cost_fn(tparm(cursor_address, 8, 10), 1);
    *costptr++ = _cost_fn(tparm(row_address, 8), 1);

    xon_xoff = save_xflag;
#ifdef	DEBUG
    if (outf)
    {
	fprintf(outf, "icfixed %d=%d+%d, icvar=%d\n", _COST(icfixed),
	    _cost_fn(enter_insert_mode, 0), _cost_fn(exit_insert_mode, 0),
	    _COST(Insert_character));
	fprintf(outf, "from ich1 %x '%s' %d\n", insert_character,
	    insert_character, _cost_fn(insert_character, 1));
	fprintf(outf, "ip %x '%s' %d\n", insert_padding, insert_padding,
	    _cost_fn(insert_padding, 1));
	fprintf(outf, "dcfixed %d, dcvar %d\n", _COST(dcfixed),
	    _COST(Delete_character));
    }
#endif	/* DEBUG */

}

static	counter = 0;
/* ARGSUSED */
_countchar()
{
    counter++;
}

/*
 * Figure out the _COST in characters to print this string.
 * Due to padding, we can't just use strlen, so instead we
 * feed it through tputs and trap the results.
 * Even if the terminal uses xon/xoff handshaking, count the
 * pad chars here since they estimate the real time to do the
 * operation, useful in calculating costs.
 */

static
_cost_fn(str, affcnt)
char	*str;
int	affcnt;
{
    extern	int	_countchar();

    if (str == NULL)
	return (LARGECOST);
    counter = 0;
    tputs(str, affcnt, _countchar);
    return (counter);
}
