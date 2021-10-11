#ifndef lint
static	char	sccsid[] = "@(#)add_file.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_file.c
 *
 *	Description:	Add a menu file to a menu or a form.  Allocates a
 *		menu file structure using menu_alloc() and initializes the
 *		fields.  Returns a pointer to the menu file, if successful.
 *
 *	Call syntax:	file_p = add_menu_file(obj_p, help, active, name,
 *					       begin_y, begin_x, end_y, end_x,
 *					       gap, path);
 *
 *	Parameters:	pointer		obj_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	begin_y;
 *			menu_coord	begin_x;
 *			menu_coord	end_y;
 *			menu_coord	end_x;
 *			menu_coord	gap;
 *			char *		path;
 *
 *	Return value:	menu_file *	file_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


menu_file *
add_menu_file(obj_p, help, active, name,
	      begin_y, begin_x, end_y, end_x, gap, path)
	pointer		obj_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	begin_y;
	menu_coord	begin_x;
	menu_coord	end_y;
	menu_coord	end_x;
	menu_coord	gap;
	char *		path;
{
	menu_file **	file_pp;		/* ptr to new file pointer */


	switch(((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_FORM:
		file_pp = &((form *) obj_p)->f_files;
		break;

	case MENU_OBJ_MENU:
		file_pp = &((menu *) obj_p)->m_files;
		break;

	default:
		menu_log("add_menu_file(): %x: unsupported menu object type.",
			 ((menu_core *) obj_p)->m_type);
		menu_abort(1);
	}

	while (*file_pp)
		file_pp = &(*file_pp)->mf_next;

	*file_pp = (menu_file *) menu_alloc(sizeof(**file_pp));

	(*file_pp)->m_type = MENU_OBJ_FILE;
	(*file_pp)->m_help = help;
	(*file_pp)->mf_active = active;
	(*file_pp)->mf_name = name;
	(*file_pp)->mf_begin_y = begin_y;
	(*file_pp)->mf_begin_x = begin_x;
	(*file_pp)->mf_end_y = end_y;
	(*file_pp)->mf_end_x = end_x;
	(*file_pp)->mf_gap = gap;
	(*file_pp)->mf_path = path;

	return(*file_pp);
} /* end add_menu_file() */
