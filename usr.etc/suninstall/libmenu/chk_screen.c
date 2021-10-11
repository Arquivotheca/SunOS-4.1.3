#ifndef lint
#ident			"@(#)chk_screen.c 1.1 92/07/30 SMI";
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <curses.h>


/****************************************************************************
**
**	Name:		chk_screen_size
**
**	Description:	Determine if the screen is at least 24 by 80
**
**	Call syntax:	ret_code = chk_screen_size();
**
**	Return value:	exits if screen is too small
**			
**
****************************************************************************
*/
void
chk_screen_size()
{
	if ((LINES < 24) || (COLS < 80)) {
		endwin();
		fprintf(stderr,"The display has to be at least 24 rows by 80 columns\n");
		exit(1);
	}


} /* end chk_swap() */

