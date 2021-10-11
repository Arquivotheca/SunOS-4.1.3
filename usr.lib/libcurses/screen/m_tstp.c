#ifndef lint
static	char sccsid[] = "@(#)m_tstp.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

# include	"curses.ext"
# include	<signal.h>

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 * 3/5/81 (Berkeley) @(#)addch.c	1.3
 */

# ifdef SIGTSTP

/*
 * handle stop and start signals
 *
 * 3/5/81 (Berkeley) @(#)_tstp.c	1.1
 */
m_tstp() {

# ifdef DEBUG
	if (outf) fflush(outf);
# endif
	_ll_move(lines-1, 0);
	endwin();
	fflush(stdout);
# ifdef SIGIO		/* supports 4.2BSD signal mechanism */
	/* reset signal handler so kill below stops us */
	signal(SIGTSTP, SIG_DFL);
# ifndef sigmask
#define	sigmask(s)	(1 << ((s)-1))
# endif
	(void) sigsetmask(sigblock(0) &~ sigmask(SIGTSTP));
# endif
	kill(0, SIGTSTP);
# ifdef SIGIO
	sigblock(sigmask(SIGTSTP));
# endif
	signal(SIGTSTP, m_tstp);
	reset_prog_mode();
	SP->doclear = 1;
	_ll_refresh(0);
}
# endif
