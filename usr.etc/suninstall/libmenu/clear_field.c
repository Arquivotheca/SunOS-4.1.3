#ifndef lint
static	char	sccsid[] = "@(#)clear_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_field.c
 *
 *	Description:	Clear field.
 *
 *	Call syntax:	clear_form_field(field_p);
 *
 *	Parameters:	form_field *	field_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_form_field(field_p)
	form_field *	field_p;
{
	int		stop;			/* stopping point */
	menu_string *	string_p;		/* pointer to string */
	int		x;			/* scratch x-coordinate */
	int		x_coord, y_coord;	/* saved coordinates */


	if (field_p->ff_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = field_p->ff_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}

		stop = field_p->ff_x + field_p->ff_width;

		for (x = field_p->ff_x; x < stop; x++)
			mvaddch((int) field_p->ff_y, x, ' ');

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end clear_form_field() */
