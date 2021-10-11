#ifndef lint
static	char	sccsid[] = "@(#)add_button.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_button.c
 *
 *	Description:	Add a button to a radio.  Allocates a form button
 *		structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the form button.
 *
 *	Call syntax:	button_p = add_form_button(radio_p, help, active,
 *						   name, y_coord, x_coord,
 *						   code, func, arg);
 *
 *	Parameters:	form radio *	radio_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			int		code;
 *			int (*		func)();
 *			pointer		arg;
 *
 *	Return value:	form_button *	button_p;
 */

#include "menu.h"
#include "menu_impl.h"


form_button *
add_form_button(radio_p, help, active, name, y_coord, x_coord, code, func, arg)
	form_radio *	radio_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	y_coord;
	menu_coord	x_coord;
	int		code;
	int (*		func)();
	pointer		arg;
{
	form_button **	button_pp;		/* ptr to new button ptr */


	for (button_pp = &radio_p->fr_buttons; *button_pp;)
		button_pp = &(*button_pp)->fb_next;

	*button_pp = (form_button *) menu_alloc(sizeof(**button_pp));

	(*button_pp)->m_type = MENU_OBJ_BUTTON;
	(*button_pp)->m_help = help;
	(*button_pp)->fb_active = active;
	(*button_pp)->fb_name = name;
	(*button_pp)->fb_x = x_coord;
	(*button_pp)->fb_y = y_coord;
	(*button_pp)->fb_code = code;
	(*button_pp)->fb_func = func;
	(*button_pp)->fb_arg = arg;
	(*button_pp)->fb_radio = radio_p;

	return(*button_pp);
} /* end add_form_button() */
