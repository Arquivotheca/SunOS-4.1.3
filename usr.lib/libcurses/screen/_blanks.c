#ifndef lint
static	char sccsid[] = "@(#)_blanks.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

extern	int	_outch();
extern	int	_sethl();
extern	int	_setmode();
extern	char	*tparm();
extern	int	tputs();

/*
 * Output n blanks, or the equivalent.  This is done to erase text, and
 * also to insert blanks.  The semantics of this call do not define where
 * it leaves the cursor - it might be where it was before, or it might
 * be at the end of the blanks.  We will, of course, leave SP->phys_x
 * properly updated.
 */
_blanks (n)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_blanks(%d).\n", n);
#endif
	if (n == 0)
		return;
	_setmode ();
	_sethl ();
	if (SP->virt_irm==1 && parm_ich) {
		if (n == 1)
			tputs(insert_character, 1, _outch);
		else
			tputs(tparm(parm_ich, n), n, _outch);
		return;
	}
	if (erase_chars && SP->phys_irm != 1 && n > 5) {
		tputs(tparm(erase_chars, n), n, _outch);
		return;
	}
	if (repeat_char && SP->phys_irm != 1 && n > 5) {
		tputs(tparm(repeat_char, ' ', n), n, _outch);
		SP->phys_x += n;
		return;
	}
	while (--n >= 0) {
		if (SP->phys_irm == 1 && insert_character)
			tputs (insert_character, columns - SP->phys_x, _outch);
		if (++SP->phys_x >= columns && auto_right_margin) {
			if (SP->phys_y >= lines-1) {
				/*
				 * We attempted to put something in the last
				 * position of the last line.  Since this will
				 * cause a scroll (we only get here if the
				 * terminal has auto_right_margin) we refuse
				 * to put it out.
				 */
				SP->phys_x--;
				return;
			}
			SP->phys_x = 0;
			SP->phys_y++;
		}
		_outch (' ');
		if (SP->phys_irm == 1 && insert_padding)
			tputs (insert_padding, 1, _outch);
	}
}
