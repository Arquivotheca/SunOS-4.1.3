/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)savetty.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

savetty()
{
    SP->save_tty_buf = PROGTTY;
#ifdef DEBUG
# ifdef SYSV
    if (outf)
	fprintf(outf,"savetty(), file %x, SP %x, flags %x,%x,%x,%x\n",
	    cur_term->Filedes, SP, PROGTTY.c_iflag, PROGTTY.c_oflag,
	    PROGTTY.c_cflag, PROGTTY.c_lflag);
# else
    if (outf)
	fprintf(outf, "savetty(), file %x, SP %x, flags %x\n",
	    cur_term->Filedes, SP, PROGTTY.sg_flags);
# endif
#endif
    return (OK);
}
