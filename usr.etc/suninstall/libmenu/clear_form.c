#ifndef lint
static	char	sccsid[] = "@(#)clear_form.c 1.1 92/07/30";
#endif

/*
 *	Name:		clear_form.c
 *
 *	Description:	Clear a form.
 *
 *	Call syntax:	clear_form(form_p);
 *
 *	Parameters:	form *		form_p;
 */

#include "menu.h"


void
clear_form(form_p)
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
		clear_menu_string(string_p);
	}
	for (field_p = form_p->f_fields; field_p;
	     field_p = field_p->ff_next) {
		clear_form_field(field_p);
	}
	for (file_p = form_p->f_files; file_p;
	     file_p = file_p->mf_next) {
		clear_menu_file(file_p);
	}
	for (noecho_p = form_p->f_noechos; noecho_p;
	     noecho_p = noecho_p->fne_next) {
		clear_form_noecho(noecho_p);
	}
	for (radio_p = form_p->f_radios; radio_p;
	     radio_p = radio_p->fr_next) {
		clear_form_radio(radio_p);
	}
	for (yesno_p = form_p->f_yesnos; yesno_p;
	     yesno_p = yesno_p->fyn_next) {
		clear_form_yesno(yesno_p);
	}
} /* end clear_form() */
