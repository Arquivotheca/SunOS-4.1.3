#ifndef lint
static	char	sccsid[] = "@(#)set_menu_term.c 1.1 92/07/30";
#endif

/*
 *	Name:		set_menu_term.c
 *
 *	Description:	Set up the menu system for the given terminal type.
 *
 *	Call syntax:	set_menu_term(term);
 *
 *	Parameters:	char *		term;
 */

#include <curses.h>




void
set_menu_term(term)
	char *		term;
{
	Def_term = term;
	My_term = 1;
} /* end set_menu_term() */
