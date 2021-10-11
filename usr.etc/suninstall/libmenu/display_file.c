#ifndef lint
static	char	sccsid[] = "@(#)display_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_file.c
 *
 *	Description:	Display file.
 *
 *	Call syntax:	display_menu_file(file_p);
 *
 *	Parameters:	menu_file *	file_p;
 */

#include <curses.h>
#include "menu.h"


void
display_menu_file(file_p)
	menu_file *	file_p;
{
	char		buf[BUFSIZ];		/* input buffer */
	FILE *		fp;			/* pointer to file */
	int		len;			/* length of 'buf' */
	int		longest = 0;		/* length of longest string */
	pointer		saved_fm;		/* saved form/menu pointer */
	menu_string *	string_p;		/* pointer to string */
	int		x, y;			/* scratch coordinates */
	int		x_coord, y_coord;	/* saved coordinates */


	if (file_p->mf_active) {
		fp = fopen(file_p->mf_path, "r");
		if (fp == NULL) {
			menu_mesg("%s: cannot open file.", file_p->mf_path);
			return;
		}

		/*
		 *	Save the current form pointer and then clear it so
		 *	we do not get infinite recursion via menu_mesg().
		 */
		saved_fm = _current_fm;
		_current_fm = NULL;

						/* save where we were */
		getyx(stdscr, y_coord, x_coord);

		for (string_p = file_p->mf_mstrings; string_p;
		     string_p = string_p->ms_next) {
			display_menu_string(string_p);
		}

		x = file_p->mf_begin_x;
		y = file_p->mf_begin_y;
						/* read each line in file */
		while (fgets(buf, sizeof(buf), fp)) {
			len = strlen(buf) - 1;
			buf[len] = NULL;	/* get rid of end of line */

			if (len > longest)	/* save longest line length */
				longest = len;

			if (file_p->mf_gap && len + x > file_p->mf_end_x) {
				x = file_p->mf_begin_x;
				y = file_p->mf_begin_y;
				longest = 0;

				menu_mesg("More file text");

				clear_menu_file(file_p);
				for (string_p = file_p->mf_mstrings; string_p;
				     string_p = string_p->ms_next) {
					display_menu_string(string_p);
				}
			}

			mvaddstr(y, x, buf);
			y++;

			if (y > file_p->mf_end_y) {
				if (file_p->mf_gap) {
					y = file_p->mf_begin_y;
					x += (longest + file_p->mf_gap);
					longest = 0;
				}
				else {
					x = file_p->mf_begin_x;
					y = file_p->mf_begin_y;

					menu_mesg("More file text");

					clear_menu_file(file_p);
					for (string_p = file_p->mf_mstrings;
					     string_p;
					     string_p = string_p->ms_next) {
						display_menu_string(string_p);
					}
				}
			}
		} /* end for each line in file */

		(void) fclose(fp);

		move(y_coord, x_coord);		/* put cursor back */

		/*
		 *	Restore the saved pointer.
		 */
		_current_fm = saved_fm;
	}
} /* end display_menu_file() */
