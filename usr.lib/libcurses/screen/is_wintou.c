/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)is_wintou.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1 */
#endif

#include	"curses_inc.h"

is_wintouched(win)
WINDOW	*win;
{
    return (win->_flags & _WINCHANGED);
}
