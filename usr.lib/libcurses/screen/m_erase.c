/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_erase.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

# include	"curses.ext"

/*
 * Erase the contents of the desired screen - set to all _blanks.
 */
m_erase()
{
	register int n;

#ifdef DEBUG
	if (outf) fprintf(outf, "M_ERASE, erasing %d lines\n", lines);
#endif
	for (n = 0; n < lines; n++) {
		_clearline(n);
	}
}

/*
 * '_clearline' positions the cursor at the beginning of the
 * indicated line and clears the line (in the image)
 */
static _clearline (row)
{
	_ll_move (row, 0);
	SP->std_body[row+1] -> length = 0;
}
