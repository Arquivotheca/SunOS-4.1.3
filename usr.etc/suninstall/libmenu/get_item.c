#ifndef lint
#ident			"@(#)get_item.c 1.1 92/07/30 SMI";
#endif

/*
 *	Name:		get_item.c
 *
 *	Description:	Get a menu item.  The following values are returned:
 *
 *			 1		- item is valid
 *			 0		- an error occurred
 *			-1		- a fatal error occurred
 *			MENU_IGNORE_OBJ	- item is not active
 *			MENU_SKIP_IO	- skip object I/O, and do function
 *			MENU_EXIT_MENU	- exit the current menu
 *
 *	Call syntax:	ret_code = get_menu_item(item_p);
 *
 *	Parameters:	menu_item *	item_p;
 *
 *	Return value:	int		ret_code;
 */

#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


int
get_menu_item(item_p)
	menu_item *	item_p;
{
	int		done = 0;		/* are we done yet? */
	int		pressed = 0;		/* was a key pressed? */
	int		ret_code;		/* return code */


	if (item_p->mi_active == NOT_ACTIVE)	/* not an active item */
		return(MENU_IGNORE_OBJ);


	/*
	 *	Invoke the menu item pre-function
	 */
	if (item_p->mi_prefunc) {
		ret_code = (*item_p->mi_prefunc)(item_p->mi_prearg);

		if (ret_code != 1)
			return(ret_code);
	}

	if (item_p->mi_prefunc) {		/* invoke the pre-function */
		ret_code = (*item_p->mi_prefunc)(item_p->mi_prearg);

		/*
		 *	Skip I/O for this object and just do the function.
		 *	If there is no function, then return MENU_SKIP_IO.
		 */
		if (ret_code == MENU_SKIP_IO && item_p->mi_func)
			done = 1;

		else if (ret_code != 1)
			return(ret_code);
	}

	while (!done) {
		move((int) item_p->mi_y, (int) item_p->mi_x);
		refresh();

		switch (read_menu_char()) {
		case 'x':
		case 'X':
			/*
			 *	Clear the previous item select marker
			 */
			if (item_p->mi_menu->m_selected)
				mvaddch(
				       (int) item_p->mi_menu->m_selected->mi_y,
				       (int) item_p->mi_menu->m_selected->mi_x,
				       ' ');
			/*
			 *	Mark the current item
			 */
			mvaddch((int) item_p->mi_y, (int) item_p->mi_x, 'x');

			/*
			 *	Clear the check function status.
			 */
			display_item_status(item_p);

			item_p->mi_menu->m_selected = item_p;
			pressed = 1;
			/*
			 *	Fall through here
			 */

		case C_BACKWARD:
		case C_FORWARD:
		case ' ':
		case '\r':
		case '\n':
			done = 1;
			break;
		} /* end switch() */
	} /* end while not done */

	/*
	 *	Invoke the menu item function
	 */
	if (pressed && item_p->mi_func) {
		ret_code = (*item_p->mi_func)(item_p->mi_arg);

		switch (ret_code) {
		case 1:
			mvaddch((int) item_p->mi_y, (int) item_p->mi_x, ' ');
			item_p->mi_menu->m_selected = NULL;
			break;

		default:
			mvaddch((int) item_p->mi_y, (int) item_p->mi_x, ' ');
			item_p->mi_menu->m_selected = NULL;

			/*
			 *	Fall through here
			 */
		case MENU_GOTO_OBJ:
		case MENU_EXIT_MENU:
			return(ret_code);
		}
	}

	/*
	 *	Invoke the menu item check function and update the
	 *	check function status.
	 */
	if (pressed && item_p->mi_chkfunc) {
		ret_code = (*item_p->mi_chkfunc)(item_p->mi_chkarg);
		/*
		 * We don't have any way of knowing if a data file has
		 * been removed so we just assume that if we have
		 * marked the data file present it never disappears
		 */
		if (item_p->mi_chkvalue != 1)
			item_p->mi_chkvalue = ret_code;
		if (item_p->mi_chkvalue != 1) {
			mvaddch((int)item_p->mi_y,(int)item_p->mi_x, ' ');
			item_p->mi_menu->m_selected = NULL;
		}

		display_item_status(item_p);
	}

	return(1);
} /* end get_menu_item() */
