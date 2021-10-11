/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)halfdelay.c 1.1 92/07/30 SMI"; /* from S5R3 1.2.1.1 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses.ext"

halfdelay(tenths)
int tenths;
{
	if (tenths > 255)
		tenths = 255;
	else if (tenths <= 0)
		tenths = 1;
	_noraw();
#ifdef SYSV
	PROGTTY.c_iflag &= ~ICRNL;
	PROGTTY.c_lflag &= ~ICANON;
	PROGTTY.c_cc[VMIN] = 0;
	PROGTTY.c_cc[VTIME] = tenths;
# ifdef DEBUG
	if(outf) fprintf(outf, "halfdelay(%d), file %x, SP %x, flags %x\n", tenths, SP->term_file, SP, PROGTTY.c_lflag);
# endif
#else
# ifdef FIONREAD
	cur_term->timeout = tenths;
# else
	/* not possible to get right. use cbreak() until we can. */
	_cbreak();
# endif /* !FIONREAD */
# ifdef DEBUG
	if(outf) fprintf(outf, "halfdelay(%d)\n", tenths);
# endif
#endif
	SP->fl_rawmode = TRUE;
	reset_prog_mode();
}
