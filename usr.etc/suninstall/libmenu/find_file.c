#ifndef lint
static	char	sccsid[] = "@(#)find_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		find_file.c
 *
 *	Description:	Find a file object.
 *
 *	Call syntax:	fp = find_menu_file(obj_p, name);
 *
 *	Parameters:	pointer		obj_p;
 *			char *		name;
 *
 *	Return value:	menu_file *	fp;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


menu_file *
find_menu_file(obj_p, name)
	pointer		obj_p;
	char *		name;
{
	menu_file *	fp;			/* the pointer to return */


	switch(((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_FORM:
		fp = ((form *) obj_p)->f_files;
		break;

	case MENU_OBJ_MENU:
	default:
		menu_log("find_menu_file(): %x: unsupported menu object type.",
			 ((menu_core *) obj_p)->m_type);
		menu_abort(1);
	}

	for (; fp != NULL; fp = fp->mf_next)	/* check each file object */

		if (fp->mf_name && strcmp(name, fp->mf_name) == 0)
			return(fp);		/* found it */

	menu_log("%s: cannot find file object.", name);
	menu_abort(1);
	/*NOTREACHED*/
} /* end find_menu_file() */
