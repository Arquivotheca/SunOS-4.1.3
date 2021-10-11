#ifndef lint
static	char	sccsid[] = "@(#)clear_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_item.c
 *
 *	Description:	Clear menu item.
 *
 *	Call syntax:	clear_menu_item(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_menu_item(item_p)
	menu_item *	item_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	if (item_p->mi_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = item_p->mi_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}

		/*
		 *	The item we are clearing was selected so clear
		 *	the marker.  This is still the selected item
		 *	even though the display is clear.
		 */
		if (item_p->mi_menu->m_selected == item_p) {
			mvaddch((int) item_p->mi_y, (int) item_p->mi_x, ' ');
		}

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end clear_menu_item() */
