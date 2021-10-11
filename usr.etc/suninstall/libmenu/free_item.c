#ifndef lint
static	char	sccsid[] = "@(#)free_item.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_item.c
 *
 *	Description:	Free a menu item.
 *
 *	Call syntax:	free_menu_item(menu_p, item_p);
 *
 *	Parameters:	menu *		menu_p;
 *			menu_item *	item_p;
 */

#include "menu.h"


void
free_menu_item(menu_p, item_p)
	menu *		menu_p;
	menu_item *	item_p;
{
	menu_item *	ip;			/* scratch menu item pointer */


	if (menu_p == NULL || item_p == NULL)
		return;

	/*
	 *	The target is the first menu item so just update the pointers
	 */
	if (menu_p->m_items == item_p) {
		menu_p->m_items = item_p->mi_next;
		item_p->mi_prev = NULL;
	}
	/*
	 *	Find the previous menu item and update the pointers.
	 */
	else {
		for (ip = menu_p->m_items; ip; ip = ip->mi_next)
			if (ip->mi_next == item_p)
				break;

		if (ip == NULL) {
			menu_log("free_menu_item(): cannot find item.");
			menu_log("\titem name is %s.", item_p->mi_name);
			menu_log("\tmenu name is %s.", menu_p->m_name);
			menu_abort(1);
		}

		ip->mi_next = item_p->mi_next;
		/*
		 *	If there is a next object, then make sure it points
		 *	at its new predecessor.
		 */
		if (ip->mi_next)
			ip->mi_next->mi_prev = ip;
	}

	bzero((char *) item_p, sizeof(*item_p));
	free((char *) item_p);
} /* end free_menu_item() */
