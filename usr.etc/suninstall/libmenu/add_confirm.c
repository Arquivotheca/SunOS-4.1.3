#ifndef lint
static	char	sccsid[] = "@(#)add_confirm.c 1.1 92/07/30";
#endif

/*
 *	Name:		add_confirm.c
 *
 *	Description:	Add functions for a confirmation object.
 *
 *	Call syntax:	add_confirm_obj(form_p, name, answer_p,
 *					pre_func, pre_arg, no_func, no_arg,
 *					yes_func, yes_arg, confirm_mesg);
 *
 *	Parameters:	form *		form_p;
 *			char *		name;
 *			char *		answer_p;
 *			int (*		pre_func)();
 *			pointer		pre_arg;
 *			int (*		no_func)();
 *			pointer		no_arg)();
 *			int (*		yes_func)();
 *			pointer		yes_arg;
 *			char *		confirm_mesg;
 */

#include <curses.h>
#include "menu.h"


/*
 *	Local types:
 */

/*
 *	Definition of a pointer conversion union.  This union is defined to
 *	keep lint from complaining about converting function pointers.
 */
typedef union ptr_conv_t {
	form *		form_p;
	int		(*pfi_p)();
	pointer		pointer_p;
	form_yesno *	yesno_p;
} ptr_conv;




form_yesno *
add_confirm_obj(form_p, active, name, answer_p, pre_func, pre_arg,
		 no_func, no_arg, yes_func, yes_arg, confirm_mesg)
	form *		form_p;
	int		active;
	char *		name;
	char *		answer_p;
	int (*		pre_func)();
	pointer		pre_arg;
	int (*		no_func)();
	pointer		no_arg;
	int (*		yes_func)();
	pointer		yes_arg;
	char *		confirm_mesg;
{
	ptr_conv *	ptrs;			/* ptr to pointers */
	form_yesno *	yesno_p;		/* ptr to yes/no question */


	ptrs = (ptr_conv *) menu_alloc(sizeof(ptr_conv) * 4);

	ptrs[0].form_p =    form_p;
	ptrs[2].pfi_p =     pre_func;
	ptrs[3].pointer_p = pre_arg;

	yesno_p = add_form_yesno(form_p, PFI_NULL, active, name,
				 (menu_coord) LINES - 2,
				 (menu_coord) strlen(confirm_mesg) + 2,
				 answer_p,
				 menu_confirm_func, (pointer) ptrs,
				 no_func, no_arg,
				 yes_func, yes_arg);
	ptrs[1].yesno_p = yesno_p;
	form_p->f_shared = yesno_p;

	(void) add_menu_string((pointer) yesno_p, active,
			       (menu_coord) LINES - 2, 1, ATTR_NORMAL,
			       confirm_mesg);

	return(yesno_p);
} /* end add_confirm_obj() */




int
menu_confirm_func(ptrs)
	ptr_conv 	ptrs[];
{
	pointer		arg_p;
	form *		form_p;
	int		(*func_p)();
	int		ret_code;
	form_yesno *	yesno_p;


	form_p =  ptrs[0].form_p;
	yesno_p = ptrs[1].yesno_p;
	func_p =  ptrs[2].pfi_p;
	arg_p =   ptrs[3].pointer_p;

	if (func_p) {
		ret_code = (*func_p)(arg_p);
		if (ret_code != 1)
			return(ret_code);
	}

	if (form_p->f_shared)
		clear_form_yesno(form_p->f_shared);

	if (yesno_p->fyn_answerp == NULL)
		yesno_p->fyn_answer = ' ';
	else
		yesno_p->fyn_answer = *yesno_p->fyn_answerp;

	display_form_yesno(yesno_p);

	form_p->f_shared = yesno_p;

	return(1);
} /* end menu_confirm_func() */
