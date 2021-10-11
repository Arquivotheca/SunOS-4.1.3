/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)intrflush.c 1.1 92/07/30 SMI"; /* from S5R3 1.1.1.1 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses.ext"

/*
 * TRUE => flush input when an interrupt key is pressed
 */
intrflush(win,bf)
WINDOW *win; int bf;
{
#ifdef SYSV
	if (bf)
		PROGTTY.c_lflag &= ~NOFLSH;
	else
		PROGTTY.c_lflag |= NOFLSH;
#else
# ifdef LNOFLSH
	if (bf)
		PROGLMODE &= ~LNOFLSH;
	else
		PROGLMODE |= LNOFLSH;
# else
	/* can't do this in 4.1BSD or V7 */
# endif
#endif
	reset_prog_mode();
}
