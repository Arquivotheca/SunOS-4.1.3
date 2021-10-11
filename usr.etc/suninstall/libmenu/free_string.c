#ifndef lint
static	char	sccsid[] = "@(#)free_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		free_string.c
 *
 *	Description:	Free a menu string.
 *
 *	Call syntax:	free_menu_string(obj_p, string_p);
 *
 *	Parameters:	pointer		obj_p;
 *			menu_string *	string_p;
 */

#include "menu.h"
#include "menu_impl.h"


void
free_menu_string(obj_p, string_p)
	pointer		obj_p;
	menu_string *	string_p;
{
	char *		name_p;			/* pointer to object name */
	menu_string **	str_pp;			/* ptr to string pointer */


	if (obj_p == NULL || string_p == NULL)
		return;

	switch (((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_BUTTON:
		name_p = ((form_button *) obj_p)->fb_name;
		str_pp = &((form_button *) obj_p)->fb_mstrings;
		break;

	case MENU_OBJ_FIELD:
		name_p = ((form_field *) obj_p)->ff_name;
		str_pp = &((form_field *) obj_p)->ff_mstrings;
		break;

	case MENU_OBJ_FILE:
		name_p = ((menu_file *) obj_p)->mf_name;
		str_pp = &((menu_file *) obj_p)->mf_mstrings;
		break;

	case MENU_OBJ_FORM:
		name_p = ((form *) obj_p)->f_name;
		str_pp = &((form *) obj_p)->f_mstrings;
		break;
		
	case MENU_OBJ_ITEM:
		name_p = ((menu_item *) obj_p)->mi_name;
		str_pp = &((menu_item *) obj_p)->mi_mstrings;
		break;

	case MENU_OBJ_MENU:
		name_p = ((menu *) obj_p)->m_name;
		str_pp = &((menu *) obj_p)->m_mstrings;
		break;

	case MENU_OBJ_NOECHO:
		name_p = ((menu *) obj_p)->m_name;
		str_pp = &((form_noecho *) obj_p)->fne_mstrings;
		break;

	case MENU_OBJ_RADIO:
		name_p = ((form_radio *) obj_p)->fr_name;
		str_pp = &((form_radio *) obj_p)->fr_mstrings;
		break;
		
	case MENU_OBJ_YESNO:
		name_p = ((form_yesno *) obj_p)->fyn_name;
		str_pp = &((form_yesno *) obj_p)->fyn_mstrings;
		break;
	}

	/*
	 *	The target is the first string so just update the pointer
	 */
	if (*str_pp == string_p) {
		*str_pp = string_p->ms_next;
	}
	/*
	 *	Find the previous string and update the pointer.
	 */
	else {
		for (; *str_pp; str_pp = &(*str_pp)->ms_next)
			if ((*str_pp)->ms_next == string_p)
				break;

		if (*str_pp == NULL) {
			menu_log("free_menu_string(): cannot find string.");
			menu_log("\tobject name is %s.", name_p);
			menu_abort(1);
		}

		(*str_pp)->ms_next = string_p->ms_next;
	}

	bzero((char *) string_p, sizeof(*string_p));
	free((char *) string_p);
} /* end free_menu_string() */
