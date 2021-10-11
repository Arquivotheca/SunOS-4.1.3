#ifndef lint
static	char sccsid[] = "@(#)vidattr.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * Handy function to put out a string with padding.
 * It makes two assumptions:
 *	(1) Output is via stdio to stdout through putchar.
 *	(2) There is no count of affected lines.  Thus, this
 *	    routine is only valid for certain capabilities,
 *	    i.e. those that don't have *'s in the documentation.
 */

#include <stdio.h>

extern	int	_outchar();
/*
 * Handy way to output video attributes.
 */
vidattr(newmode)
int newmode;
{
	vidputs(newmode, _outchar);
}
