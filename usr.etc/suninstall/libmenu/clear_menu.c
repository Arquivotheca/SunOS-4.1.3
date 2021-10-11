#ifndef lint
static	char	sccsid[] = "@(#)clear_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_menu.c
 *
 *	Description:	Clear a menu.
 *
 *	Call syntax:	clear_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_menu(menu_p)
	menu *		menu_p;
{
	menu_file *	file_p;			/* ptr to a file object */
	menu_item *	item_p;			/* ptr to a menu item */
	menu_string *	string_p;		/* ptr to a string */


	if (menu_p->m_active == NOT_ACTIVE)
		return;

	for (string_p = menu_p->m_mstrings; string_p;
	     string_p = string_p->ms_next) {
		clear_menu_string(string_p);
	}
	for (file_p = menu_p->m_files; file_p;
	     file_p = file_p->mf_next) {
		clear_menu_file(file_p);
	}
	for (item_p = menu_p->m_items; item_p;
	     item_p = item_p->mi_next) {
		clear_menu_item(item_p);
	}

	if (menu_p->m_finish)
		clear_form_yesno(menu_p->m_finish);
} /* end clear_menu() */
