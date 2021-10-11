#ifndef lint
static	char sccsid[] = "@(#)_window.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Set the desired window to the box with the indicated boundaries.
 * All scrolling should only affect the area inside the window.
 * We currently ignore the last 2 args since we're only using this
 * for scrolling and want to use the feature on vt100's as well as
 * on concept 100's.  left and right are for future expansion someday.
 *
 * Note that we currently assume cursor addressing within the window
 * is relative to the screen, not the window.  This will have to be
 * generalized if concept windows are to be used.
 */
/* ARGSUSED */
_window(top, bottom, left, right)
int top, bottom, left, right;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_window old top=%d, bot %d; new top=%d, bot %d\n",
		SP->des_top_mgn, SP->des_bot_mgn, top, bottom);
#endif
	if (change_scroll_region || set_window) {
		SP->des_top_mgn = top;
		SP->des_bot_mgn = bottom;
	}
#ifdef DEBUG
	else
		if(outf) fprintf(outf, "window setting ignored\n");
#endif
}
