#ifndef lint
static	char	sccsid[] = "@(#)free_map.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_map.c
 *
 *	Description:	Free a form map entry.
 *
 *	Call syntax:	free_form_map(form_p, obj_p, name_p);
 *
 *	Parameters:	form *		form_p;
 *			pointer		obj_p;
 *			char *		name_p;
 */

#include "menu.h"


void
free_form_map(form_p, obj_p, name_p)
	form *		form_p;
	pointer		obj_p;
	char *		name_p;
{
	form_map *	map_p;			/* ptr to target map entry */
	form_map *	mp;			/* ptr to scratch map entry */


	for (map_p = form_p->f_map; map_p; map_p = map_p->fm_next)
		if (map_p->fm_obj == obj_p)
			break;

	if (map_p == NULL) {
		menu_log("free_form_map(): cannot find object's map entry.");
		menu_log("\tobject name is %s.", name_p);
		menu_log("\tform name is %s", form_p->f_name);
		menu_abort(1);
	}

	/*
	 *	The target is the first field so just update the pointers
	 */
	if (map_p == form_p->f_map) {
		form_p->f_map = map_p->fm_next;
		form_p->f_map->fm_prev = NULL;
	}
	else {
		for (mp = form_p->f_map; mp; mp = mp->fm_next)
			if (mp->fm_next == map_p)
				break;

		mp->fm_next = map_p->fm_next;
		/*
		 *	If there is a next map entry, make sure it points
		 *	at its new predecessor.
		 */
		if (mp->fm_next)
			mp->fm_next->fm_prev = mp;
	}

	bzero((char *) map_p, sizeof(*map_p));
	free((char *) map_p);
} /* end free_form_map() */
