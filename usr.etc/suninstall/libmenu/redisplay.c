#ifndef lint
static	char	sccsid[] = "@(#)redisplay.c 1.1 92/07/30";
#endif

/*
 *	Name:		redisplay.c
 *
 *	Description:	Redisplays the current form or menu.
 *
 *	Call syntax:	redisplay_form();
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


void
redisplay(clear_screen)
	int		clear_screen;
{
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);

	if (clear_screen == MENU_CLEAR)
		clear();

	if (_current_fm) {
		switch (((menu_core *) _current_fm)->m_type) {
		case MENU_OBJ_FORM:
			display_form((form *) _current_fm);
			break;

		case MENU_OBJ_MENU:
			display_menu((menu *) _current_fm);
			break;
		} /* end switch */
	}

	move(y_coord, x_coord);

	refresh();
} /* end redisplay() */
