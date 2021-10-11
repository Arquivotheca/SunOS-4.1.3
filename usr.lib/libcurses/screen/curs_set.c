/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)curs_set.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7 */
#endif

#include	"curses_inc.h"

/* Change the style of cursor in use. */

curs_set(visibility)
register	int	visibility;
{
    extern	int	_outch();
    int		ret = cur_term->_cursorstate;
    char	**cursor_seq = cur_term->cursor_seq;

    if ((visibility < 0) || (visibility > 2) || (!cursor_seq[visibility]))
	ret = ERR;
    else
	if (visibility != ret)
	    tputs(cursor_seq[cur_term->_cursorstate = visibility], 0, _outch);
    (void) fflush(SP->term_file);
    return (ret);
}
