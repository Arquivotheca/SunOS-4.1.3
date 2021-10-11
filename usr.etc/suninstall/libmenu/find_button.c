#ifndef lint
static	char	sccsid[] = "@(#)find_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_button.c
 *
 *	Description:	Find a radio button.
 *
 *	Call syntax:	bp = find_form_button(form_p, name);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *
 *	Return value:	form_button *	bp;
 */

#include <curses.h>
#include "menu.h"


form_button *
find_form_button(form_p, name)
	form *		form_p;
	char *		name;
{
	form_button *	bp;			/* the pointer to return */
	form_radio *	rp;			/* scratch radio pointer */


						/* check each radio panel */
	for (rp = form_p->f_radios; rp != NULL; rp = rp->fr_next) {

						/* check each button */
		for (bp = rp->fr_buttons; bp != NULL; bp = bp->fb_next)

			if (bp->fb_name && strcmp(name, bp->fb_name) == 0)
				return(bp);	/* found the button */
	}

	menu_log("%s: cannot find radio button.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_form_button() */
