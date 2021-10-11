#ifndef lint
static	char	sccsid[] = "@(#)free_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_radio.c
 *
 *	Description:	Free a radio panel.
 *
 *	Call syntax:	free_radio(form_p, radio_p)
 *
 *	Parameters:	form *		form_p;
 *			form_radio *	radio_p;
 */

#include "menu.h"


void
free_form_radio(form_p, radio_p)
	form *		form_p;
	form_radio *	radio_p;
{
	form_radio *	fp;			/* scratch radio pointer */


	if (form_p == NULL || radio_p == NULL)
		return;

	free_form_map(form_p, (pointer) radio_p, radio_p->fr_name);

	/*
	 *	Free the buttons first
	 */
	while (radio_p->fr_buttons)
		free_form_button(radio_p, radio_p->fr_buttons);

	/*
	 *	The target is the first radio so just update the pointer
	 */
	if (form_p->f_radios == radio_p) {
		form_p->f_radios = radio_p->fr_next;
	}
	/*
	 *	Find the previous radio panel and update the pointer
	 */
	else {
		for (fp = form_p->f_radios; fp; fp = fp->fr_next)
			if (fp->fr_next == radio_p)
				break;

		if (fp == NULL) {
			menu_log("free_form_radio(): cannot find radio.");
			menu_log("\tradio name is %s.", radio_p->fr_name);
			menu_log("\tform name is %s.", form_p->f_name);
			menu_abort(1);
		}

		fp->fr_next = radio_p->fr_next;
	}

	bzero((char *) radio_p, sizeof(*radio_p));
	free((char *) radio_p);
} /* end free_form_radio() */
