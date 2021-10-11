#ifndef lint
static	char	sccsid[] = "@(#)read_char.c 1.1 92/07/30";
#endif

/*
 *	Name:		read_char.c
 *
 *	Description:	Read a character and perform any necessary
 *		translations.  Returns the character read.
 *
 *	Call syntax:	ch = read_menu_char();
 *
 *	Parameters:	char		ch;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


static	char		done = 0;		/* got characteristics? */
static	struct sgttyb	local;			/* terminal characteristics */
static	struct tchars	tlocal;			/* more characteristics */


char
read_menu_char()
{
	char		ch;			/* scratch character */


	if (done == 0) {
		(void) ioctl(0, TIOCGETP, &local);
		(void) ioctl(0, TIOCGETC, &tlocal);
		done = 1;
	}

	_menu_forward = 0;
	_menu_backward = 0;

	while (1) {
		if (read(0, &ch, sizeof(ch)) != 1)
			return(CEOT);

		if (ch == local.sg_erase || ch == CTRL(h)) {
			return(CERASE);
		}
		else if (ch == local.sg_kill) {
			return(CKILL);
		}
		else if (ch == tlocal.t_eofc) {
			return(CEOT);
		}
		else if (ch == C_BACKWARD || ch == CTRL(p)) {
			_menu_backward = 1;
			return(C_BACKWARD);
		}
		else if (ch == C_FORWARD || ch == CTRL(n)) {
			_menu_forward = 1;
			return(C_FORWARD);
		}
		else if (ch == C_REDRAW || ch == CTRL(r)) {
			/*
			 *	Redisplay the form/menu, and clear the screen.
			 */
			redisplay(MENU_CLEAR);
		}
		else if (ch == C_HELP && is_help_screen()) {
			/*
			 *	If there is a help function defined for the
			 *	current object, then use it.
			 */
			if (_current_obj &&
			    ((menu_core *) _current_obj)->m_help)
				(*((menu_core *) _current_obj)->m_help)();

			/*
			 *	If there is a help function defined for the
			 *	current form/menu, the use it.
			 */
			else if (_current_fm &&
				 ((menu_core *) _current_fm)->m_help)
				(*((menu_core *) _current_fm)->m_help)();
			/*
			 *	Use the default help function.
			 */
			else {
				switch (((menu_core *) _current_fm)->m_type) {
				case MENU_OBJ_FORM:
					form_help_screen();
					break;

				case MENU_OBJ_MENU:
					menu_help_screen();
					break;
				}
			}
		}
		else {
			return(ch);
		}
	}
} /* end read_menu_char() */
