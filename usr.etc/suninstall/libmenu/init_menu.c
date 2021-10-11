#ifndef lint
static	char	sccsid[] = "@(#)init_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		init_menu.c
 *
 *	Description:	Initialize the menu system.
 *
 *	Call syntax:	init_menu();
 */

#include <curses.h>
#include <signal.h>
#include "menu.h"


void
init_menu()
{
#ifdef CATCH_SIGNALS
	int		i;			/* signal counter */


	/*
	 *	Set up software signals
	 */
	for (i = 1; i < NSIG; i++)
		_menu_signals[i] = signal(i, menu_sig_trap);

	/*
	 *	The following signals should be ignored
	 */
	(void) signal(SIGQUIT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
        (void) signal(SIGTSTP, SIG_IGN);

	/*
	 *	The following signals should do default action
	 */
	(void) signal(SIGURG, SIG_DFL);
	(void) signal(SIGCONT, SIG_DFL);
	(void) signal(SIGCLD, SIG_DFL);
	(void) signal(SIGIO, SIG_DFL);
	(void) signal(SIGWINCH, SIG_DFL);
#else
	(void) signal(SIGINT, menu_abort);
	(void) signal(SIGQUIT, menu_abort);
#endif
	
	savetty();
	(void) initscr();
	clear();
	refresh();

	/*
	 *	If CR->NL mapping is turned on, then reentrant menus and forms
	 *	do not work.
	 */
	noecho();				/* turn off echo */
	nonl();					/* turn off CR->LF mapping */
	noraw();				/* turn off raw mode */

	crmode();				/* turn on c-break */

	leaveok(stdscr, FALSE);
	scrollok(stdscr, FALSE);

	_menu_init_done = 1;
} /* end init_menu() */
