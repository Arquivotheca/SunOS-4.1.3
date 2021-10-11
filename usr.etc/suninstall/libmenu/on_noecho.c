#ifndef lint
static	char	sccsid[] = "@(#)on_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_noecho.c
 *
 *	Description:	Turn on (activate) noecho.
 *
 *	Call syntax:	on_form_noecho(noecho_p);
 *
 *	Parameters:	form_noecho *	noecho_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form_noecho(noecho_p)
	form_noecho *	noecho_p;
{
	menu_string *	string_p;		/* pointer to string */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	noecho_p->fne_active = ACTIVE;
	for (string_p = noecho_p->fne_mstrings; string_p;
	     string_p = string_p->ms_next) {
		on_menu_string(string_p);
	}

	display_form_noecho(noecho_p);

	move(y_coord, x_coord);			/* put cursor back */
} /* end on_form_noecho() */
