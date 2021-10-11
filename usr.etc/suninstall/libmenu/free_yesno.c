#ifndef lint
static	char	sccsid[] = "@(#)free_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_yesno.c
 *
 *	Description:	Free a yes/no question.
 *
 *	Call syntax:	free_yesno(form_p, yesno_p);
 *
 *	Parameters:	form *		form_p;
 *			form_yesno *	yesno_p;
 */

#include "menu.h"


void
free_form_yesno(form_p, yesno_p)
	form *		form_p;
	form_yesno *	yesno_p;
{
	form_yesno *	ynp;			/* scratch yes/no pointer */


	if (form_p == NULL || yesno_p == NULL)
		return;

	free_form_map(form_p, (pointer) yesno_p, yesno_p->fyn_name);

	/*
	 *	The target is the first yes/no so just update the pointer
	 */
	if (form_p->f_yesnos == yesno_p) {
		form_p->f_yesnos = yesno_p->fyn_next;
	}
	/*
	 *	Find previous yes/no and update the pointer.
	 */
	else {
		for (ynp = form_p->f_yesnos; ynp; ynp = ynp->fyn_next)
			if (ynp->fyn_next == yesno_p)
				break;

		if (ynp == NULL) {
			menu_log("free_form_yesno(): cannot find yes/no.");
			menu_log("\tyesno name is %s.", yesno_p->fyn_name);
			menu_log("\tform name is %s.", form_p->f_name);
			menu_abort(1);
		}

		ynp->fyn_next = yesno_p->fyn_next;
	}

	if (yesno_p->fyn_prefunc == menu_confirm_func)
		free((char *) yesno_p->fyn_prearg);

	bzero((char *) yesno_p, sizeof(*yesno_p));
	free((char *) yesno_p);
} /* end free_form_yesno() */
