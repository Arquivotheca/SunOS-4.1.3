#ifndef lint
static	char	sccsid[] = "@(#)use_menu.c 1.1 92/07/30";
#endif

/*
 *	Name:		use_menu.c
 *
 *	Description:	Routines for controlling a menu.
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"



/*
 *	Name:		set_menu_item()
 *
 *	Description:	Set the menu pointer to a specific item.  If the
 *		specific item cannot be found, then the menu system aborts.
 *		If item_p is NULL, then the menu pointer is set to NULL
 *		which causes the finish object to become active.
 *
 *	Call syntax:	set_menu_item(menu_p, item_p);
 *
 *	Parameters:	menu *		menu_p;
 *			menu_item *	item_p;
 */

void
set_menu_item(menu_p, item_p)
	menu *		menu_p;
	menu_item *	item_p;
{
	if (item_p == NULL) {
		menu_p->m_ptr = NULL;
		return;
	}

	for (menu_p->m_ptr = menu_p->m_items; menu_p->m_ptr;
	     menu_p->m_ptr = menu_p->m_ptr->mi_next)
		if (menu_p->m_ptr == item_p)
			return;

	menu_log("%s: cannot find object in menu's map.", menu_p->m_name);
	menu_abort(1);
} /* end set_menu_item() */




/*
 *	Name:		use_menu()
 *
 *	Description:	Main control routine for a menu.  The following
 *		values are returned:
 *
 *			 1 - menu is valid
 *			 0 - menu is invalid
 *			-1 - an error occurred
 *
 *	Call syntax:	ret_code = use_menu(menu_p);
 *
 *	Parameters:	menu *		menu_p;
 *
 *	Return value:	int		ret_code;
 */

int
use_menu(menu_p)
	menu *		menu_p;
{
	menu_item *	last_p;			/* ptr to last item */
	int		ret_code;		/* return code from function */
	pointer		saved_fm;		/* ptr to saved form/menu */


	saved_fm = _current_fm;
	_current_fm = (pointer) menu_p;

	/*
	 *	Run all the check functions before the menu is painted.
	 */
	for (last_p = menu_p->m_items; last_p; last_p = last_p->mi_next) {
		if (last_p->mi_chkfunc)
			last_p->mi_chkvalue =
				      (*last_p->mi_chkfunc)(last_p->mi_chkarg);
	}

	/*
	 *	Clear the screen and display the menu
	 */
	redisplay(MENU_CLEAR);

	/*
	 *	Start at first item if an item has not been specified.
	 *	Find the last item.
	 */
	if (menu_p->m_ptr == NULL)
		menu_p->m_ptr = menu_p->m_items;
	for (last_p = menu_p->m_items; last_p->mi_next;
	     last_p = last_p->mi_next)
		/* NULL statement */ ;

	while (1) {
		_current_obj = (pointer) menu_p->m_ptr;

		/*
		 *	Only explicitly handle return codes for fatal errors,
		 *	non-fatal errors, and MENU_REPEAT_OBJ
		 */
		if (menu_p->m_ptr)
			ret_code = get_menu_item(menu_p->m_ptr);
		else {
			menu_p->m_finish->fyn_answer = ' ';
			ret_code = get_form_yesno(menu_p->m_finish);
		}

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
		 *	Move to the previous item.
		 */
		if (_menu_backward) {
			if (menu_p->m_ptr == NULL)
				menu_p->m_ptr = last_p;

			/*
			 *	At the beginning of the item list.  If there
			 *	is a finish object, then goto it.  Otherwise
			 *	wrap around.
			 */
			else if (menu_p->m_ptr == menu_p->m_items) {
				if (menu_p->m_finish)
					menu_p->m_ptr = NULL;
				else
					menu_p->m_ptr = last_p;
			}
			else
				menu_p->m_ptr = menu_p->m_ptr->mi_prev;
		}
		/*
		 *	Move to next item
		 */
		else if (_menu_forward) {
			if (menu_p->m_ptr == NULL)
				menu_p->m_ptr = menu_p->m_items;

			/*
			 *	At the end of the item list.  If there is
			 *	a finish object, then goto it.  Otherwise
			 *	wrap around.
			 */
			else if (menu_p->m_ptr == last_p) {
				if (menu_p->m_finish)
					menu_p->m_ptr = NULL;
				else
					menu_p->m_ptr = menu_p->m_items;
			}
			else
				menu_p->m_ptr = menu_p->m_ptr->mi_next;
		}
		/*
		 *	Item was gotten
		 */
		else {
			if (menu_p->m_ptr == NULL || ret_code == MENU_EXIT_MENU) {
				/*
				 *	If the finish object exists and
				 *	the answer is yes, or if the return
				 *	value was to exit the menu, then return to
				 *	the caller.
				 */
				if ((menu_p->m_finish &&
				    menu_p->m_finish->fyn_answer == 'y') ||
				    ret_code == MENU_EXIT_MENU) {
					clear_menu(menu_p);
					_current_fm = saved_fm;
					redisplay(MENU_CLEAR);
					return(1);
				}

				menu_p->m_ptr = menu_p->m_items;
			}
			/*
			 *	At the end of the item list.  If there is
			 *	a finish object, then goto it.  Otherwise
			 *	wrap around.
			 */
			else if (menu_p->m_ptr == last_p) {
				if (menu_p->m_finish)
					menu_p->m_ptr = NULL;
				else
					menu_p->m_ptr = menu_p->m_items;
			}
			else
				menu_p->m_ptr = menu_p->m_ptr->mi_next;
		}
	}
} /* end use_menu() */
