#ifndef lint
static	char	sccsid[] = "@(#)free_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_field.c
 *
 *	Description:	Free a form field.
 *
 *	Call syntax:	free_form_field(form_p, field_p);
 *
 *	Parameters:	form *		form_p;
 *			form_field *	field_p;
 */

#include "menu.h"


void
free_form_field(form_p, field_p)
	form *		form_p;
	form_field *	field_p;
{
	form_field *	fp;			/* scratch field pointer */


	if (form_p == NULL || field_p == NULL)
		return;

	free_form_map(form_p, (pointer) field_p, field_p->ff_name);

	/*
	 *	The target is the first field so just update the pointer
	 */
	if (form_p->f_fields == field_p) {
		form_p->f_fields = field_p->ff_next;
	}
	/*
	 *	Find the previous field and update the pointer
	 */
	else {
		for (fp = form_p->f_fields; fp; fp = fp->ff_next)
			if (fp->ff_next == field_p)
				break;

		if (fp == NULL) {
			menu_log("free_form_field(): cannot find field.");
			menu_log("\tfield name is %s.", field_p->ff_name);
			menu_log("\tform name is %s.", form_p->f_name);
			menu_abort(1);
		}

		fp->ff_next = field_p->ff_next;
	}

	bzero((char *) field_p, sizeof(*field_p));
	free((char *) field_p);
} /* end free_form_field() */
