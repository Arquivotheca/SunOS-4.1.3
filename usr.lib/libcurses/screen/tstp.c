/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tstp.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.9 */
#endif

#include	<signal.h>
#include	"curses_inc.h"


/* handle stop and start signals */

#ifdef	SIGTSTP
# ifdef SIGPOLL
void
# else	/* SIGPOLL */
int
# endif	/* SIGPOLL */
_tstp()
{
# ifdef SIGIO
    struct sigvec vec;
# endif

#ifdef	DEBUG
    if (outf)
	(void) fflush(outf);
#endif	/* DEBUG */
    curscr->_attrs = A_ATTRIBUTES;
    (void) endwin();
    (void) fflush(stdout);
# ifdef SIGIO		/* supports 4.2BSD signal mechanism */
    /* reset signal handler so kill below stops us */
    vec.sv_handler = SIG_DFL;
    vec.sv_mask = 0;
    vec.sv_flags = 0;
    (void) sigvec(SIGTSTP, &vec, (struct sigvec *)NULL);
# ifndef sigmask
#define	sigmask(s)	(1 << ((s)-1))
# endif
    (void) sigsetmask(sigblock(0) &~ sigmask(SIGTSTP));
# endif
    kill(0, SIGTSTP);
# ifdef SIGIO
    sigblock(sigmask(SIGTSTP));
    vec.sv_handler = _tstp;
    vec.sv_mask = 0;
    vec.sv_flags = 0;
    (void) sigvec(SIGTSTP, &vec, (struct sigvec *)NULL);
# else
    (void) signal(SIGTSTP, _tstp);
# endif
    fixterm();
    /* changed ehr3 SP->doclear = 1; */
    curscr->_clear = TRUE;
    (void) wrefresh(curscr);
}
#endif	/* SIGTSTP */

#ifdef SIGPOLL
void
#else	/* SIGPOLL */
int
#endif	/* SIGPOLL */
_ccleanup(signo)
int	signo;
{
    (void) signal(signo, SIG_IGN);

    /*
     * Fake curses into thinking that all attributes are on so that
     * endwin will turn them off since the <BREAK> key may have interrupted
     * the sequence to turn them off.
     */

    curscr->_attrs = A_ATTRIBUTES;
    (void) endwin();
#ifdef	DEBUG
    fprintf(stderr, "signal %d caught. quitting.\n", signo);
#endif	/* DEBUG */
    if (signo == SIGQUIT)
	(void) abort();
    else
	exit(1);
}
