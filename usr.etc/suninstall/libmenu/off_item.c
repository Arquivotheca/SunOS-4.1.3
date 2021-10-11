#ifndef lint
static	char	sccsid[] = "@(#)off_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_item.c
 *
 *	Description:	Turn off (deactivate) menu item.
 *
 *	Call syntax:	off_menu_item(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 */

#include <curses.h>
#include "menu.h"


void
off_menu_item(item_p)
	menu_item *	item_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_menu_item(item_p);

	item_p->mi_active = NOT_ACTIVE;
	for (string_p = item_p->mi_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}

	/* 
	 *	Since this item is off it cannot be the currently
	 *	selected item.
	 */
	if (item_p->mi_menu->m_selected == item_p)
		item_p->mi_menu->m_selected = NULL;

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_menu_item() */
