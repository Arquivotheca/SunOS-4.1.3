/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)flush_in.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/05/30 */
#endif not lint

# include	<curses.h>

/*
 * flush_in:
 *	Flush all pending input.
 */
flush_in()
{
# ifdef TIOCFLUSH
	int input = 1;

	ioctl(fileno(stdin), TIOCFLUSH, &input);
# else TIOCFLUSH
	crmode();
# endif TIOCFLUSH
}
