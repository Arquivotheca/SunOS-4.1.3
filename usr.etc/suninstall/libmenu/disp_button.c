#ifndef lint
static	char	sccsid[] = "@(#)disp_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		disp_button.c
 *
 *	Description:	Display radio button.
 *
 *	Call syntax:	display_form_button(button_p);
 *
 *	Parameters:	form_button *	button_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form_button(button_p)
	form_button *	button_p;
{
	menu_string *	string_p;		/* pointer to string */


	if (button_p->fb_active == ACTIVE) {
		for (string_p = button_p->fb_mstrings; string_p;
		     string_p = string_p->ms_next) {
			display_menu_string(string_p);
		}

		/*
		 *	If this button matches the code, then it
		 *	is the currently pressed button.
		 */
		if (button_p->fb_radio->fr_codeptr &&
		    *button_p->fb_radio->fr_codeptr == button_p->fb_code) {
			button_p->fb_radio->fr_pressed = button_p;

			mvaddch((int) button_p->fb_y, (int) button_p->fb_x,
				'x');
		}

		move((int) button_p->fb_y, (int) button_p->fb_x);
	}
} /* end display_form_button() */
