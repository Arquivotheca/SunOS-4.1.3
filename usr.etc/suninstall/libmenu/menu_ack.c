#ifndef lint
static	char	sccsid[] = "@(#)menu_ack.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_ack.c
 *
 *	Description:	Print MESG on the last line of the screen
 *		and wait for the user to type a return.
 *
 *	Call syntax:	menu_ack();
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


static	char		MESG[] = "Press <return> to continue ... ";


void
menu_ack()
{
	char		ch;			/* scratch character */
	int		x_coord, y_coord;	/* saved coordinates */


	if (!_menu_init_done) {
		(void) fprintf(stderr, MESG);
		do
			(void) read(0, &ch, sizeof(ch));
		while (ch != '\n' && ch != '\r');

		return;
	}

	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	move(LINES - 1, 0);
	clrtoeol();
	addstr(MESG);
	refresh();

	help_screen_off();

	do
		ch = read_menu_char();
	while (ch != '\n' && ch != '\r');

	help_screen_on();

	move(LINES - 1, 0);			/* clear the message */
	clrtoeol();

	/*
	 *	Redisplay the form/menu, but do not clear the screen.
	 */
	redisplay(MENU_NO_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */

	refresh();
} /* end menu_ack() */
