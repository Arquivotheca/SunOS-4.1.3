#ifndef lint
static  char    sccsid[] = 	"@(#)ask_num.c 1.1 92/07/30 SMI";
#endif lint


/*
 *	Name:		menu_ask_num()
 *
 *	Description:	Print get imput for a number, and don't let anthing
 *			else pass
 *
 *	Return value:	the digit typed in (as a char)
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


char
menu_ask_num(x, y)
	int	x,y;				/* location to put ? */
{
	char		ch = 0;			/* the char that was read */
	char		ch1 = 0;		/* the char that was read */
	int		done = 0;		/* are we done yet? */
	
	move(y, x);

	refresh();

	while (!done) {
		ch1 = read_menu_char();

		switch (ch1) {
		case '\n':
		case '\r':
			if (isdigit(ch))
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
			if (isdigit(ch1)) {
				ch = ch1;
				mvaddch(y, x, ch);
			} else {
				ch = 0;
				break;
			}
		} /* end switch() */

		move(y, x);
		refresh();
	}

	return(ch);

} /* menu_ask_num() */

