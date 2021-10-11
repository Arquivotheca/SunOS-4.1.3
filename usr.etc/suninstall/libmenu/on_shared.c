#ifndef lint
static	char	sccsid[] = "@(#)on_shared.c 1.1 92/07/30";
#endif

/*
 *	Name:		on_shared.c
 *
 *	Description:	Turn on (activate) a shared yes/no question.
 *
 *	Call syntax:	on_form_shared(form_p, shared_p);
 *
 *	Parameters:	form *		form_p;
 *			form_yesno *	shared_p;
 */

#include <curses.h>
#include "menu.h"


void
on_form_shared(form_p, shared_p)
	form *		form_p;
	form_yesno *	shared_p;
{
	if (form_p->f_shared)
		clear_form_yesno(form_p->f_shared);

	form_p->f_shared = shared_p;

	on_form_yesno(shared_p);
} /* end on_form_shared() */
