/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)chkinput.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6.1.7 */
#endif

/*
 * chkinput()
 *
 * Routine to check to see if any input is waiting.
 * It returns a 1 if there is input waiting, and a zero if
 * no input is waiting.
 *
 * This function replaces system calls to ioctl() with the FIONREAD
 * parameter. It enables curses to stop a screen refresh whenever
 * a character is input.
 * Standard BTL UNIX 4.0 or 5.0 does not handle FIONREAD.
 * Changes have been made to 'wrefresh.c' to
 * call this routine as "_inputpending = chkinput()".
 * (delay.c and getch.c also use FIONREAD for nodelay, select and fast peek,
 * but these routines have not been changed).
 *
 * Philip N. Crusius - July 20, 1983
 * Modified to handle various systems March 9, 1984 by Mark Horton.
 */
#include	"curses_inc.h"

_chkinput()
{
    /* If input is waiting in curses queue, return (TRUE). */
#ifdef DEBUG
	fprintf(outf, "Entering _chkinput()\n");
#endif /* DEBUG */

    if ((int) cur_term->_chars_on_queue > 0)
    {
#ifdef	DEBUG
	if (outf)
	{
	    (void) fprintf(outf, "Found a character on the input queue\n");
	    _print_queue();
	}
#endif	/* DEBUG */
	return (TRUE);
    }

#ifdef	FIONREAD
    {
    int	i;
    if (!cur_term->fl_typeahdok || (cur_term->_chars_on_queue == INP_QSIZE) ||
        (cur_term->_check_fd < 0)) {
#ifdef DEBUG
	    fprintf(outf,"no peek at input - curses not reading\n");
#endif /* DEBUG */
            return FALSE;
        }
    ioctl(cur_term->_check_fd, FIONREAD, &i);
    return (i > 0);
    }
#else
# ifdef	SYSV
    {
    unsigned	char	c;	/* character input */

    /*
     * Only check typeahead if the user is using our input
     * routines. This is because the read below will put
     * stuff into the inputQ that will never be read and the
     * screen will never get updated from now on.
     * This code should GO AWAY when a poll() or FIONREAD can
     * be done on the file descriptor as then the check
     * will be non-destructive.
     */

    if (!cur_term->fl_typeahdok || (cur_term->_chars_on_queue == INP_QSIZE) ||
	(cur_term->_check_fd < 0))
    {
	goto bad;
    }

    if (read(cur_term->_check_fd, (char *) &c, 1) > 0)
    {
#ifdef	DEBUG
	if (outf)
	{
	    (void) fprintf(outf, "Reading ahead\n");
	}
#endif	/* DEBUG */
	/*
	 * A character was waiting.  Put it at the end
	 * of the curses queue and return 1 to show that
	 * input is waiting.
	 */
#ifdef	DEBUG
	if (outf)
	    _print_queue();
#endif	/* DEBUG */
	cur_term->_input_queue[cur_term->_chars_on_queue++] = c;
good:
	return (TRUE);
    }
    else	/* No input was waiting so return 0. */
    {
#ifdef	DEBUG
	if (outf)
	    (void) fprintf(outf, "No input waiting\n");
#endif	/* DEBUG */
bad:
	return (FALSE);
    }
# else	/* SYSV */
    return (FALSE);	/* can't do the check */
# endif	/* SYSV */
#endif	/* FIONREAD */
}

#ifdef	DEBUG
_print_queue()	/* FOR DEBUG ONLY */
{
    int		i, j = cur_term->_chars_on_queue;
    short	*inputQ = cur_term->_input_queue;

    if (outf)
	for (i = 0; i < j; i++)
	    (void) fprintf (outf, "inputQ[%d] = %c\n", i, inputQ[i]);
}
#endif	/* DEBUG */
