#ifndef lint
static	char sccsid[] = "@(#)_c_clean.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

_c_clean ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_c_clean().\n");
#endif
	_hlmode (0);
	_kpmode(0);
	SP->virt_irm = 0;
	_window(0, lines-1, 0, columns-1);
	_syncmodes();
	tputs(exit_ca_mode, 0, _outch);
	tputs(cursor_normal, 0, _outch);
}
