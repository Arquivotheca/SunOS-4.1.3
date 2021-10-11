#ifndef lint
static	char	sccsid[] = "@(#)clear_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_noecho.c
 *
 *	Description:	Clear a noecho.
 *
 *	Call syntax:	clear_form_noecho(noecho_p);
 *
 *	Parameters:	form_noecho *	noecho_p;
 */

#include <curses.h>
#include "menu.h"


void
clear_form_noecho(noecho_p)
	form_noecho *	noecho_p;
{
	menu_string *	string_p;		/* pointer to string */


	if (noecho_p->fne_active) {
		for (string_p = noecho_p->fne_mstrings; string_p;
		     string_p = string_p->ms_next) {
			clear_menu_string(string_p);
		}
	}
} /* end clear_form_noecho() */
