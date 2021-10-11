#ifndef lint
static	char	sccsid[] = "@(#)find_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_item.c
 *
 *	Description:	Find a item.
 *
 *	Call syntax:	ip = find_menu_item(menu_p, name);
 *
 *	Parameters:	menu *		menu_p;
 *			char *		name;
 *
 *	Return value:	menu_item *	ip;
 */

#include <curses.h>
#include "menu.h"


menu_item *
find_menu_item(menu_p, name)
	menu *		menu_p;
	char *		name;
{
	menu_item *	ip;			/* the pointer to return */


						/* check each item */
	for (ip = menu_p->m_items; ip != NULL; ip = ip->mi_next)

		if (ip->mi_name && strcmp(name, ip->mi_name) == 0)
			return(ip);		/* found it */

	menu_log("%s: cannot find item.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_menu_item() */
