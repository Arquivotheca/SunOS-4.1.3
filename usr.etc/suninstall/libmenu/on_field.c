#ifndef lint
static	char	sccsid[] = "@(#)on_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_field.c
 *
 *	Description:	Turn on (activate) field.
 *
 *	Call syntax:	on_form_field(field_p);
 *
 *	Parameters:	form_field *	field_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form_field(field_p)
	form_field *	field_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	field_p->ff_active = ACTIVE;
	for (string_p = field_p->ff_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_form_field(field_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_form_field() */
