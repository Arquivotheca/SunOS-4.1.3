#ifndef lint
static	char	sccsid[] = "@(#)find_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_radio.c
 *
 *	Description:	Find a radio.
 *
 *	Call syntax:	rp = find_form_radio(form_p, name);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *
 *	Return value:	form_radio *	rp;
 */

#include <curses.h>
#include "menu.h"


form_radio *
find_form_radio(form_p, name)
		form *		form_p;
		char *		name;
{
	form_radio *	rp;			/* the pointer to return */


						/* check each radio */
	for (rp = form_p->f_radios; rp != NULL; rp = rp->fr_next)

		if (rp->fr_name && strcmp(name, rp->fr_name) == 0)
			return(rp);		/* found it */

	menu_log("%s: cannot find radio panel.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_form_radio() */
