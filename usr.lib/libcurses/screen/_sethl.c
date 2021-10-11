#ifndef lint
static	char sccsid[] = "@(#)_sethl.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();

_sethl ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_sethl().  SP->phys_gr=%o, SP->virt_gr %o\n", SP->phys_gr, SP->virt_gr);
#endif
#ifdef	 	VIDEO
	if (SP->phys_gr == SP->virt_gr)
		return;
	vidputs(SP->virt_gr, _outch);
	SP->phys_gr = SP->virt_gr;
	/* Account for the extra space the cookie takes up */
	if (magic_cookie_glitch >= 0)
		SP->phys_x += magic_cookie_glitch;
#endif 		VIDEO
}
