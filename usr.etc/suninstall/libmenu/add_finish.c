#ifndef lint
static	char	sccsid[] = "@(#)add_finish.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_finish.c
 *
 *	Description:	Add functions for the finish object.  The finish
 *		object is treated special by use_form() and use_menu().
 *
 *	Call syntax:	add_finish_obj(obj_p, pre_func, pre_arg,
 *					 no_func, no_arg, yes_func, yes_arg);
 *
 *	Parameters:	form *		obj_p;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		no_func)();
 *			pointer		no_arg)();
 *			int (*		yes_func)();
 *			pointer		yes_arg;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


static	char		FORM_MESG[] = "Are you finished with this form [y/n] ?";
static	char		MENU_MESG[] = "Are you finished with this menu [y/n] ?";


form_yesno *
add_finish_obj(obj_p, pre_func, pre_arg, no_func, no_arg, yes_func, yes_arg)
	pointer		obj_p;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		no_func)();
	pointer		no_arg;
	int (*		yes_func)();
	pointer		yes_arg;
{
	form_yesno *	yesno_p;		/* ptr to yes/no object */


	switch (((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_FORM:
		yesno_p = add_confirm_obj((form *) obj_p, ACTIVE,
					  CP_NULL, CP_NULL,
					  pre_func, pre_arg,
					  no_func, no_arg, yes_func, yes_arg,
					  FORM_MESG);

		((form *) obj_p)->f_finish = yesno_p;
		((form *) obj_p)->f_shared = yesno_p;
		break;

	case MENU_OBJ_MENU:
		/*
		 *	The yes/no object is hand crafted here instead of
		 *	calling add_confirm_obj() and add_form_yesno()
		 *	to prevent special casing both add_confirm_obj()
		 *	and add_form_yesno().
		 */
		yesno_p = (form_yesno *) menu_alloc(sizeof(*yesno_p));
		yesno_p->m_type = MENU_OBJ_YESNO;
		yesno_p->fyn_active = ACTIVE;
		yesno_p->fyn_y = LINES - 2;
		yesno_p->fyn_x = strlen(MENU_MESG) + 2;
		yesno_p->fyn_prefunc = pre_func;
		yesno_p->fyn_prearg = pre_arg;
		yesno_p->fyn_nofunc = no_func;
		yesno_p->fyn_noarg = no_arg;
		yesno_p->fyn_yesfunc = yes_func;
		yesno_p->fyn_yesarg = yes_arg;
		(void) add_menu_string((pointer) yesno_p, ACTIVE,
				       (menu_coord) LINES - 2, 1,
				       ATTR_NORMAL, MENU_MESG);

		((menu *) obj_p)->m_finish = yesno_p;
		break;
	} /* end switch */

	return(yesno_p);
} /* end add_finish_obj() */
