/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_scrdown.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

#include "curses.ext"

/*
 * Scroll the screen down (e.g. in the normal direction of text) one line
 * physically, and update the internal notion of what's on the screen
 * (SP->cur_body) to know about this.  Do it in such a way that we will
 * realize this has been done later and take advantage of it.
 */
_scrdown()
{
	struct line *old_d, *old_p;
	register int i, l=lines;

#ifdef DEBUG
	if(outf) fprintf(outf, "_scrdown()\n");
#endif

	/* physically... */
	_pos(lines-1, 0);
	_scrollf(1);

	/* internally */
	old_d = SP->std_body[1];
	old_p = SP->cur_body[1];

	for (i=1; i<=l; i++) {
		SP->std_body[i] = SP->std_body[i+1];
		SP->cur_body[i] = SP->cur_body[i+1];
	}
	SP->std_body[l] = NULL;
	SP->cur_body[l] = NULL;
	_line_free(old_d);
	if (old_d != old_p)
		_line_free(old_p);
}
