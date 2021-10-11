/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_echo.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

/*
 *  Here are lower level functions for wechochar() and pechochar() to
 *  use to output a single character. The latter is similar to trm.c
 *  (writechars()) in purpose.
 */


#include "curses.ext"
extern int outchcount;

int
_ll_echochar(SPy, SPx, c, attrs, rawc)
register int SPy, SPx;
register chtype c, attrs, rawc;
{
	register chtype *curbody = SP->cur_body[SPy+1]->body;
	outchcount = 0;

	/* If the cursor is not in the right place, put it there! */
	if ((SPx != SP->phys_x) || (SPy != SP->phys_y))
		_pos (SPy, SPx);

	if (ceol_standout_glitch) {
		/* if ((lefthl != thishl) || (oldhl != thishl)) */
		if (((SPx > 0) && ((curbody[SPx-1]&A_ATTRIBUTES) != attrs)) ||
		    ((curbody[SPx]&A_ATTRIBUTES) != attrs) )
			_forcehl();
	}

	if (SPx >= SP->cur_body [SPy + 1]->length)
		SP->cur_body [SPy + 1]->length = SPx + 1;
	curbody [SPx] = c;

	SP->phys_x++;
	SP->curptr++;
	SP->leave_x = SP->virt_x = SP->phys_x;
	SP->leave_y = SP->virt_y = SP->phys_y;

	/* If in insert mode, take us out. */
	if (SP->phys_irm == 1) {
		_insmode (0);
		_setmode ();
	}

	/* If not with the proper highlighting mode, put us in. */
	if ((SP->virt_gr != attrs) || (SP->virt_gr != SP->phys_gr)) {
		_hlmode ((int) attrs);
		_sethl ();
	}

	/* Handle strange underlining terminals. We don't */
	/* need to worry about the right hand side here */
	/* because that's handled before _ll_echo() is called. */
	if (transparent_underline && erase_overstrike && (rawc == '_')) {
		_outch (' ');
		tputs (cursor_left, 1, _outch);
	}

	/* Write it out! */
	_outch ((tilde_glitch && (rawc == '~')) ? (chtype) '`' : (chtype) rawc);
	__cflush ();

	return outchcount;
}
