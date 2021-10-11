#ifndef lint
static	char	sccsid[] = "@(#)menu_cols.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_cols.c
 *
 *	Description:	Return the number of columns on the screen in the
 *		menu system.
 *
 *	Call syntax:	count = menu_cols();
 *
 *	Return value:	menu_coord	count;
 */

#include <curses.h>
#include "menu.h"




menu_coord
menu_cols()
{
	return(COLS);
} /* end menu_cols() */
