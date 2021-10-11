#ifndef lint
static	char	sccsid[] = "@(#)off_shared.c 1.1 92/07/30";
#endif

/*
 *	Name:		off_shared.c
 *
 *	Description:	Turn off (deactivate) a shared yes/no question.
 *
 *	Call syntax:	off_form_shared(form_p, shared_p);
 *
 *	Parameters:	form *		form_p;
 *			form_yesno *	shared_p;
 */

#include <curses.h>
#include "menu.h"


void
off_form_shared(form_p, shared_p)
	form *		form_p;
	form_yesno *	shared_p;
{
	off_form_yesno(shared_p);

	/*
	 *	This was the active shared object.  So set the active
	 *	shared object to the finish object.
	 */
	if (form_p->f_shared == shared_p)
		form_p->f_shared = form_p->f_finish;

	if (form_p->f_shared)
		display_form_yesno(form_p->f_shared);
} /* end off_form_shared() */
