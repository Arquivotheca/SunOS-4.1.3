#ifndef lint
static	char	sccsid[] = "@(#)add_string.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_string.c
 *
 *	Description:	Add a string to a menu object.  Allocates a menu
 *		string structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the menu string.
 *
 *		There are several ways to add strings to menu objects.
 *
 *		1)	A separate routine could be written for each object.
 *			This would be would allow only the needed routines to
 *			be loaded.
 *		2)	The address of the menu_string list for the object
 *			could be passed as a parameter.  This was tried in
 *			an earlier version of the library, but has been
 *			discarded as too messy.
 *		3)	A generic object pointer could be passed.  The routine
 *			could decode the object pointer and the menu string
 *			could be added to the appropriate place.
 *
 *		The final solution was chosen.  This implementation does
 *		not cause any extra routines to be loaded and it does not
 *		require numerous routines.
 *
 *	Call syntax:	string_p = add_menu_string(obj_p, active, y_coord,
 *					           x_coord, attribute, data_p);
 *
 *	Parameters:	pointer *	obj_p;
 *			char		active;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			menu_attr	attribute;
 *			char *		data_p;
 *
 *	Return value:	menu_string *	string_p;
 */

#include "menu.h"
#include "menu_impl.h"


menu_string *
add_menu_string(obj_p, active, y_coord, x_coord, attribute, data_p)
	pointer		obj_p;
	char		active;
	menu_coord	y_coord;
	menu_coord	x_coord;
	menu_attr	attribute;
	char *		data_p;
{
	menu_string **	str_pp;			/* ptr to new string pointer */


	switch (((menu_core *) obj_p)->m_type) {
	case MENU_OBJ_BUTTON:
		str_pp = &((form_button *) obj_p)->fb_mstrings;
		break;

	case MENU_OBJ_FIELD:
		str_pp = &((form_field *) obj_p)->ff_mstrings;
		break;

	case MENU_OBJ_FILE:
		str_pp = &((menu_file *) obj_p)->mf_mstrings;
		break;

	case MENU_OBJ_FORM:
		str_pp = &((form *) obj_p)->f_mstrings;
		break;
		
	case MENU_OBJ_ITEM:
		str_pp = &((menu_item *) obj_p)->mi_mstrings;
		break;

	case MENU_OBJ_MENU:
		str_pp = &((menu *) obj_p)->m_mstrings;
		break;

	case MENU_OBJ_NOECHO:
		str_pp = &((form_noecho *) obj_p)->fne_mstrings;
		break;

	case MENU_OBJ_RADIO:
		str_pp = &((form_radio *) obj_p)->fr_mstrings;
		break;
		
	case MENU_OBJ_YESNO:
		str_pp = &((form_yesno *) obj_p)->fyn_mstrings;
		break;
	}

	while (*str_pp)
		str_pp = &(*str_pp)->ms_next;

	*str_pp = (menu_string *) menu_alloc(sizeof(**str_pp));

	(*str_pp)->ms_active = active;
	(*str_pp)->ms_x = x_coord;
	(*str_pp)->ms_y = y_coord;
	(*str_pp)->ms_attribute = attribute;
	(*str_pp)->ms_data = data_p;

	return(*str_pp);
} /* end add_menu_string() */
