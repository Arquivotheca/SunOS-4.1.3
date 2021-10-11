#ifndef lint
static	char	sccsid[] = "@(#)free_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_menu.c
 *
 *	Description:	Free a menu.
 *
 *	Call syntax:	free_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 */

#include "menu.h"


void
free_menu(menu_p)
	menu *		menu_p;
{
	if (menu_p == NULL)
		return;

	while (menu_p->m_mstrings)
		free_menu_string((pointer) menu_p, menu_p->m_mstrings);

	while (menu_p->m_files)
		free_menu_file((pointer) menu_p, menu_p->m_files);

	while (menu_p->m_items)
		free_menu_item(menu_p, menu_p->m_items);

	bzero((char *) menu_p, sizeof(*menu_p));
	free((char *) menu_p);
} /* end free_menu() */
