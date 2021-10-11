#ifndef lint
static	char	sccsid[] = "@(#)off_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_file.c
 *
 *	Description:	Turn off (deactivate) file object.
 *
 *	Call syntax:	off_menu_file(file_p);
 *
 *	Parameters:	menu_file *	file_p;
 */

#include <curses.h>
#include "menu.h"


void
off_menu_file(file_p)
	menu_file *	file_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_menu_file(file_p);

	file_p->mf_active = NOT_ACTIVE;
	for (string_p = file_p->mf_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_menu_file() */
