/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_trm.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

#include "curses.ext"

extern _outch();
char *tparm();

_syncmodes()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_syncmodes().\n");
#endif
	_sethl();
	_setmode();
	_setwind();
}

_hlmode (on)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_hlmode(%o).\n", on);
#endif
	SP->virt_gr = on;
}

_kpmode(m)
{
#ifdef DEBUG
	if (outf) fprintf(outf, "kpmode(%d), SP->kp_state %d\n",
	m, SP->kp_state);
#endif
	if (m == SP->kp_state)
		return;
	if (m)
		tputs(keypad_xmit, 1, _outch);
	else
		tputs(keypad_local, 1, _outch);
	SP->kp_state = m;
}

_sethl ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_sethl().  SP->phys_gr=%o, SP->virt_gr %o\n", SP->phys_gr, SP->virt_gr);
#endif
	if (SP->phys_gr == SP->virt_gr)
		return;
	vidputs((int)SP->virt_gr, _outch);
	SP->phys_gr = SP->virt_gr;
	/* Account for the extra space the cookie takes up */
	if (magic_cookie_glitch > 0)
		SP->phys_x += magic_cookie_glitch;
}

_setmode ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_setmode().\n");
#endif
	if (SP->virt_irm == SP->phys_irm)
		return;
	tputs(SP->virt_irm==1 ? enter_insert_mode : exit_insert_mode, 0, _outch);
	SP->phys_irm = SP->virt_irm;
}

/* Force the window to be as desired */
_setwind()
{
	if (	SP->phys_top_mgn == SP->des_top_mgn &&
		SP->phys_bot_mgn == SP->des_bot_mgn) {
#ifdef DEBUG
		if(outf) fprintf(outf, "_setwind, same values %d & %d, do nothing\n",
			SP->phys_top_mgn, SP->phys_bot_mgn);
#endif
		return;
	}
	if (set_window)
		tputs(tparm(set_window, SP->des_top_mgn,
			SP->des_bot_mgn, 0, columns-1), 1, _outch);
	else if (change_scroll_region && save_cursor && restore_cursor) {
		/* Save & Restore SP->curptr since it becomes undefined */
		tputs(save_cursor, 1, _outch);
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);	/* put SP->curptr back */
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "set phys window from (%d,%d) to (%d,%d)\n",
	SP->phys_top_mgn, SP->phys_bot_mgn, SP->des_top_mgn, SP->des_bot_mgn);
#endif
	SP->phys_top_mgn = SP->des_top_mgn;
	SP->phys_bot_mgn = SP->des_bot_mgn;
}

/*
 * Set the desired window to the box with the indicated boundaries.
 * All scrolling should only affect the area inside the window.
 * We currently ignore the last 2 args since we're only using this
 * for scrolling and want to use the feature on vt100's as well as
 * on concept 100's.  left and right are for future expansion someday.
 *
 * Note that we currently assume cursor addressing within the window
 * is relative to the screen, not the window.  This will have to be
 * generalized if concept windows are to be used.
 */
/* ARGSUSED */
_window(top, bottom, left, right)
int top, bottom, left, right;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_window old top=%d, bot %d; new top=%d, bot %d\n",
		SP->des_top_mgn, SP->des_bot_mgn, top, bottom);
#endif
	if (change_scroll_region || set_window) {
		SP->des_top_mgn = top;
		SP->des_bot_mgn = bottom;
	}
#ifdef DEBUG
	else
		if(outf) fprintf(outf, "window setting ignored\n");
#endif
}

void
_absmovehome()
{
    if (cursor_home)
	tputs(cursor_home, 1, _outch);
    else if (cursor_address)
	tputs (tparm(cursor_address, 0, 0), 1, _outch);
    SP->phys_x = 0;
    SP->phys_y = 0;
}

_reset ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_reset().\n");
#endif
	tputs(enter_ca_mode, 0, _outch);
	if (hard_cursor) {
		tputs(cursor_visible, 0, _outch);
		SP->cursorstate = 2;
	}
	tputs(exit_attribute_mode, 0, _outch);
	if (clear_screen) {
		SP->phys_x = 0;
		SP->phys_y = 0;
		tputs(clear_screen, 0, _outch);
	} else {
		register int l;

		_absmovehome();
		if (clr_eos)
			tputs(clr_eos, lines, _outch);
		/* clear from beginning of each line */
		else if (clr_eol)
			for (l = 0; l < lines; l++) {
				_pos (l, 0);
				tputs (clr_eol, columns, _outch);
			}
		/* delete all lines from screen */
		else if (delete_line && insert_line) {
			_pos (0, 0);
			_dellines (lines);
			if (memory_below)  /* if necessary, put them back */
				_inslines (lines);
		}
		_pos (0, 0);
	}
	SP->phys_irm = 1;
	SP->virt_irm = 0;
	SP->phys_top_mgn = 4;
	SP->phys_bot_mgn = 4;
	SP->des_top_mgn = 0;
	SP->des_bot_mgn = lines-1;
	SP->ml_above = 0;
	_setwind();
}
