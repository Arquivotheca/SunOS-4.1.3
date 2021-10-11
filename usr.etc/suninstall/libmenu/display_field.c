#ifndef lint
static	char	sccsid[] = "@(#)display_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_field.c
 *
 *	Description:	Display field.
 *
 *	Call syntax:	display_form_field(field_p);
 *
 *	Parameters:	form_field *	field_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form_field(field_p)
	form_field *	field_p;
{
	char *		end_p;			/* ptr to end of field */
	char *		start_p;		/* ptr to start of field */
	menu_string *	string_p;		/* pointer to string */
	int		x;			/* scratch x-coordinate */
	int		x_coord, y_coord;	/* saved coordinates */


	if (field_p->ff_active) {
						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = field_p->ff_mstrings; string_p;
		     string_p = string_p->ms_next) {
			if ((pointer)field_p == _current_obj)
				standout();
			display_menu_string(string_p);
			if ((pointer)field_p == _current_obj)
				standend();
		}

		/*
		 *	Find the starting and ending point of the
		 *	window into the data and display it
		 */
		start_p = field_p->ff_data;
		end_p = &field_p->ff_data[strlen(field_p->ff_data)];
		if (end_p - start_p > field_p->ff_width)
			start_p = end_p - field_p->ff_width;

		mvaddstr((int) field_p->ff_y, (int) field_p->ff_x, start_p);

		/*
		 *	Window is full so put the cursor on
		 *	top of the last character.
		 */
		if (end_p - field_p->ff_data == field_p->ff_width) {
			move((int) field_p->ff_y, (int) (field_p->ff_x +
			     field_p->ff_width - 1));
		}
		/*
		 *	The window is scrolling.
		 */
		else if (end_p - field_p->ff_data > field_p->ff_width) {
			mvaddch((int) field_p->ff_y, (int) field_p->ff_x,
				'<');
			move((int) field_p->ff_y, (int) (field_p->ff_x +
			     field_p->ff_width - 1));
		}
		else {
			for (x = (int) (end_p - field_p->ff_data);
			     x < field_p->ff_width; x++)
				mvaddch((int) field_p->ff_y,
					(int) field_p->ff_x + x, ' ');
		}

/****
		move((int)field_p->ff_y, (int)field_p->ff_x + end_p - start_p);	
		(void) wclrtoeol(stdscr);
****/
		move(y_coord, x_coord);		/* put cursor back */
	}
} /* end display_form_field() */
