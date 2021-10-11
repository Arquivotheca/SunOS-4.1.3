/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)fixdelay.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses.ext"

/*
 * The user has just changed his notion of whether we want nodelay mode.
 * Do any system dependent processing.
 */
/* ARGSUSED */
_fixdelay(old, new)
bool old, new;
{
#ifdef SYSV
# include <fcntl.h>
	register int fl, fd;
	extern int errno;

	if (old != new) {
		fd = fileno(SP->term_file);
		fl = fcntl(fd, F_GETFL, 0);
		if (fl == -1)
		    (void) fprintf(stderr, "fcntl(F_GETFL) failed in _fixdelay\n");
		if (new)
			fl |= O_NDELAY;
		else
			fl &= ~O_NDELAY;
		(void) fcntl(fd, F_SETFL, fl);
	}
#else
	/* No system dependent processing on the V7 or Berkeley systems. */
#endif
	SP->fl_nodelay = new;
}
