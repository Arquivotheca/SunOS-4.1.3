/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)def_prog.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1.1.5 */
#endif

#include "curses_inc.h"

def_prog_mode()
{
    /* ioctl errors are ignored so pipes work */
#ifdef SYSV
    (void) ioctl(cur_term -> Filedes, TCGETA, &(PROGTTY));
#else
    (void) ioctl(cur_term -> Filedes, TIOCGETP, &(PROGTTY));
#endif
    return (OK);
}
