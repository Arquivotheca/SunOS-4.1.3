#ifndef lint
static	char	sccsid[] = "@(#)display_form.c 1.1 92/07/30";
#endif

/*
 *	Name:		display_form.c
 *
 *	Description:	Display a form.
 *
 *	Call syntax:	display_form(form_p);
 *
 *	Parameters:	form *		form_p;
 */

#include <curses.h>
#include "menu.h"


void
display_form(form_p)
	form *		form_p;
{
	form_field *	field_p;		/* ptr to a field */
	menu_file *	file_p;			/* ptr to a file object */
	form_noecho *	noecho_p;		/* ptr to noecho object */
	form_radio *	radio_p;		/* ptr to a radio */
	menu_string *	string_p;		/* ptr to a string */
	form_yesno *	yesno_p;		/* ptr to a yes/no question */


	if (form_p->f_active == NOT_ACTIVE)
		return;

	for (string_p = form_p->f_mstrings; string_p;
	     string_p = string_p->ms_next) {
		display_menu_string(string_p);
	}
	for (field_p = form_p->f_fields; field_p;
	     field_p = field_p->ff_next) {
		display_form_field(field_p);
	}
	for (radio_p = form_p->f_radios; radio_p;
	     radio_p = radio_p->fr_next) {
		display_form_radio(radio_p);
	}
	for (noecho_p = form_p->f_noechos; noecho_p;
	     noecho_p = noecho_p->fne_next) {
		display_form_noecho(noecho_p);
	}
	for (yesno_p = form_p->f_yesnos; yesno_p;
	     yesno_p = yesno_p->fyn_next) {
		/*
		 *	If there is a shared object and this object
		 *	shares the same line as the shared object,
		 *	then don't display it.
		 */
		if (form_p->f_shared && form_p->f_shared != yesno_p &&
		    form_p->f_shared->fyn_y == yesno_p->fyn_y)
			continue;

		display_form_yesno(yesno_p);
	}

	/*
	 *	Display file objects after regular objects because a file
	 *	object may use menu_mesg() and stop the display.  The file
	 *	objects are before the shared objects, because the shared
	 *	objects use the same line as menu_mesg().
	 */
	for (file_p = form_p->f_files; file_p;
	     file_p = file_p->mf_next) {
		display_menu_file(file_p);
	}

	/*
	 *	If there is a shared object, then display it.
	 */
	if (form_p->f_shared)
		display_form_yesno(form_p->f_shared);

	/*
	 *	Repeat the strings one more time.  If a file object pauses
	 *	the screen we want all the strings to be displayed except
	 *	those in conflict.  We redisplay the strings here just in
	 *	case a string was in conflict.
	 */
	for (string_p = form_p->f_mstrings; string_p;
	     string_p = string_p->ms_next) {
		display_menu_string(string_p);
	}

	refresh();
} /* end display_form() */
