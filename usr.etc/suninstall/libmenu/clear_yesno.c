#ifndef lint
static	char	sccsid[] = "@(#)clear_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_yesno.c
 *
 *	Description:	Clear a yes/no question.
 *
 *	Call syntax:	clear_form_yesno(yesno_p);
 *
 *	Parameters:	form_yesno *	yesno_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_form_yesno(yesno_p)
	form_yesno *	yesno_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	if (yesno_p->fyn_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = yesno_p->fyn_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}

		/*
		 *	Clear the current answer:
		 */
		mvaddch((int) yesno_p->fyn_y, (int) yesno_p->fyn_x, ' ');

		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end clear_form_yesno() */
