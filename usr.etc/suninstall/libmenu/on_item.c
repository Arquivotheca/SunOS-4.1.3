#ifndef lint
static	char	sccsid[] = "@(#)on_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_item.c
 *
 *	Description:	Turn on (activate) menu item.
 *
 *	Call syntax:	on_menu_item(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 */

#include <curses.h>
#include "menu.h"


void
on_menu_item(item_p)
	menu_item *	item_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	item_p->mi_active = ACTIVE;
	for (string_p = item_p->mi_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_menu_item(item_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_menu_item() */
