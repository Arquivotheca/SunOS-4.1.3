#ifndef lint
static	char	sccsid[] = "@(#)find_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_noecho.c
 *
 *	Description:	Find a noecho.
 *
 *	Call syntax:	fp = find_form_noecho(form_p, name);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *
 *	Return value:	form_noecho *	fp;
 */

#include <curses.h>
#include "menu.h"


form_noecho *
find_form_noecho(form_p, name)
	form *		form_p;
	char *		name;
{
	form_noecho *	fp;			/* the pointer to return */


						/* check each noecho */
	for (fp = form_p->f_noechos; fp != NULL; fp = fp->fne_next)

		if (fp->fne_name && strcmp(name, fp->fne_name) == 0)
			return(fp);		/* found it */

	menu_log("%s: cannot find noecho.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_form_noecho() */
