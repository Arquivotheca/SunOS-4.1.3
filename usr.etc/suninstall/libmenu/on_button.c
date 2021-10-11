#ifndef lint
static	char	sccsid[] = "@(#)on_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_button.c
 *
 *	Description:	Turn on (activate) radio button.
 *
 *	Call syntax:	on_form_button(button_p);
 *
 *	Parameters:	form_button *	button_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form_button(button_p)
	form_button *	button_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where cursor was */

	button_p->fb_active = ACTIVE;
	for (string_p = button_p->fb_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_form_button(button_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_form_button() */
