/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)_mvaddchstr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1 */
#endif

#define	NOMACROS

#include	"curses_inc.h"

mvaddchstr(y, x, s)
int	y, x;
chtype	*s;
{
    return (wmove(stdscr, y, x)==ERR?ERR:waddchstr(stdscr, s));
}
