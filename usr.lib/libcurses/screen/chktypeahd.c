#ifndef lint
static	char sccsid[] = "@(#)chktypeahd.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"
extern	int InputPending;

/*
 * If it's been long enough, check to see if we have any typeahead
 * waiting.  If so, we quit this update until next time.
 */
_chk_typeahead()
{
#ifdef FIONREAD
# ifdef DEBUG
	if(outf) fprintf(outf, "end of _id_char: --SP->check_input %d, InputPending %d, chars buffered %d: ", SP->check_input-1, InputPending, (SP->term_file->_ptr-SP->term_file->_base));
# endif
	if(--SP->check_input<0 && !InputPending &&
	    ((SP->term_file->_ptr - SP->term_file->_base) > 20)) {
		__cflush();
		if (SP->check_fd >= 0)
			ioctl(SP->check_fd, FIONREAD, &InputPending);
		else
			InputPending = 0;
		SP->check_input = SP->baud / 2400;
# ifdef DEBUG
		if(outf) fprintf(outf, "flush, ioctl returns %d, SP->check_input set to %d\n", InputPending, SP->check_input);
# endif
	}
# ifdef DEBUG
	if(outf) fprintf(outf, ".\n");
# endif
#endif
}
