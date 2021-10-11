#ifndef lint
static	char	sccsid[] = "@(#)off_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_radio.c
 *
 *	Description:	Turn off (deactivate) a radio.
 *
 *	Call syntax:	off_form_radio(radio_p);
 *
 *	Parameters:	form_radio *	radio_p;
 */

#include <curses.h>
#include "menu.h"


void
off_form_radio(radio_p)
	form_radio *	radio_p;
{
	form_button *	button_p;		/* pointer to buttons */
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_form_radio(radio_p);

	radio_p->fr_active = NOT_ACTIVE;
	for (string_p = radio_p->fr_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}
	for (button_p = radio_p->fr_buttons; button_p;
	     button_p = button_p->fb_next) {
		off_form_button(button_p);
	}

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_form_radio() */
