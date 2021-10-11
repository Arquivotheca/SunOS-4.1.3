#ifndef lint
static	char	sccsid[] = "@(#)off_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_button.c
 *
 *	Description:	Turn off (deactivate) radio button.
 *
 *	Call syntax:	off_form_button(button_p);
 *
 *	Parameters:	form_button *	button_p;
 */

#include <curses.h>
#include "menu.h"


void
off_form_button(button_p)
	form_button *	button_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_form_button(button_p);

	button_p->fb_active = NOT_ACTIVE;
	for (string_p = button_p->fb_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}

	/*
	 *	Since this button is off it cannot be the pressed button.
	 */
	if (button_p->fb_radio->fr_pressed == button_p)
		button_p->fb_radio->fr_pressed = NULL;

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_form_button() */
