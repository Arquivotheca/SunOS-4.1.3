#ifndef lint
static	char	sccsid[] = "@(#)get_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		get_yesno.c
 *
 *	Description:	Get the answer to a yes/no question.  The following
 *		values are returned:
 *
 *			 1		- yes/no answer is valid
 *			 0		- an error occurred
 *			-1		- a fatal error occurred
 *			MENU_IGNORE_OBJ	- yes/no is not active
 *			MENU_SKIP_IO	- skip object I/O, but no answer ptr
 *
 *	Call syntax:	ret_code = get_form_yesno(yesno_p);
 *
 *	Parameters:	form_yesno *	yesno_p;
 *
 *	Return value:	int		ret_code;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


int
get_form_yesno(yesno_p)
	form_yesno *	yesno_p;
{
	int		done = 0;		/* are we done yet? */
	char		pressed = 0;		/* key that was pressed */
	int		ret_code = 1;		/* return code */
	int		force_yn = 0;		/* require that we get ans */


	if (yesno_p->fyn_active == 0)		/* not an active yesno */
		return(MENU_IGNORE_OBJ);

	if (yesno_p->fyn_prefunc) {		/* invoke the pre-function */
		ret_code = (*yesno_p->fyn_prefunc)(yesno_p->fyn_prearg);

		/*
		 *	Skip I/O for this object and just do the post
		 *	function.  If there is no answer pointer, then
		 *	return MENU_SKIP_IO.  If there is no function
		 *	for the appropriate answer, then MENU_SKIP_IO
		 *	is returned at the end of the function.
		 */
		if (ret_code == MENU_SKIP_IO && yesno_p->fyn_answerp) {
			pressed = *yesno_p->fyn_answerp;
			done = 1;
		}
		else if (ret_code == MENU_GET_YN) {
			force_yn = 1;
			ret_code = 1;
		}
		else if (ret_code == MENU_EXIT_MENU) {
			yesno_p->fyn_answer = 'y';
			if (yesno_p->fyn_answerp)
				*yesno_p->fyn_answerp = 'y';
			return(ret_code);
		}
		else if (ret_code != 1)
			return(ret_code);
	}

	while (!done) {
		if (yesno_p->fyn_answerp)
			yesno_p->fyn_answer = *yesno_p->fyn_answerp;

		move((int) yesno_p->fyn_y, (int) yesno_p->fyn_x);
		switch (yesno_p->fyn_answer) {
		case 'Y':
		case 'y':
			addch('y');
			break;

		case 'N':
		case 'n':
			addch('n');
			break;

		default:
			addch(' ');
		} /* end switch */
		move((int) yesno_p->fyn_y, (int) yesno_p->fyn_x);

		refresh();

		switch (read_menu_char()) {
		case C_BACKWARD:
		case C_FORWARD:
			if (!force_yn || yesno_p->fyn_answer == 'n' ||
			    yesno_p->fyn_answer == 'y')
			done = 1;
			break;

		case '\n':
		case '\r':
			if (yesno_p->fyn_answer == 'n' ||
			    yesno_p->fyn_answer == 'y')
				done = 1;
			break;

		case 'n':
		case 'N':
			mvaddch((int) yesno_p->fyn_y, (int) yesno_p->fyn_x,
				'n');
			pressed = 'n';
			yesno_p->fyn_answer = 'n';
			if (yesno_p->fyn_answerp)
				*yesno_p->fyn_answerp = 'n';
			break;

		case 'y':
		case 'Y':
			mvaddch((int) yesno_p->fyn_y, (int) yesno_p->fyn_x,
				'y');
			pressed = 'y';
			yesno_p->fyn_answer = 'y';
			if (yesno_p->fyn_answerp)
				*yesno_p->fyn_answerp = 'y';
			break;

		case CERASE:   
			/*
			 * If no characters pressed then break
			 */
			if (pressed == 0)
				break;
			mvaddch((int) yesno_p->fyn_y, (int) yesno_p->fyn_x,
				' ');
			pressed = ' ';
			yesno_p->fyn_answer = ' ';
			if (yesno_p->fyn_answerp)
				*yesno_p->fyn_answerp = ' ';
			break;

		default:
			continue;
		} /* end switch() */
	} /* end while not done */

	if (pressed == 'y' && yesno_p->fyn_yesfunc) {
		return((*yesno_p->fyn_yesfunc)(yesno_p->fyn_yesarg));
	}
	else if (pressed == 'n' && yesno_p->fyn_nofunc) {
		return((*yesno_p->fyn_nofunc)(yesno_p->fyn_noarg));
	}

	/*
	 *	If we get here then there is no function for the
	 *	answer that was given.  The value returned here
	 *	is either 1 (from initialization) or is MENU_SKIP_IO
	 *	from the pre function.
	 */
	return(ret_code);
} /* end get_form_yesno() */
