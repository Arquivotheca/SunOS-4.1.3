#ifndef lint
static	char	sccsid[] = "@(#)item_status.c 1.1 92/07/30";
#endif

/*
 *	Name:		item_status.c
 *
 *	Description:	Display menu item status.
 *
 *	Call syntax:	display_item_status(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


void
display_item_status(item_p)
	menu_item *	item_p;
{
	mvaddch((int) item_p->mi_y, (int) item_p->mi_x + ITEM_OFFSET,
		(item_p->mi_chkvalue == 1) ? '+' : ' ');
} /* end display_item_status() */
