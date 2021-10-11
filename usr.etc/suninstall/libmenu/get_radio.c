#ifndef lint
static	char	sccsid[] = "@(#)get_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		get_radio.c
 *
 *	Description:	Get a form radio button.  The following values are
 *		returned:
 *
 *			 1		- radio button is valid
 *			 0		- an error occurred
 *			-1		- a fatal error occurred
 *			MENU_IGNORE_OBJ	- radio is not active
 *			MENU_SKIP_IO	- skip object I/O, but no post function
 *
 *	Call syntax:	ret_code = get_form_radio(radio_p);
 *
 *	Parameters:	form_radio *	radio_p;
 *
 *	Return value:	int		ret_code;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


int
get_form_radio(radio_p)
	form_radio *	radio_p;
{
	form_button *	button_p;		/* button pointer */
	int		done = 0;		/* are we done yet? */
	form_button *	pressed_p = NULL;	/* was button pressed? */
	int		ret_code;		/* return code */


	if (radio_p->fr_active == 0)		/* not an active radio */
		return(MENU_IGNORE_OBJ);

	if (radio_p->fr_prefunc) {		/* invoke the pre-function */
		ret_code = (*radio_p->fr_prefunc)(radio_p->fr_prearg);

		/*
		 *	Skip I/O for this object and just do the post
		 *	function.  If there is no post function, then
		 *	return MENU_SKIP_IO.
		 */
		if (ret_code == MENU_SKIP_IO && radio_p->fr_postfunc)
			done = 1;

		else if (ret_code != 1)
			return(ret_code);
	}

	button_p = radio_p->fr_buttons;

	while (button_p && !done) {
		while (button_p->fb_active == 0) {
			button_p = button_p->fb_next;
			if (button_p == NULL)
				button_p = radio_p->fr_buttons;
		}

		move((int) button_p->fb_y, (int) button_p->fb_x);
		refresh();

		switch (read_menu_char()) {
		case C_BACKWARD:
		case C_FORWARD:
			done = 1;
			break;

		case '\n':
		case '\r':
			if (radio_p->fr_pressed)
				done = 1;
			break;

		case ' ':
			button_p = button_p->fb_next;
			if (button_p == NULL)
				button_p = radio_p->fr_buttons;
			move((int) button_p->fb_y, (int) button_p->fb_x);
			break;

		case 'x':
		case 'X':
			if (radio_p->fr_pressed)
				mvaddch((int) radio_p->fr_pressed->fb_y,
					(int) radio_p->fr_pressed->fb_x, ' ');

			pressed_p = button_p;
			radio_p->fr_pressed = button_p;
			if (radio_p->fr_codeptr)
				*radio_p->fr_codeptr = button_p->fb_code;
			mvaddch((int) button_p->fb_y, (int) button_p->fb_x,
				'x');
			done = 1;
			break;
		} /* end switch() */
	} /* end while not done */

						/* invoke the button func */
	if (pressed_p && pressed_p->fb_func) {
		ret_code = (*pressed_p->fb_func)(pressed_p->fb_arg);

		switch (ret_code) {
		case 1:
			break;

		default:
			if (radio_p->fr_pressed)
				mvaddch((int) radio_p->fr_pressed->fb_y,
					(int) radio_p->fr_pressed->fb_x, ' ');
			radio_p->fr_pressed = NULL;
			if (radio_p->fr_codeptr)
				*radio_p->fr_codeptr = 0;

			/*
			 *	Falling through here:
			 */
		case MENU_GOTO_OBJ:
			return(ret_code);
		} /* end switch */
	}
						/* invoke the post-function */
	if (radio_p->fr_postfunc) {
		ret_code = (*radio_p->fr_postfunc)(radio_p->fr_postarg);

		switch (ret_code) {
		case 1:
			break;

		default:
			if (radio_p->fr_pressed)
				mvaddch((int) radio_p->fr_pressed->fb_y,
					(int) radio_p->fr_pressed->fb_x, ' ');
			radio_p->fr_pressed = NULL;
			if (radio_p->fr_codeptr)
				*radio_p->fr_codeptr = 0;

			/*
			 *	Falling through here:
			 */
		case MENU_GOTO_OBJ:
			return(ret_code);
		} /* end switch */
	}

	return(1);
} /* end get_form_radio() */
