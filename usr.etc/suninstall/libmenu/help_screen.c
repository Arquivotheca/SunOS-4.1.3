#ifndef lint
static	char	sccsid[] = "@(#)help_screen.c 1.1 92/07/30";
#endif

/*
 *	Name:		help_screen.c
 *
 *	Description:	Thie file contains the two default help screens.
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


static	int		help_state = 1;




/*
 *	Name:		form_help_screen()
 *
 *	Description:	Print help screen for forms.
 *
 *	Call syntax:	form_help_screen()
 */

static	char *		form_lines[] = {
"",
"                           ON-LINE HELP FOR FORMS",
" ------------------------------------------------------------------------------",
"",
"",
"             -----------------------------------------------------",
"                KEYS                        PURPOSE               ",
"             -----------------------------------------------------",
"                CONTROL B                   move to previous object",
"                CONTROL P                   move to previous object",
"                CONTROL F                   move to next object",
"                CONTROL N                   move to next object",
"",
"                CONTROL U                   erase field",
"                <DELETE>                    erase one character",
"                <RETURN>                    end field input",
"                x or X                      select a button",
"                <SPACE>                     move to next button",
"",
"                CONTROL L                   repaint screen",
"                CONTROL C                   abort",

CP_NULL
};


void
form_help_screen()
{
	pointer		saved_fm;		/* pointer to saved form */
	int		x_coord, y_coord;	/* saved coordinates */
	int		y;			/* scratch y-coordinate */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear(); 

	for (y = 0; form_lines[y]; y++) {
		if (form_lines[y][0] == NULL)	/* skip blank lines */
			continue;

		mvaddstr(y, 0, form_lines[y]);
	}

	refresh();

	/*
	 *	Wait for an acknowledge from the user.  Clear _current_fm
	 *	so menu_ack does not try to display the form/menu on top
	 *	of the help screen.
	 */
	saved_fm = _current_fm;
	_current_fm = NULL;

	menu_ack();

	_current_fm = saved_fm;

	/*
	 *	Redisplay the form/menu, and clear the screen.
	 */
	redisplay(MENU_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */
	refresh();
} /* end form_help_screen() */




/*
 *	Name:		help_screen_off()
 *
 *	Description:	Disable check for help screens.
 *
 *	Call syntax:	help_screen_off();
 */

void
help_screen_off()
{
	help_state = 0;
} /* end help_screen_off() */




/*
 *	Name:		help_screen_on()
 *
 *	Description:	Enable check for help screens.
 *
 *	Call syntax:	help_screen_on();
 */

void
help_screen_on()
{
	help_state = 1;
} /* end help_screen_on() */




/*
 *	Name:		is_help_screen()
 *
 *	Description:	Return the status of the help screen system.
 *
 *	Call syntax:	ret_code = is_help_screen();
 *
 *	Return value:	int		ret_code;
 */

int
is_help_screen()
{
	return(help_state);
} /* end help_screen_on() */




/*
 *	Name:		menu_help_screen()
 *
 *	Description:	Print help screen for menus.
 *
 *	Call syntax:	menu_help_screen()
 */

static	char *		menu_help_lines[] = {
"",
"                           ON-LINE HELP FOR MENUS",
" ------------------------------------------------------------------------------",
"",
"",
"             -----------------------------------------------------",
"                KEYS                        PURPOSE               ",
"             -----------------------------------------------------",
"                CONTROL B                   move to previous item",
"                CONTROL P                   move to previous item",
"                CONTROL F                   move to next item",
"                CONTROL N                   move to next item",
"                <RETURN>                    move to next item",
"                <SPACE>                     move to next item",
"",
"                x or X                      select an item",
"",
"                CONTROL L                   repaint screen",
"                CONTROL C                   abort",

CP_NULL
};


void
menu_help_screen()
{
	pointer		saved_fm;		/* pointer to saved form */
	int		x_coord, y_coord;	/* saved coordinates */
	int		y;			/* scratch y-coordinate */


	getyx(stdscr, y_coord, x_coord);	/* save where we were */

	clear(); 

	for (y = 0; menu_help_lines[y]; y++) {
						/* skip blank lines */
		if (menu_help_lines[y][0] == NULL)
			continue;

		mvaddstr(y, 0, menu_help_lines[y]);
	}

	refresh();

	/*
	 *	Wait for an acknowledge from the user.  Clear _current_fm
	 *	so menu_ack does not try to display the form/menu on top
	 *	of the help screen.
	 */
	saved_fm = _current_fm;
	_current_fm = NULL;

	menu_ack();

	_current_fm = saved_fm;

	/*
	 *	Redisplay the form/menu, and clear the screen.
	 */
	redisplay(MENU_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */
	refresh();
} /* end menu_help_screen() */
