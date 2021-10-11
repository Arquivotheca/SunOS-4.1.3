/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)slk_clear.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.8 */
#endif

#include	"curses_inc.h"

/* Clear the soft labels. */

slk_clear()
{
    extern	int	_outch();
    register	SLK_MAP	*slk;
    register	int	i;

    if ((slk = SP->slk) == NULL)
	return (ERR);

    slk->_changed = 2;	/* This means no more soft labels. */
    if (slk->_win)
    {
	(void) werase(slk->_win);
	(void) wrefresh(slk->_win);
    }
    else
    {
	/* send hardware clear sequences */
	for (i = 0; i < slk->_num; i++)
	    _PUTS(tparm(plab_norm, i + 1, "        "), 1);
	_PUTS(label_off, 1);
	(void) fflush(SP->term_file);
    }

    for (i = 0; i < slk->_num; ++i)
	slk->_lch[i] = FALSE;

    return (OK);
}
