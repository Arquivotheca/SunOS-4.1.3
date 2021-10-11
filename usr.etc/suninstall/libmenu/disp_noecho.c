#ifndef lint
static	char	sccsid[] = "@(#)disp_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_noecho.c
 *
 *	Description:	Display a noecho.
 *
 *	Call syntax:	display_form_noecho(noecho_p);
 *
 *	Parameters:	form_noecho *	noecho_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form_noecho(noecho_p)
	form_noecho *	noecho_p;
{
	menu_string *	string_p;		/* pointer to string */


	if (noecho_p->fne_active) {
		for (string_p = noecho_p->fne_mstrings; string_p;
		     string_p = string_p->ms_next) {
			display_menu_string(string_p);
		}
	}
} /* end display_form_noecho() */
