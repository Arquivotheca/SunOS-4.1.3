#ifndef lint
static	char sccsid[] = "@(#)_reset.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

_reset ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_reset().\n");
#endif
	tputs(enter_ca_mode, 0, _outch);
	tputs(cursor_visible, 0, _outch);
	tputs(exit_attribute_mode, 0, _outch);
	tputs(clear_screen, 0, _outch);
	SP->phys_x = 0;
	SP->phys_y = 0;
	SP->phys_irm = 1;
	SP->virt_irm = 0;
	SP->phys_top_mgn = 4;
	SP->phys_bot_mgn = 4;
	SP->des_top_mgn = 0;
	SP->des_bot_mgn = lines-1;
	SP->ml_above = 0;
	_setwind();
}
