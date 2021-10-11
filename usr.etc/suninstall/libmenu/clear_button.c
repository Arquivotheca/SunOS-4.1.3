#ifndef lint
static	char	sccsid[] = "@(#)clear_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_button.c
 *
 *	Description:	Clear radio button.
 *
 *	Call syntax:	clear_form_button(button_p);
 *
 *	Parameters:	form_button *	button_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_form_button(button_p)
	form_button *	button_p;
{
	menu_string *	string_p;		/* pointer to string */


	if (button_p->fb_active == ACTIVE) {
		for (string_p = button_p->fb_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}
		/*
		 *	Clear the currently pressed button.  This button is
		 *	still the currently pressed button even though the
		 *	display is clear.
		 */
		if (button_p->fb_radio->fr_pressed == button_p)
			mvaddch((int) button_p->fb_y, (int) button_p->fb_x,
				' ');
	}
} /* end clear_form_button() */
