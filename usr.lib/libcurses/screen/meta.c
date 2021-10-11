/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)meta.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1.1.8 */
#endif

#include	"curses_inc.h"

/* TRUE => all 8 bits of input character should be passed through. */

_meta(bf)
int	bf;
{
    extern	int	_outch();
    /*
     * Do the appropriate fiddling with the tty driver to make it send
     * all 8 bits through.  On SYSV this means clearing ISTRIP, on
     * V7 you have to resort to RAW mode.
     * If the driver was in all-8-bits mode when we entered curses,
     * leave everything alone.
     */
#ifdef	SYSV
    if (SHELLTTY.c_iflag & ISTRIP) {
	if (bf)
	    PROGTTY.c_iflag &= ~ISTRIP;
	else
	    PROGTTY.c_iflag |= ISTRIP;
	reset_prog_mode();
    }
#else	/* SYSV */
    if (bf)
	raw();
    else
	noraw();
#endif	/* SYSV */

    /* Do whatever is needed to put the terminal into meta-mode. */

    if (SP->fl_meta = bf)
	tputs(meta_on, 1, _outch);
    else
	tputs(meta_off, 1, _outch);
    (void) fflush(SP->term_file);
    return (OK);
}
