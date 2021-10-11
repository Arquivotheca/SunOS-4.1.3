#ifndef lint
static	char	sccsid[] = "@(#)off_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_field.c
 *
 *	Description:	Turn off (deactivate) field.
 *
 *	Call syntax:	off_form_field(field_p);
 *
 *	Parameters:	form_field *	field_p;
 */

#include <curses.h>
#include "menu.h"


void
off_form_field(field_p)
	form_field *	field_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_form_field(field_p);

	field_p->ff_active = NOT_ACTIVE;
	for (string_p = field_p->ff_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_form_field() */
