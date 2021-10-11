#ifndef lint
static	char	sccsid[] = "@(#)menu_print.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_print.c
 *
 *	Description:	Low level print interface for menmu system.
 *
 *	Call syntax:	menu_print(fmt, ap);
 *
 *	Parameters:	char *		fmt;
 *			va_list		ap;
 */

#include <curses.h>
#include <varargs.h>


int
menu_print(fmt, ap)
        char *		fmt;
	va_list		ap;
{
	char		buf[BUFSIZ];		/* buffer for string */
	int		too_long = 0;		/* is the line too long? */
	int		x_coord, y_coord;	/* saved coordinates */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

#ifdef lint
	y_coord = y_coord;
#endif lint

	(void) vsprintf(buf, fmt, ap);			/* do the formatting */

	/*
	 *	Line is too long for the message line.
	 */
	if (x_coord + strlen(buf) > COLS) {
		too_long = 1;
		scrollok(stdscr, TRUE);
	}

	(void) waddstr(stdscr, buf);		/* add it to the screen */
	(void) wclrtoeol(stdscr);		/* clear anything after it */

	/*
	 *	If the line was too long, then scroll the screen so the
	 *	user can see the whole message.
	 */
	if (too_long) {
		(void) scroll(stdscr);
		(void) wrefresh(stdscr);
		scrollok(stdscr, FALSE);
	}

	return(too_long);
} /* end menu_print() */
