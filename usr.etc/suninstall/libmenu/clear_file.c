#ifndef lint
static	char	sccsid[] = "@(#)clear_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_file.c
 *
 *	Description:	Clear file.
 *
 *	Call syntax:	clear_menu_file(file_p);
 *
 *	Parameters:	menu_file *	file_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_menu_file(file_p)
	menu_file *	file_p;
{
	menu_string *	string_p;		/* pointer to menu string */
	int		x, y;			/* scratch coordinates */
	int		x_coord, y_coord;	/* saved coordinates */


	if (file_p->mf_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = file_p->mf_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}

		/*
		 *	Clear the area (slowly).
		 */
		for (y = file_p->mf_begin_y; y <= file_p->mf_end_y; y++)
			for (x = file_p->mf_begin_x; x <= file_p->mf_end_x; x++)
				mvaddch(y, x, ' ');

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end clear_menu_file() */
