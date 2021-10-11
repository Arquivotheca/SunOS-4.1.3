#ifndef lint
static	char	sccsid[] = "@(#)clear_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_string.c
 *
 *	Description:	Clear a menu string.
 *
 *	Call syntax:	clear_menu_string(string_p);
 *
 *	Parameters:	menu_string *	string_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_menu_string(string_p)
	menu_string *	string_p;
{
	int		stop;			/* stopping point */
	int		x;			/* scratch x-coordinate */
	int		x_coord, y_coord;	/* saved coordinates */


	if (string_p->ms_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		stop = string_p->ms_x + strlen(string_p->ms_data);

		for (x = string_p->ms_x; x < stop; x++)
			mvaddch((int) string_p->ms_y, x, ' ');

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end clear_menu_string() */
