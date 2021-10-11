#ifndef lint
static	char sccsid[] = "@(#)_inschars.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char *tparm();

extern	int	_outch();

/*
 * Insert n blank characters.
 */
_inschars(n)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_inschars(%d).\n", n);
#endif
	if (enter_insert_mode && SP->phys_irm == 0) {
		tputs(enter_insert_mode, 1, _outch);
		SP->phys_irm = 1;
	}
	if (parm_ich && n > 1)
		tputs(tparm(parm_ich, n), n, _outch);
	else
		while (--n >= 0)
			tputs(insert_character, 1, _outch);
}
