#ifndef lint
static	char	sccsid[] = "@(#)on_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_file.c
 *
 *	Description:	Turn on (activate) file object.
 *
 *	Call syntax:	on_menu_file(file_p);
 *
 *	Parameters:	menu_file *	file_p;
 */

#include <curses.h>
#include "menu.h"


void
on_menu_file(file_p)
	menu_file *	file_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	file_p->mf_active = ACTIVE;
	for (string_p = file_p->mf_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_menu_file(file_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_menu_file() */
