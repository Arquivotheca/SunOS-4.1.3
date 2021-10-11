#ifndef lint
static	char	sccsid[] = "@(#)free_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_noecho.c
 *
 *	Description:	Free a form noecho.
 *
 *	Call syntax:	free_form_noecho(form_p, noecho_p);
 *
 *	Parameters:	form *		form_p;
 *			form_noecho *	noecho_p;
 */

#include "menu.h"


void
free_form_noecho(form_p, noecho_p)
	form *		form_p;
	form_noecho *	noecho_p;
{
	form_noecho *	fp;			/* scratch noecho pointer */


	if (form_p == NULL || noecho_p == NULL)
		return;

	free_form_map(form_p, (pointer) noecho_p, noecho_p->fne_name);

	/*
	 *	The target is the first noecho so just update the pointer
	 */
	if (form_p->f_noechos == noecho_p) {
		form_p->f_noechos = noecho_p->fne_next;
	}
	/*
	 *	Find the previous noecho and update the pointer
	 */
	else {
		for (fp = form_p->f_noechos; fp; fp = fp->fne_next)
			if (fp->fne_next == noecho_p)
				break;

		if (fp == NULL) {
			menu_log("free_form_noecho(): cannot find noecho.");
			menu_log("\tnoecho name is %s.", noecho_p->fne_name);
			menu_log("\tform name is %s.", form_p->f_name);
			menu_abort(1);
		}

		fp->fne_next = noecho_p->fne_next;
	}

	bzero((char *) noecho_p, sizeof(*noecho_p));
	free((char *) noecho_p);
} /* end free_form_noecho() */
