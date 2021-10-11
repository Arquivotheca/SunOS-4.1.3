#ifndef lint
static	char	sccsid[] = "@(#)display_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_yesno.c
 *
 *	Description:	Display a yes/no question.
 *
 *	Call syntax:	display_form_yesno(yesno_p);
 *
 *	Parameters:	form_yesno *	yesno_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form_yesno(yesno_p)
	form_yesno *	yesno_p;
{
	menu_string *	string_p;		/* pointer to string */


	if (yesno_p->fyn_active) {
		for (string_p = yesno_p->fyn_mstrings; string_p;
		     string_p = string_p->ms_next) {
			if ((pointer)yesno_p == _current_obj)
				standout();
			display_menu_string(string_p);
			if ((pointer)yesno_p == _current_obj)
				standend();
		}

		/*
		 *	Print the current answer:
		 */
		if (yesno_p->fyn_answerp)
			yesno_p->fyn_answer = *yesno_p->fyn_answerp;

		move((int) yesno_p->fyn_y, (int) yesno_p->fyn_x);
		switch (yesno_p->fyn_answer) {
		case 'n':
		case 'N':
			addch('n');
			break;

		case 'y':
		case 'Y':
			addch('y');
			break;

		default:
			addch(' ');
			break;
		} /* end switch */
	}
} /* end display_form_yesno() */
