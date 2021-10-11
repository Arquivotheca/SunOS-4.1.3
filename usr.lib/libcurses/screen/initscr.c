/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)initscr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3.1.7 */
#endif

#include	"curses_inc.h"

/*
 * This routine initializes the current and standard screen,
 * and sets up the terminal.  In case of error, initscr aborts.
 * If you want an error status returned, call
 *	scp = newscreen(getenv("TERM"), 0, 0, 0, stdout, stdin);
 */

WINDOW	*
initscr()
{
    static	char	i_called_before = FALSE;

/* Free structures we are about to throw away so we can reuse the memory. */

    if (i_called_before && SP)
    {
	delscreen(SP);
	SP = NULL;
    }
    if (newscreen((char *)NULL, 0, 0, 0, stdout, stdin) == NULL)
    {
	reset_shell_mode();
	if (term_errno != -1)
	    termerr();
	else
	    curserr();
	exit(1);
    }

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "initscr: term = %s\n", SP->tcap->_termname);
#endif	/* DEBUG */
    i_called_before = TRUE;

    return (stdscr);
}
