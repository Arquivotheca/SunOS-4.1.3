#ifndef lint
static	char	sccsid[] = "@(#)create_form.c 1.1 92/07/30";
#endif

/*
 *	Name:		create_form.c
 *
 *	Description:	Create a form structure.  Allocates a form structure
 *		using menu_alloc() and initializes the fields.  The name of the
 *		form is not copied, and the data is expected to be static.
 *		Returns a pointer to the form.
 *
 *	Call syntax:	form_p = create_form(help, active, name);
 *
 *	Parameters:	int (*		help)();
 *			int		active;
 *			char *		name;
 *
 *	Return value:	form *		form_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


form *
create_form(help, active, name)
	int (*		help)();
	int		active;
	char *		name;
{
	form *		form_p;			/* pointer to form to return */


	form_p = (form *) menu_alloc(sizeof(*form_p));
	form_p->m_type = MENU_OBJ_FORM;
	form_p->m_help = help;
	form_p->f_active = active;
	form_p->f_name = name;

	(void) add_menu_string((pointer) form_p, active, 0, 1,
			       ATTR_NORMAL, name);
	(void) add_menu_string((pointer) form_p, active, 0, 26, ATTR_STAND,
			       "[?=help]");
	(void) add_menu_string((pointer) form_p, active, 0, 35, ATTR_STAND,
			       "[DEL=erase one char]");
	(void) add_menu_string((pointer) form_p, active, 0, 56, ATTR_STAND,
			       "[RET=end of input data]");
	(void) add_menu_string((pointer) form_p, active, 1, 1,
			       ATTR_NORMAL,
"-----------------------------------------------------------------------------");
	(void) add_menu_string((pointer) form_p, active,
			       (menu_coord) LINES - 1, 4, ATTR_STAND,
			       "[x/X=select choice]");
	(void) add_menu_string((pointer) form_p, active,
			       (menu_coord) LINES - 1, 24, ATTR_STAND,
			       "[space=next choice]");
	(void) add_menu_string((pointer) form_p, active,
			       (menu_coord) LINES - 1, 44, ATTR_STAND,
			       "[^B/^P=backward]");
	(void) add_menu_string((pointer) form_p, active,
			       (menu_coord) LINES - 1, 61, ATTR_STAND,
			       "[^F/^N=forward]");

	return(form_p);
} /* end create_form() */
