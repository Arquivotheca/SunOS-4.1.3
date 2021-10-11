#ifndef lint
static	char	sccsid[] = "@(#)clear_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_radio.c
 *
 *	Description:	Clear a radio.
 *
 *	Call syntax:	clear_form_radio(radio_p);
 *
 *	Parameters:	form_radio *	radio_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_form_radio(radio_p)
	form_radio *	radio_p;
{
	form_button *	button_p;		/* pointer to buttons */
	menu_string *	string_p;		/* pointer to string */


	if (radio_p->fr_active) {
		for (string_p = radio_p->fr_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}
		for (button_p = radio_p->fr_buttons; button_p;
		     button_p = button_p->fb_next) {
			clear_form_button(button_p);
		}
	}
} /* end clear_form_radio() */
