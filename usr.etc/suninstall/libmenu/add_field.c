#ifndef lint
static	char	sccsid[] = "@(#)add_field.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_field.c
 *
 *	Description:	Add a field to a form.  Allocates a form field
 *		structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the form field.
 *
 *	Call syntax:	field_p = add_form_field(form_p, help, active, name,
 *						 y_coord, x_coord,
 *						 width, data_p, data_size, lex,
 *						 pre_func, pre_arg, post_func,
 *						 post_arg);
 *
 *	Parameters:	form *		form_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			menu_coord	width;
 *			char *		data_p;
 *			int		data_size;
 *			int (*		lex)();
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		post_func)();
 *			pointer		post_arg;
 *
 *	Return value:	form_field *	field_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


form_field *
add_form_field(form_p, help, active, name, y_coord, x_coord, width,
	       data_p, data_size, lex, pre_func, pre_arg, post_func, post_arg)
	form *		form_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	y_coord;
	menu_coord	x_coord;
	menu_coord	width;
	char *		data_p;
	int		data_size;
	int (*		lex)();
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		post_func)();
	pointer		post_arg;
{
	form_field **	field_pp;		/* ptr to new field ptr */
	form_map *	last_p = NULL;		/* pointer to last map */
	form_map **	map_pp;			/* ptr to new map ptr */


	for (field_pp = &form_p->f_fields; *field_pp;)
		field_pp = &(*field_pp)->ff_next;

	*field_pp = (form_field *) menu_alloc(sizeof(**field_pp));

	for (map_pp = &form_p->f_map; *map_pp;) {
		last_p = *map_pp;
		map_pp = &(*map_pp)->fm_next;
	}

	*map_pp = (form_map *) menu_alloc(sizeof(**map_pp));

	(*map_pp)->fm_obj = (pointer) *field_pp;
	(*map_pp)->fm_func = get_form_field;
	(*map_pp)->fm_prev = last_p;

	(*field_pp)->m_type = MENU_OBJ_FIELD;
	(*field_pp)->m_help = help;
	(*field_pp)->ff_active = active;
	(*field_pp)->ff_name = name;
	(*field_pp)->ff_x = x_coord;
	(*field_pp)->ff_y = y_coord;
	if (width == 0)
		(*field_pp)->ff_width = COLS - x_coord - 1;
	else
		(*field_pp)->ff_width = width;
	(*field_pp)->ff_data = data_p;
	(*field_pp)->ff_datasize = data_size;
	(*field_pp)->ff_lex = lex;
	(*field_pp)->ff_prefunc = pre_func;
	(*field_pp)->ff_prearg = pre_arg;
	(*field_pp)->ff_postfunc = post_func;
	(*field_pp)->ff_postarg = post_arg;

	return(*field_pp);
} /* end add_form_field() */
