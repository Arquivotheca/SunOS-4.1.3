#ifndef lint
static	char sccsid[] = "@(#)_forcehl.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Output the string to get us in the right highlight mode,
 * no matter what mode we are currently in.
 */
_forcehl()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_forcehl().\n");
#endif
	SP->phys_gr = -1;
	_sethl();
}
