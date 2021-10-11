#ifndef lint
static	char	sccsid[] = "@(#)on_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_string.c
 *
 *	Description:	Turn on (activate) menu string.
 *
 *	Call syntax:	on_menu_string(string_p);
 *
 *	Parameters:	menu_string *	string_p;
 */

#include <curses.h>
#include "menu.h"


void
on_menu_string(string_p)
	menu_string *	string_p;
{
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	string_p->ms_active = ACTIVE;
	display_menu_string(string_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_menu_string() */
