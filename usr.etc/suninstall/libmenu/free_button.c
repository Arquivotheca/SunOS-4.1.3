#ifndef lint
static	char	sccsid[] = "@(#)free_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_button.c
 *
 *	Description:	Free a radio button.
 *
 *	Call syntax:	free_form_button(radio_p, button_p);
 *
 *	Parameters:	form_radio *	radio_p;
 *			form_button *	button_p;
 */

#include "menu.h"


void
free_form_button(radio_p, button_p)
	form_radio *	radio_p;
	form_button *	button_p;
{
	form_button *	bp;			/* scratch button pointer */


	if (radio_p == NULL || button_p == NULL)
		return;

	/*
	 *	The target is the first button so just update the pointer
	 */
	if (radio_p->fr_buttons == button_p) {
		radio_p->fr_buttons = button_p->fb_next;
	}
	/*
	 *	Find the previous button and update the pointer.
	 */
	else {
		for (bp = radio_p->fr_buttons; bp; bp = bp->fb_next)
			if (bp->fb_next == button_p)
				break;

		if (bp == NULL) {
			menu_log("free_form_button(): cannot find button.");
			menu_log("\tbutton name is %s.", button_p->fb_name);
			menu_log("\tradio name is %s.", radio_p->fr_name);
			menu_abort(1);
		}

		bp->fb_next = button_p->fb_next;
	}

	bzero((char *) button_p, sizeof(*button_p));
	free((char *) button_p);
} /* end free_form_button() */
