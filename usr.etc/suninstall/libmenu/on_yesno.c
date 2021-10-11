#ifndef lint
static	char	sccsid[] = "@(#)on_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_yesno.c
 *
 *	Description:	Turn on (activate) yes/no question.
 *
 *	Call syntax:	on_form_yesno(yesno_p);
 *
 *	Parameters:	form_yesno *	yesno_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form_yesno(yesno_p)
	form_yesno *	yesno_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	yesno_p->fyn_active = ACTIVE;
	for (string_p = yesno_p->fyn_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_form_yesno(yesno_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_form_yesno() */
