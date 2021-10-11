#ifndef lint
#ident		"@(#)M% 1.1 92/07/30";
#endif

/*
 *	Name:		use_form.c
 *
 *	Description:	Routines for controlling a form.
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"

static void	high_light_obj();

/*
 *	Name:		set_form_map()
 *
 *	Description:	Set the form map pointer to a specific object.  If the
 *		specific object cannot be found, then then menu system aborts.
 *
 *	Call syntax:	set_form_map(form_p, obj_p);
 *
 *	Parameters:	form *		form_p;
 *			pointer		obj_p;
 */

void
set_form_map(form_p, obj_p)
	form *		form_p;
	pointer		obj_p;
{
	for (form_p->f_obj = form_p->f_map; form_p->f_obj;
	     form_p->f_obj = form_p->f_obj->fm_next)
		if (form_p->f_obj->fm_obj == obj_p) {

/*			_current_obj = obj_p;
*/
			return;
		}

	menu_log("%s: cannot find object in form's map.", form_p->f_name);
	menu_abort(1);
} /* end set_form_map() */




/*
 *	Name:		use_form()
 *
 *	Description:	Main control routine for a form.  The following
 *		values are returned:
 *
 *			 1 - form is valid
 *			 0 - form is invalid
 *			-1 - an error occurred
 *
 *	Call syntax:	ret_code = use_form(form_p);
 *
 *	Parameters:	form *		form_p;
 *
 *	Return value:	int		ret_code;
 */


int
use_form(form_p)
	form *		form_p;
{
	form_map *	last_p;			/* ptr to last map */
	int		ret_code;		/* return code from function */
	pointer		saved_fm;		/* saved form/menu pointer */
	pointer		previous_obj;	

	saved_fm = _current_fm;
	_current_fm = (pointer) form_p;

	/*
	 *	Clear the screen and display the form
	 */
	redisplay(MENU_CLEAR);

	/*
	 *	Start at first object if an object has not been specified.
	 *	Find the last object.
	 */
	if (form_p->f_obj == NULL)
		form_p->f_obj = form_p->f_map;
	for (last_p = form_p->f_obj; last_p->fm_next; last_p = last_p->fm_next)
		/* NULL statement */ ;

	_current_obj = form_p->f_obj->fm_obj;

	while (1) {

		previous_obj = _current_obj;

		_current_obj = form_p->f_obj->fm_obj;
    
		high_light_obj(previous_obj);

		high_light_obj(_current_obj);

		/*
		 *	Get the object.  Only explicitly handle return codes
		 *	for fatal errors, non-fatal errors, and MENU_REPEAT_OBJ
		 */
		ret_code = (*form_p->f_obj->fm_func)(form_p->f_obj->fm_obj);

		/*
		 *	Explicitly repeat the object
		 */
		if (ret_code == MENU_GOTO_OBJ || ret_code == MENU_REPEAT_OBJ)
			continue;

		if (ret_code == 0)		/* some other error occured */
			continue;

		if (ret_code == -1)		/* some fatal error occurred */
			return(-1);

		/*
		 *	Move to the previous object.
		 */
		if (_menu_backward) {
			if (form_p->f_obj == form_p->f_map)
				form_p->f_obj = last_p;
			else
				form_p->f_obj = form_p->f_obj->fm_prev;
		}
		/*
		 *	Move to next object
		 */
		else if (_menu_forward) {
			if (form_p->f_obj == last_p)
				form_p->f_obj = form_p->f_map;
			else
				form_p->f_obj = form_p->f_obj->fm_next;
		}
		/*
		 *	Object was gotten
		 */
		else {
			/*
			 *	If this is the finish object and the answer
			 *	is yes, then return to the caller.
			 */
			if (form_p->f_obj->fm_obj == (pointer) form_p->f_finish
		&& ((form_yesno *) form_p->f_obj->fm_obj)->fyn_answer == 'y') {
				_current_fm = saved_fm;
				clear_form(form_p);
				redisplay(MENU_CLEAR);
				return(1);
			}

			if (form_p->f_obj == last_p)
				form_p->f_obj = form_p->f_map;
			else
				form_p->f_obj = form_p->f_obj->fm_next;
		}
	}
} /* end use_form() */


static void	
high_light_obj(obj_p)
	menu_core *	obj_p;
{
	switch(obj_p->m_type)	{
	case MENU_OBJ_BUTTON:
		display_form_button(obj_p);
		break;

	case MENU_OBJ_FIELD:
		display_form_field(obj_p);
		break;

	case MENU_OBJ_FILE:
		break;

	case MENU_OBJ_FORM:
		break;
		
	case MENU_OBJ_ITEM:
		break;

	case MENU_OBJ_MENU:
		break;

	case MENU_OBJ_NOECHO:
		display_form_noecho(obj_p);
		break;

	case MENU_OBJ_RADIO:
		display_form_radio(obj_p);
		break;
		
	case MENU_OBJ_YESNO:
		display_form_yesno(obj_p);
		break;
	}

}
