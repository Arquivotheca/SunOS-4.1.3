#ifndef lint
static	char	sccsid[] = "@(#)find_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_field.c
 *
 *	Description:	Find a field.
 *
 *	Call syntax:	fp = find_form_field(form_p, name);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *
 *	Return value:	form_field *	fp;
 */

#include <curses.h>
#include "menu.h"


form_field *
find_form_field(form_p, name)
	form *		form_p;
	char *		name;
{
	form_field *	fp;			/* the pointer to return */


						/* check each field */
	for (fp = form_p->f_fields; fp != NULL; fp = fp->ff_next)

		if (fp->ff_name && strcmp(name, fp->ff_name) == 0)
			return(fp);		/* found it */

	menu_log("%s: cannot find field.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_form_field() */
