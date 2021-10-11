/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)nonl.c 1.1 92/07/30 SMI"; /* from S5R3 1.3.1.2 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses.ext"

nonl()
{
#ifdef SYSV
/*	PROGTTY.c_iflag &= ~ICRNL;	this messes up input virt. */
	PROGTTY.c_oflag &= ~ONLCR;
	if (PROGTTY.c_oflag == OPOST)
		PROGTTY.c_oflag = 0;
# ifdef DEBUG
	if(outf) fprintf(outf, "nonl(), file %x, SP %x, flags %x,%x\n", SP->term_file, SP, PROGTTY.c_iflag, PROGTTY.c_oflag);
# endif
#else
	/* this messes up input virt. as well, but can not be helped. */
	PROGTTY.sg_flags &= ~CRMOD;
# ifdef DEBUG
	if(outf) fprintf(outf, "nonl(), file %x, SP %x, flags %x\n", SP->term_file, SP, PROGTTY.sg_flags);
# endif
#endif
	reset_prog_mode();
}
