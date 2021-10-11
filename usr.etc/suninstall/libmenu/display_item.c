#ifndef lint
static	char	sccsid[] = "@(#)display_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_item.c
 *
 *	Description:	Display menu item.
 *
 *	Call syntax:	display_menu_item(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 */

#include <curses.h>
#include "menu.h"


void
display_menu_item(item_p)
	menu_item *	item_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	if (item_p->mi_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = item_p->mi_mstrings; string_p;
		     string_p = string_p->ms_next) {
			display_menu_string(string_p);
		}

		/*
		 *	Display the selected item marker.
		 */
		if (item_p->mi_menu->m_selected == item_p)
			mvaddch((int) item_p->mi_y, (int) item_p->mi_x, 'x');

		/*
		 *	Display the check function status.
		 */
		display_item_status(item_p);

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end display_menu_item() */
