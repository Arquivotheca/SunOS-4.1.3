#ifndef lint
static	char	sccsid[] = "@(#)find_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_yesno.c
 *
 *	Description:	Find a yes/no question.
 *
 *	Call syntax:	ynp = find_form_yesno(form_p, name);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *
 *	Return value:	form_yesno *	ynp;
 */

#include <curses.h>
#include "menu.h"


form_yesno *
find_form_yesno(form_p, name)
	form *		form_p;
	char *		name;
{
	form_yesno *	ynp;			/* the pointer to return */


						/* check each yes/no question */
	for (ynp = form_p->f_yesnos; ynp != NULL; ynp = ynp->fyn_next)

		if (ynp->fyn_name && strcmp(name, ynp->fyn_name) == 0)
			return(ynp);		/* found it */

	menu_log("%s: cannot find yes/no question.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_form_yesno() */
