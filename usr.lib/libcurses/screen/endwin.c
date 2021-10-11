/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)endwin.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7.1.13 */
#endif

/* Clean things up before exiting. */

#include	"curses_inc.h"

#ifdef	_VR2_COMPAT_CODE
char	_endwin = 0;
#endif	/* _VR2_COMPAT_CODE */

isendwin()
{
    /*
     * The test below must stay at TRUE because the value of 2
     * has special meaning to wrefresh but does not mean that
     * endwin has been called.
     */
    if (SP && (SP->fl_endwin == TRUE))
	return (TRUE);
    else
	return (FALSE);
}

endwin()
{
    extern	int	_outch();

    /* See comment above why this test is explicitly against TRUE. */
    if (SP->fl_endwin == TRUE)
	return (ERR);

    /* Flush out any output not output due to typeahead. */
    if (_INPUTPENDING)
	(void) force_doupdate();

    /* Close things down. */
    (void) ttimeout(-1);
    if (SP->fl_meta)
	tputs(meta_off, 1, _outch);
    (void) mvcur(curscr->_cury, curscr->_curx, curscr->_maxy - 1, 0);

    if (SP->kp_state)
	tputs(keypad_local, 1, _outch);
    if (cur_term->_cursorstate != 1)
	tputs(cursor_normal, 0, _outch);
    if ((curscr->_attrs != A_NORMAL) && (magic_cookie_glitch < 0) && (!ceol_standout_glitch))
	_VIDS(A_NORMAL, curscr->_attrs);

    if (SP->phys_irm)
	_OFFINSERT();

    SP->fl_endwin = TRUE;

#ifdef	_VR2_COMPAT_CODE
    _endwin = TRUE;
#endif	/* _VR2_COMPAT_CODE */

    curscr->_clear = TRUE;
    reset_shell_mode();
    tputs(exit_ca_mode, 0, _outch);
    (void) fflush(SP->term_file);
    return (OK);
}

force_doupdate()
{
    int		ret;

    /*
     * This will cause wrefresh not to check for input waiting, so that
     * it will finish its refresh.
     */

    cur_term->_forceupdate = TRUE;
    ret = doupdate();
    cur_term->_forceupdate = FALSE;
    return (ret);
}
