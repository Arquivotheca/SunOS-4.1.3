#ifndef lint
static	char	sccsid[] = "@(#)add_radio.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_radio.c
 *
 *	Description:	Add a radio to a form.  Allocates a form radio
 *		structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the form radio.
 *
 *	Call syntax:	radio_p = add_form_radio(form_p, help, active, name,
 *						 code_ptr,
 *						 pre_func, pre_arg,
 *						 post_func, post_arg);
 *
 *	Parameters:	form *		form_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			int *		code_ptr;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		post_func)();
 *			pointer		post_arg;
 *
 *	Return value:	form_radio *	radio_p;
 */

#include "menu.h"
#include "menu_impl.h"


form_radio *
add_form_radio(form_p, help, active, name, code_ptr,
	       pre_func, pre_arg, post_func, post_arg)
	form *		form_p;
	int (*		help)();
	char		active;
	char *		name;
	int *		code_ptr;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		post_func)();
	pointer		post_arg;
{
	form_map *	last_p = NULL;		/* pointer to last map */
	form_map **	map_pp;			/* ptr to new map pointer */
	form_radio **	radio_pp;		/* ptr to new radio pointer */


	for (radio_pp = &form_p->f_radios; *radio_pp;)
		radio_pp = &(*radio_pp)->fr_next;

	*radio_pp = (form_radio *) menu_alloc(sizeof(**radio_pp));

	for (map_pp = &form_p->f_map; *map_pp;) {
		last_p = *map_pp;
		map_pp = &(*map_pp)->fm_next;
	}

	*map_pp = (form_map *) menu_alloc(sizeof(**map_pp));

	(*map_pp)->fm_obj = (pointer) *radio_pp;
	(*map_pp)->fm_func = get_form_radio;
	(*map_pp)->fm_prev = last_p;

	(*radio_pp)->m_type = MENU_OBJ_RADIO;
	(*radio_pp)->m_help = help;
	(*radio_pp)->fr_active = active;
	(*radio_pp)->fr_name = name;
	(*radio_pp)->fr_codeptr = code_ptr;
	(*radio_pp)->fr_prefunc = pre_func;
	(*radio_pp)->fr_prearg = pre_arg;
	(*radio_pp)->fr_postfunc = post_func;
	(*radio_pp)->fr_postarg = post_arg;

	return(*radio_pp);
} /* end add_form_radio() */
