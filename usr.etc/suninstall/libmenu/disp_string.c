#ifndef lint
static	char	sccsid[] = "@(#)disp_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		disp_string.c
 *
 *	Description:	Display a menu string.
 *
 *	Call syntax:	display_menu_string(string_p);
 *
 *	Parameters:	menu_string *	string_p;
 */

#include <curses.h>
#include "menu.h"


void
display_menu_string(string_p)
	menu_string *	string_p;
{
	int		x_coord, y_coord;	/* saved coordinates */


	if (string_p->ms_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		if (string_p->ms_attribute == ATTR_STAND)
			standout();

		mvaddstr((int) string_p->ms_y, (int) string_p->ms_x,
			 string_p->ms_data);

		if (string_p->ms_attribute == ATTR_STAND)
			standend();

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end display_menu_string() */
