#ifndef lint
static	char sccsid[] = "@(#)_setwind.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char *tparm();

extern	int	_outch();

/* Force the window to be as desired */
_setwind()
{
	if (	SP->phys_top_mgn == SP->des_top_mgn &&
		SP->phys_bot_mgn == SP->des_bot_mgn) {
#ifdef DEBUG
		if(outf) fprintf(outf, "_setwind, same values %d & %d, do nothing\n",
			SP->phys_top_mgn, SP->phys_bot_mgn);
#endif
		return;
	}
	if (set_window)
		tputs(tparm(set_window, SP->des_top_mgn,
			SP->des_bot_mgn, 0, columns-1), 1, _outch);
	else if (change_scroll_region) {
		/* Save & Restore SP->curptr since it becomes undefined */
		tputs(save_cursor, 1, _outch);
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);	/* put SP->curptr back */
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "set phys window from (%d,%d) to (%d,%d)\n",
	SP->phys_top_mgn, SP->phys_bot_mgn, SP->des_top_mgn, SP->des_bot_mgn);
#endif
	SP->phys_top_mgn = SP->des_top_mgn;
	SP->phys_bot_mgn = SP->des_bot_mgn;
}
