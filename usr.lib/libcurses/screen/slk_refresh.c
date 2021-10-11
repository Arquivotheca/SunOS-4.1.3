/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)slk_refresh.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

/* Update the soft-label window. */

slk_refresh()
{
    if (_slk_update())
    {
	return (wrefresh (SP->slk->_win));
    }
}

/* Update soft labels. Return TRUE if a window was updated. */

_slk_update()
{
    extern	int	_outch();
    register	WINDOW	*win;
    register	SLK_MAP	*slk;
    register	int	i;

    if ((slk = SP->slk) == NULL || (slk->_changed != TRUE))
	return (FALSE);

    win = slk->_win;
    for (i = 0; i < slk->_num; ++i)
	if (slk->_lch[i])
	{
	    if (win)	
		(void) mvwaddstr(win, 0, slk->_labx[i], slk->_ldis[i]);
	    else
		_PUTS(tparm(plab_norm, i + 1, slk->_ldis[i]), 1);

	    slk->_lch[i] = FALSE;
	}
    if (!win)
    {
	_PUTS(label_on, 1);
	/*
	 * Added an fflush because if application code calls a slk_refresh
	 * or a slk_noutrefresh and a doupdate nothing will get flushed
	 * since this information is not being kept in curscr or _virtscr.
	 */
	 (void) fflush (SP->term_file);
    }

    slk->_changed = FALSE;

    return (win ? TRUE : FALSE);
}
