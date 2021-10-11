#ifndef lint
static	char	sccsid[] = "@(#)free_form.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_form.c
 *
 *	Description:	Free a form.
 *
 *	Call syntax:	free_form(form_p);
 *
 *	Parameters:	form *		form_p;
 */

#include "menu.h"


void
free_form(form_p)
	form *		form_p;
{
	if (form_p == NULL)
		return;

	while (form_p->f_mstrings)
		free_menu_string((pointer) form_p, form_p->f_mstrings);

	while (form_p->f_fields)
		free_form_field(form_p, form_p->f_fields);

	while (form_p->f_files)
		free_menu_file((pointer) form_p, form_p->f_files);

	while (form_p->f_noechos)
		free_form_noecho(form_p, form_p->f_noechos);

	while (form_p->f_radios)
		free_form_radio(form_p, form_p->f_radios);

	while (form_p->f_yesnos)
		free_form_yesno(form_p, form_p->f_yesnos);

	bzero((char *) form_p, sizeof(*form_p));
	free((char *) form_p);
} /* end free_form() */
