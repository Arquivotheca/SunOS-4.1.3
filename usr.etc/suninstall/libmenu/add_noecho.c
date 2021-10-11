#ifndef lint
static	char	sccsid[] = "@(#)add_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_noecho.c
 *
 *	Description:	Add a noecho to a form.  Allocates a form noecho
 *		structure using menu_alloc() and initializes the noechos.
 *		Returns a pointer to the form noecho.
 *
 *	Call syntax:	noecho_p = add_form_noecho(form_p, help, active,
 *						     name, y_coord, x_coord,
 *						     data_p, data_size,
 *						     pre_func, pre_arg,
 *						     post_func, post_arg);
 *
 *	Parameters:	form *		form_p;
 *			int (*		help)();
 *			char		active;
 *			char *		name;
 *			menu_coord	y_coord;
 *			menu_coord	x_coord;
 *			char *		data_p;
 *			int		data_size;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		post_func)();
 *			pointer		post_arg;
 *
 *	Return value:	form_noecho *	noecho_p;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


form_noecho *
add_form_noecho(form_p, help, active, name, y_coord, x_coord,
	       data_p, data_size, pre_func, pre_arg, post_func, post_arg)
	form *		form_p;
	int (*		help)();
	char		active;
	char *		name;
	menu_coord	y_coord;
	menu_coord	x_coord;
	char *		data_p;
	int		data_size;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		post_func)();
	pointer		post_arg;
{
	form_noecho **	noecho_pp;		/* ptr to new noecho ptr */
	form_map *	last_p = NULL;		/* pointer to last map */
	form_map **	map_pp;			/* ptr to new map ptr */


	for (noecho_pp = &form_p->f_noechos; *noecho_pp;)
		noecho_pp = &(*noecho_pp)->fne_next;

	*noecho_pp = (form_noecho *) menu_alloc(sizeof(**noecho_pp));

	for (map_pp = &form_p->f_map; *map_pp;) {
		last_p = *map_pp;
		map_pp = &(*map_pp)->fm_next;
	}

	*map_pp = (form_map *) menu_alloc(sizeof(**map_pp));

	(*map_pp)->fm_obj = (pointer) *noecho_pp;
	(*map_pp)->fm_func = get_form_noecho;
	(*map_pp)->fm_prev = last_p;

	(*noecho_pp)->m_type = MENU_OBJ_NOECHO;
	(*noecho_pp)->m_help = help;
	(*noecho_pp)->fne_active = active;
	(*noecho_pp)->fne_name = name;
	(*noecho_pp)->fne_x = x_coord;
	(*noecho_pp)->fne_y = y_coord;
	(*noecho_pp)->fne_data = data_p;
	(*noecho_pp)->fne_datasize = data_size;
	(*noecho_pp)->fne_prefunc = pre_func;
	(*noecho_pp)->fne_prearg = pre_arg;
	(*noecho_pp)->fne_postfunc = post_func;
	(*noecho_pp)->fne_postarg = post_arg;

	return(*noecho_pp);
} /* end add_form_noecho() */
