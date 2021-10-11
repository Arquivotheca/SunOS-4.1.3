#ifndef lint
static	char	sccsid[] = "@(#)off_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_string.c
 *
 *	Description:	Turn off (deactivate) menu string.
 *
 *	Call syntax:	off_menu_string(string_p);
 *
 *	Parameters:	menu_string *	string_p;
 */

#include <curses.h>
#include "menu.h"


void
off_menu_string(string_p)
	menu_string *	string_p;
{
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_menu_string(string_p);

	string_p->ms_active = NOT_ACTIVE;

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_menu_string() */
