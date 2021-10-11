#ifndef lint
static	char	sccsid[] = "@(#)on_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_menu.c
 *
 *	Description:	Turn on (activate) a menu.
 *
 *	Call syntax:	on_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 */

#include <curses.h>
#include "menu.h"


void
on_menu(menu_p)
	menu *		menu_p;
{
	menu_file *	file_p;			/* ptr to file object */
	menu_item *	item_p;			/* ptr to menu item */
	menu_string *	string_p;		/* ptr to a string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	for (string_p = menu_p->m_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}
	for (file_p = menu_p->m_files; file_p; file_p = file_p->mf_next) {
		on_menu_file(file_p);
	}
	for (item_p = menu_p->m_items; item_p; item_p = item_p->mi_next) {
		on_menu_item(item_p);
	}

	if (menu_p->m_finish)
		on_form_yesno(menu_p->m_finish);

	display_menu(menu_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_menu() */
