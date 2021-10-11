#ifndef lint
static	char	sccsid[] = "@(#)menu_abort.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_abort.c
 *
 *	Description:	Abort from the menu system.  'exit_code' is passed
 *		to exit().
 *
 *	Call syntax:	menu_abort(exit_code);
 *
 *	Parameters:	int		exit_code;
 */

#include "menu.h"


void
menu_abort(exit_code)
	int		exit_code;
{
	end_menu();
	exit(exit_code);
} /* end menu_abort() */
