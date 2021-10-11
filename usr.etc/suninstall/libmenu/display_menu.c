#ifndef lint
static	char	sccsid[] = "@(#)display_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_menu.c
 *
 *	Description:	Display a menu.
 *
 *	Call syntax:	display_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 */

#include <curses.h>
#include "menu.h"


void
display_menu(menu_p)
	menu *		menu_p;
{
	menu_file *	file_p;			/* ptr to a file object */
	menu_item *	item_p;			/* ptr to a menu item */
	menu_string *	string_p;		/* ptr to a string */


	if (menu_p->m_active == NOT_ACTIVE)
		return;

	for (string_p = menu_p->m_mstrings; string_p;
	     string_p = string_p->ms_next) {
		display_menu_string(string_p);
	}

	for (item_p = menu_p->m_items; item_p;
	     item_p = item_p->mi_next) {
		display_menu_item(item_p);
	}

	/*
	 *	Display file objects after regular objects because a file
	 *	object may use menu_mesg() and stop the display.  The file
	 *	objects are before the shared objects, because the shared
	 *	objects use the same line as menu_mesg().
	 */
	for (file_p = menu_p->m_files; file_p;
	     file_p = file_p->mf_next) {
		display_menu_file(file_p);
	}

	/*
	 *	If there is a finish object, then display it.
	 */
	if (menu_p->m_finish)
		display_form_yesno(menu_p->m_finish);

	/*
	 *	Repeat the strings one more time.  If a file object pauses
	 *	the screen we want all the strings to be displayed except
	 *	those in conflict.  We redisplay the strings here just in
	 *	case a string was in conflict.
	 */
	for (string_p = menu_p->m_mstrings; string_p;
	     string_p = string_p->ms_next) {
		display_menu_string(string_p);
	}

	refresh();
} /* end display_menu() */
