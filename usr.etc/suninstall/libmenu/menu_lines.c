#ifndef lint
static	char	sccsid[] = "@(#)menu_lines.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_lines.c
 *
 *	Description:	Return the number of lines on the screen in the
 *		menu system.
 *
 *	Call syntax:	count = menu_lines();
 *
 *	Return value:	menu_coord	count;
 */

#include <curses.h>
#include "menu.h"




menu_coord
menu_lines()
{
	return(LINES);
} /* end menu_lines() */
