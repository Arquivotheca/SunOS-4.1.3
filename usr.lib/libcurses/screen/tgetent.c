/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"

/*	@(#)tgetent.c 1.1 92/07/30 SMI	(1.8	3/6/83)	*/

int
tgetent(bp, name)
char *bp, *name;
{
	int rv;

	if (setupterm(name, 1, &rv) >= 0)
		/* Leave things as they were (for compatibility) */
		reset_shell_mode();
	return rv;
}
