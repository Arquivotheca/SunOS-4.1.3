#ifndef lint
static	char	sccsid[] = "@(#)add_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_item.c
 *
 *	Description:	Add a menu item to a menu.  Allocates a menu item
 *		structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the menu item.
 *
 *	Call syntax:	item_p = add_menu_item(menu_p, help, active, name,
 *					       y_coord, x_coord,
 *					       pre_func, pre_arg, func, arg,
 *					       chk_func, chk_arg);
 *
 *	Parameters:	menu *		menu_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		func)();
 *			pointer		arg;
 *			int (*		chk_func)();
 *			pointer		chk_arg;
 *
 *	Return value:	menu_item *	item_p;
 */

#include "menu.h"
#include "menu_impl.h"


menu_item *
add_menu_item(menu_p, help, active, name, y_coord, x_coord,
	      pre_func, pre_arg, func, arg, chk_func, chk_arg)
	menu *		menu_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	y_coord;
	menu_coord	x_coord;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		func)();
	pointer		arg;
	int (*		chk_func)();
	pointer		chk_arg;
{
	menu_item **	item_pp;		/* ptr to new menu item ptr */
	menu_item *	last_p = NULL;		/* ptr to last menu item */


	for (item_pp = &menu_p->m_items; *item_pp;) {
		last_p = *item_pp;
		item_pp = &(*item_pp)->mi_next;
	}

	*item_pp = (menu_item *) menu_alloc(sizeof(**item_pp));

	(*item_pp)->m_type = MENU_OBJ_ITEM;
	(*item_pp)->m_help = help;
	(*item_pp)->mi_active = active;
	(*item_pp)->mi_name = name;
	(*item_pp)->mi_x = x_coord;
	(*item_pp)->mi_y = y_coord;
	(*item_pp)->mi_prefunc = pre_func;
	(*item_pp)->mi_prearg = pre_arg;
	(*item_pp)->mi_func = func;
	(*item_pp)->mi_arg = arg;
	(*item_pp)->mi_chkfunc = chk_func;
	(*item_pp)->mi_chkarg = chk_arg;
	(*item_pp)->mi_prev = last_p;
	(*item_pp)->mi_menu = menu_p;

	return(*item_pp);
} /* end add_menu_item() */
