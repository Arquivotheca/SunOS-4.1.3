#ifndef lint
static	char	sccsid[] = "@(#)off_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_menu.c
 *
 *	Description:	Turn off (deactivate) a menu.
 *
 *	Call syntax:	off_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 */

#include <curses.h>
#include "menu.h"


void
off_menu(menu_p)
	menu *		menu_p;
{
	menu_file *	file_p;			/* ptr to a file object */
	menu_item *	item_p;			/* ptr to a menu item */
	menu_string *	string_p;		/* ptr to a string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_menu(menu_p);

	for (string_p = menu_p->m_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}
	for (file_p = menu_p->m_files; file_p; file_p = file_p->mf_next) {
		off_menu_file(file_p);
	}
	for (item_p = menu_p->m_items; item_p; item_p = item_p->mi_next) {
		off_menu_item(item_p);
	}

	if (menu_p->m_finish)
		off_form_yesno(menu_p->m_finish);

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_menu() */
