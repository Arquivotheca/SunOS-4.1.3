#ifndef lint
static	char	sccsid[] = "@(#)create_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		create_menu.c
 *
 *	Description:	Create a menu structure.  Allocates a menu structure
 *		using menu_alloc() and initializes the fields.  The name of the
 *		menu is not copied, and the data is expected to be static.
 *		Returns a pointer to the menu.
 *
 *	Call syntax:	menu_p = create_menu(help, active, name);
 *
 *	Parameters:	int (*		help)();
 *			int		active;
 *			char *		name;
 *
 *	Return value:	menu *		menu_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


menu *
create_menu(help, active, name)
	int (*		help)();
	int		active;
	char *		name;
{
	menu *		menu_p;			/* pointer to menu to return */


	menu_p = (menu *) menu_alloc(sizeof(*menu_p));
	menu_p->m_type = MENU_OBJ_MENU;
	menu_p->m_help = help;
	menu_p->m_active = active;
	menu_p->m_name = name;

	(void) add_menu_string((pointer) menu_p, active, 0, 1, ATTR_NORMAL,
			       name);
	(void) add_menu_string((pointer) menu_p, active, 0, 70, ATTR_STAND,
			       "[?=help]");
	(void) add_menu_string((pointer) menu_p, active, 1, 1,
			       ATTR_NORMAL,
"-----------------------------------------------------------------------------");
	(void) add_menu_string((pointer) menu_p, active,
			       (menu_coord) LINES - 1, 2, ATTR_STAND,
			       "[RET/SPACE=next choice]");
	(void) add_menu_string((pointer) menu_p, active,
			       (menu_coord) LINES - 1, 26, ATTR_STAND,
			       "[x/X=select choice]");
	(void) add_menu_string((pointer) menu_p, active,
			       (menu_coord) LINES - 1, 46, ATTR_STAND,
			       "[^B/^P=backward]");
	(void) add_menu_string((pointer) menu_p, active,
			       (menu_coord) LINES - 1, 63, ATTR_STAND,
			       "[^F/^N=forward]");

	return(menu_p);
} /* end create_menu() */
