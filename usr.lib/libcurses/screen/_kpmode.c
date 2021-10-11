#ifndef lint
static	char sccsid[] = "@(#)_kpmode.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

_kpmode(m)
{
#ifdef DEBUG
	if (outf) fprintf(outf, "kpmode(%d), SP->kp_state %d\n",
	m, SP->kp_state);
#endif
	if (m == SP->kp_state)
		return;
	if (m)
		tputs(keypad_xmit, 1, _outch);
	else
		tputs(keypad_local, 1, _outch);
	SP->kp_state = m;
}
