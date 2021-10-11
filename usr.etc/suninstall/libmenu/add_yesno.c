#ifndef lint
static	char	sccsid[] = "@(#)add_yesno.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_yesno.c
 *
 *	Description:	Add a yes/no question to a form.  Allocates a yes/no
 *		structure using menu_alloc() and initializes the fields.
 *		Returns a pointer to the yes/no structure.
 *
 *	Call syntax:	yesno_p = add_form_yesno(form_p, help, active, name,
 *						 y_coord, x_coord,
 *						 answer_p,
 *						 pre_func, pre_arg,
 *						 no_func, no_arg,
 *						 yes_func, yes_arg);
 *
 *	Parameters:	form *		form_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			char *		answer_p;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		no_func)();
 *			pointer		no_arg;
 *			int (*		yes_func)();
 *			pointer		yes_arg;
 *
 *	Return value:	form_yesno *	yesno_p;
 */

#include "menu.h"
#include "menu_impl.h"


form_yesno *
add_form_yesno(form_p, help, active, name, y_coord, x_coord, answer_p,
	       pre_func, pre_arg, no_func, no_arg, yes_func, yes_arg)
	form *		form_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	y_coord;
	menu_coord	x_coord;
	char *		answer_p;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		no_func)();
	pointer		no_arg;
	int (*		yes_func)();
	pointer		yes_arg;
{
	form_map *	last_p = NULL;		/* pointer to last map */
	form_map **	map_pp;			/* ptr to new map pointer */
	form_yesno **	yesno_pp;		/* ptr to new yes/no pointer */


	for (yesno_pp = &form_p->f_yesnos; *yesno_pp;)
		yesno_pp = &(*yesno_pp)->fyn_next;

	*yesno_pp = (form_yesno *) menu_alloc(sizeof(**yesno_pp));

	for (map_pp = &form_p->f_map; *map_pp; ) {
		last_p = *map_pp;
		map_pp = &(*map_pp)->fm_next;
	}

	*map_pp = (form_map *) menu_alloc(sizeof(**map_pp));

	(*map_pp)->fm_obj = (pointer) *yesno_pp;
	(*map_pp)->fm_func = get_form_yesno;
	(*map_pp)->fm_prev = last_p;

	(*yesno_pp)->m_type = MENU_OBJ_YESNO;
	(*yesno_pp)->m_help = help;
	(*yesno_pp)->fyn_active = active;
	(*yesno_pp)->fyn_name = name;
	(*yesno_pp)->fyn_x = x_coord;
	(*yesno_pp)->fyn_y = y_coord;
	(*yesno_pp)->fyn_answerp = answer_p;
	(*yesno_pp)->fyn_prefunc = pre_func;
	(*yesno_pp)->fyn_prearg = pre_arg;
	(*yesno_pp)->fyn_nofunc = no_func;
	(*yesno_pp)->fyn_noarg = no_arg;
	(*yesno_pp)->fyn_yesfunc = yes_func;
	(*yesno_pp)->fyn_yesarg = yes_arg;

	return(*yesno_pp);
} /* end add_form_yesno() */
