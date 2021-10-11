#ifndef lint
static	char	sccsid[] = "@(#)free_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_file.c
 *
 *	Description:	Free a file object.
 *
 *	Call syntax:	free_menu_file(obj_p, file_p);
 *
 *	Parameters:	pointer		obj_p;
 *			menu_file *	file_p;
 */

#include "menu.h"
#include "menu_impl.h"


void
free_menu_file(obj_p, file_p)
	pointer		obj_p;
	menu_file *	file_p;
{
	menu_file **	fpp;			/* ptr to file obj pointer */
	char *		name_p;			/* pointer to object name */


	if (obj_p == NULL || file_p == NULL)
		return;


	switch(((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_FORM:
		fpp = &((form *) obj_p)->f_files;
		name_p = ((form *) obj_p)->f_name;
		break;

	case MENU_OBJ_MENU:
		fpp = &((menu *) obj_p)->m_files;
		name_p = ((menu *) obj_p)->m_name;
		break;

	default:
		menu_log("add_menu_file(): %x: unsupported menu object type.",
			 ((menu_core *) obj_p)->m_type);
		menu_abort(1);
	}

	/*
	 *	The target is the first file obj so just update the pointer
	 */
	if (*fpp == file_p) {
		*fpp = file_p->mf_next;
	}
	/*
	 *	Find the previous file and update the pointer.
	 */
	else {
		for (; *fpp; fpp = &(*fpp)->mf_next)
			if ((*fpp)->mf_next == file_p)
				break;

		if (*fpp == NULL) {
			menu_log("free_menu_file(): cannot find file.");
			menu_log("\tfile name is %s.", file_p->mf_name);
			menu_log("\tobject name is %s.", name_p);
			menu_abort(1);
		}

		(*fpp)->mf_next = file_p->mf_next;
	}

	bzero((char *) file_p, sizeof(*file_p));
	free((char *) file_p);
} /* end free_menu_file() */
