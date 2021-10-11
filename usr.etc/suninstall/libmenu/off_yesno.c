#ifndef lint
static	char	sccsid[] = "@(#)off_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_yesno.c
 *
 *	Description:	Turn off (deactivate) yes/no question.
 *
 *	Call syntax:	off_form_yesno(yesno_p);
 *
 *	Parameters:	form_yesno *	yesno_p;
 */

#include <curses.h>
#include "menu.h"


void
off_form_yesno(yesno_p)
	form_yesno *	yesno_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear_form_yesno(yesno_p);

	yesno_p->fyn_active = NOT_ACTIVE;
	for (string_p = yesno_p->fyn_mstrings; string_p;
	     string_p = string_p->ms_next) {
		off_menu_string(string_p);
	}

	move(y_coord, x_coord);			/* put cursor back */
} /* end off_form_yesno() */
