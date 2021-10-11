#ifndef lint
static	char sccsid[] = "@(#)__cflush.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * This routine is one of the main things
 * in this level of curses that depends on the outside
 * environment.
 */
#include "curses.ext"

/*
 * Flush stdout.
 */
__cflush()
{
	fflush(SP->term_file);
}
