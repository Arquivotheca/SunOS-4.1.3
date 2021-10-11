/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)slk_noutref.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.2 */
#endif

#include	"curses_inc.h"

/* Wnoutrefresh for the softkey window. */

slk_noutrefresh()
{
    if (SP->slk == NULL)
	return (ERR);

    if (SP->slk->_win && _slk_update())
	(void) wnoutrefresh(SP->slk->_win);

    return (OK);
}
