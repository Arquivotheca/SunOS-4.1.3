/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)nodelay.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

/*
 * TRUE => don't wait for input, but return -1 instead.
 */

nodelay(win,bf)
WINDOW *win; int bf;
{
    win->_delay = (bf) ? 0 : -1;
    return (OK);
}
