#ifndef lint
static	char	sccsid[] = "@(#)menu_mesg.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_mesg.c
 *
 *	Description:	Print a message on the menu or form and wait for
 *		an acknowledgement from the user.
 *
 *	Call syntax:	menu_mesg(fmt, va_alist);
 *
 *	Parameters:	char *		fmt;
 *			va_dcl
 */

#include <curses.h>
#include <varargs.h>
#include "menu.h"
#include "menu_impl.h"



void
_menu_mesg(fmt, ap)
        char *		fmt;
	va_list		ap;
{
	int		too_long;		/* is mesg too long? */
	int		x_coord, y_coord;	/* saved coordinates */


	if (!_menu_init_done) {
		(void) vfprintf(stderr, fmt, ap);
		(void) fputc('\n', stderr);

		menu_ack();

		return;
	}

	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	move(LINES - 2, 0);
	clrtoeol();
	standout();
	too_long = menu_print(fmt, ap);
	standend();

	menu_ack();

	move(LINES - 2, 0);			/* clear the message */
	clrtoeol();

	/*
	 *	Redisplay the form/menu, but do not clear the screen.
	 */
	redisplay(too_long ? MENU_CLEAR : MENU_NO_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */

	refresh();
} /* end _menu_mesg() */




/*VARARGS1*/
void
menu_mesg(fmt, va_alist)
        char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */


	va_start(ap);				/* init varargs stuff */

	_menu_mesg(fmt, ap);
} /* end menu_mesg() */
