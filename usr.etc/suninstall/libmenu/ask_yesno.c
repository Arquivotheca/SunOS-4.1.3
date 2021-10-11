#ifndef lint
static	char	sccsid[] = "@(#)ask_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		ask_yesno.c
 *
 *	Description:	Print a yes/no question on the menu or form and wait
 *		for the answer.  Returns 1 is the answer is yes, and 0 if
 *		the answer is no.
 *
 *	Call syntax:	ret_code = menu_ask_yesno(display,fmt, va_alist);
 *
 *	Parameters:	int		display;
 *			char *		fmt;
 *			va_dcl
 *
 *	Return value:	int		ret_code;
 */

#include <curses.h>
#include <varargs.h>
#include "menu.h"
#include "menu_impl.h"


/*VARARGS1*/
int
menu_ask_yesno(display,fmt, va_alist)
	int		display;
        char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */
	char		ch = 0;			/* the char that was read */
	int		done = 0;		/* are we done yet? */
	int		too_long;		/* is message too long? */
	int		x_coord, y_coord;	/* saved coordinates */
	int		x, y;			/* current coordinates */


	va_start(ap);				/* init varargs stuff */

	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	move(LINES - 2, 0);
	clrtoeol();
	standout();
	too_long = menu_print(fmt, ap);
	too_long += menu_print(" [y/n] ", (va_list) NULL);
	standend();
	refresh();

	getyx(stdscr, y, x);	/* save where we were */

	help_screen_off();

	while (!done) {
		switch (read_menu_char()) {
		case 'Y':
		case 'y':
			ch = 'y';
			mvaddch(y, x, ch);
			break;

		case 'n':
		case 'N':
			ch = 'n';
			mvaddch(y, x, ch);
			break;

		case '\n':
		case '\r':
			if (ch == 'n' || ch == 'y')
				done = 1;
			break;	
		
		case CERASE:   
			/*
			 * If no characters pressed then break
			 */
			if (ch == 0)
				break;
			ch = 0;
			mvaddch(y, x, ' ');
			break;

		default:
			ch = 0;
			break;
		
		} /* end switch() */

		move(y, x);
		refresh();
	}

	help_screen_on();

	move(LINES - 2, 0);			/* clear the message */
	clrtoeol();

	/*
	 *	Redisplay the form/menu
	 */
	if (display)
		redisplay(too_long ? MENU_CLEAR : MENU_NO_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */

	refresh();

	return(ch == 'y');
} /* end menu_ask_yesno() */
