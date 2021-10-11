#ifndef lint
static	char	sccsid[] = "@(#)end_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		end_menu.c
 *
 *	Description:	Provide a clean ending for the menu system.
 *
 *	Call syntax:	end_menu();
 */

#include <curses.h>
#ifdef CATCH_SIGNALS
#	include <signal.h>
#endif
#include "menu.h"


void
end_menu()
{
#ifdef CATCH_SIGNALS
	int		i;			/* signal counter */
#endif


	if (_menu_init_done) {
		/*
		 *	Do a standend() to make sure that standout mode is
		 *	off.  It is followed by a refresh() since it does
		 *	not seem to work if followed by a clear().
		 */
		standend();
		refresh();

		clear();
		refresh();

		endwin();
		resetty();

#ifdef CATCH_SIGNALS
		for (i = 1; i < NSIG; i++)
			(void) signal(i, _menu_signals[i]);
#endif

		_menu_init_done = 0;
	}
} /* end end_menu() */
