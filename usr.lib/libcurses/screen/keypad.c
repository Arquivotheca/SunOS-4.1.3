/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)keypad.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6 */
#endif

#include	"curses_inc.h"

/* TRUE => special keys should be passed as a single value by getch. */

keypad(win, bf)
WINDOW	*win;
int	bf;
{
    extern	int	_outch();
    /*
     * Since _use_keypad is a bit and not a bool, we must check
     * bf, in case someone uses an odd number instead of 1 for TRUE
     */

    win->_use_keypad = (bf) ? TRUE : FALSE;
    if (bf && (!SP->kp_state))
    {
	tputs(keypad_xmit, 1, _outch);
	(void) fflush(SP->term_file);
	SP->kp_state = TRUE;
	return (setkeymap());
    }
    return (OK);
}
