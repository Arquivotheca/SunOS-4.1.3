#ifndef lint
static	char	sccsid[] = "@(#)display_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_radio.c
 *
 *	Description:	Display a radio.
 *
 *	Call syntax:	display_form_radio(radio_p);
 *
 *	Parameters:	form_radio *	radio_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form_radio(radio_p)
	form_radio *	radio_p;
{
	form_button *	button_p;		/* pointer to buttons */
	menu_string *	string_p;		/* pointer to string */


	if (radio_p->fr_active) {
		for (string_p = radio_p->fr_mstrings; string_p;
		     string_p = string_p->ms_next) {
			if ((pointer)radio_p == _current_obj)
				standout();
			display_menu_string(string_p);
			if ((pointer)radio_p == _current_obj)
				standend();
		}
		for (button_p = radio_p->fr_buttons; button_p;
		     button_p = button_p->fb_next) {
			display_form_button(button_p);
		}
	}
} /* end display_form_radio() */
