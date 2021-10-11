#ifndef lint
static	char sccsid[] = "@(#)_clearline.c 1.1 92/07/30 SMI"; /* from S5R2 1.9 */
#endif

#include "curses.ext"

/*
 * '_clearline' positions the cursor at the beginning of the
 * indicated line and clears the line (in the image)
 */
_clearline (row)
{
	_ll_move (row, 0);
	SP->std_body[row+1] -> length = 0;
}
