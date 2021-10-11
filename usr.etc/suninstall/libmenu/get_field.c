#ifndef lint
static	char	sccsid[] = "@(#)get_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		get_field.c
 *
 *	Description:	Get a form field.  The following values are returned:
 *
 *			 1		- field is valid
 *			 0		- an error occurred
 *			-1		- a fatal error occurred
 *			MENU_IGNORE_OBJ	- field is not active
 *			MENU_SKIP_IO	- skip object I/O, but no post function
 *
 *	Call syntax:	ret_code = get_form_field(field_p);
 *
 *	Parameters:	form_field *	field_p;
 *
 *	Return value:	int		ret_code;
 */

#include <ctype.h>
#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


int
get_form_field(field_p)
	form_field *	field_p;
{
	char		ch;			/* current character */
	char *		cp;			/* ptr into data buffer */
	int		done = 0;		/* are we done yet? */
	char *		end_p;			/* ptr to end of field */
	int		ret_code;		/* return code */
	char *		start_p;		/* ptr to start of field */
	int		x;			/* scratch x-coordinate */


	if (field_p->ff_active == 0)		/* not an active field */
		return(MENU_IGNORE_OBJ);

	if (field_p->ff_prefunc) {		/* invoke the pre-function */
		ret_code = (*field_p->ff_prefunc)(field_p->ff_prearg,
						  field_p->ff_data);

		/*
		 *	Skip I/O for this object and just do the post
		 *	function.  If there is no post function, then
		 *	return MENU_SKIP_IO.
		 */
		if (ret_code == MENU_SKIP_IO && field_p->ff_postfunc)
			done = 1;

		else if (ret_code != 1)
			return(ret_code);
	}

	if (!done) {
		/*
		 *	Find the starting and ending point of the window
		 *	into the data
		 */
		start_p = field_p->ff_data;
		end_p = &field_p->ff_data[strlen(field_p->ff_data)];
		cp = end_p;
		if (end_p - start_p > field_p->ff_width)
			start_p = end_p - field_p->ff_width;

		/*
		 *	Find the x-coordinate.  If the window is full,
		 *	then the cursor sits on top of the last character.
		 */
		if (strlen(start_p) < field_p->ff_width)
			x = field_p->ff_x + strlen(start_p);
		else
			x = field_p->ff_x + strlen(start_p) - 1;
		move((int) field_p->ff_y, x);
	}

	while (!done) {
		refresh();

		switch (ch = read_menu_char()) {
		case C_BACKWARD:
		case C_FORWARD:
		case '\n':
		case '\r':
			*cp = '\0';
			done = 1;
			break;

		case CERASE:
			/*
			 *	Nothing to erase
			 */
			if (cp == field_p->ff_data)
				break;

			/*
			 *	No scrolling to worry about
			 */
			if (start_p == field_p->ff_data) {
				/*
				 *	Cursor is past the last character
				 */
				if (end_p - start_p < field_p->ff_width)
					mvaddch((int) field_p->ff_y, --x, ' ');
				/*
				 *	Cursor is on top of the last character
				 *	and make sure '<' is replaced.
				 */
				else {
					mvaddch((int) field_p->ff_y, x, ' ');
					mvaddch((int) field_p->ff_y,
						(int) field_p->ff_x,
						*start_p);
				}
				move((int) field_p->ff_y, x);
			}
			/*
			 *	The window to the data is scrolled
			 */
			else {
				/*
				 *	Replace the '<'
				 */
				mvdelch((int) field_p->ff_y, x);
				mvaddch((int) field_p->ff_y, (int)
					field_p->ff_x, *start_p);
				--start_p;
				/*
				 *	No more scrolling needed
				 */
				if (start_p == field_p->ff_data)
					mvinsch((int) field_p->ff_y,
						(int) field_p->ff_x,
						*start_p);
				/*
				 *	Put back the scroll indicator
				 */
				else
					mvinsch((int) field_p->ff_y,
						(int) field_p->ff_x,
						'<');
				move((int) field_p->ff_y, x);
			}
			--end_p;
			*(--cp) = '\0';
			break;

		case CKILL:
			/*
			 *	Cursor is on top of the last character
			 *	so erase it specially.
			 */
			if (end_p - start_p == field_p->ff_width) {
				mvaddch((int) field_p->ff_y, x, ' ');
				move((int) field_p->ff_y, x);
				*(--cp) = '\0';
			}

			/*
			 *	Let refresh() figure out what is optimal.
			 */
			for (; cp > start_p;) {
				mvaddch((int) field_p->ff_y, --x, ' ');
				*(--cp) = '\0';
                        }
			/*
			 *	Erase any data that was scrolled.
			 */
			for (; cp > field_p->ff_data;)
				*(--cp) = '\0';
			move((int) field_p->ff_y, x);
			start_p = field_p->ff_data;
			end_p = field_p->ff_data;
			break;

		default:
			/*
			 *	No more room
			 */
			if (cp >= &field_p->ff_data[field_p->ff_datasize - 1])
				continue;

			/*
			 *	Add it to the buffer it is a valid char.
			 */
			if ((field_p->ff_lex &&
			     (*field_p->ff_lex)(field_p->ff_data, cp, ch)) ||
			    (field_p->ff_lex == NULL && isprint(ch))) {
				*cp++ = ch;
				*cp = '\0';
				end_p++;
				/*
				 *	Not full window yet
				 */
				if (end_p - start_p < field_p->ff_width) {
					mvaddch((int) field_p->ff_y, x++, ch);
				}
				/*
				 *	Window is full so put the cursor on
				 *	top of the last character.
				 */
				else if (end_p - start_p == field_p->ff_width) {
					mvaddch((int) field_p->ff_y, x, ch);
					move((int) field_p->ff_y, x);
				}
				/*
				 *	The window is scrolling.
				 */
				else {
					start_p++;
					mvdelch((int) field_p->ff_y,
						(int) field_p->ff_x);
					mvaddch((int) field_p->ff_y,
						(int) field_p->ff_x,
						'<');
					mvinsch((int) field_p->ff_y, x, ch);
				}
			}
			break;
		} /* end switch */
	} /* end while not done */

	if (field_p->ff_postfunc)		/* invoke the post-function */
		return((*field_p->ff_postfunc)(field_p->ff_postarg,
					       field_p->ff_data));

	return(1);
} /* end get_form_field() */
