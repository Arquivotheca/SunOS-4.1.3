#ifndef lint
static	char	sccsid[] = "@(#)get_noecho.c 1.1 92/07/30";
#endif

/*
 *	Name:		get_noecho.c
 *
 *	Description:	Get a form noecho.  The following values are returned:
 *
 *			 1		- noecho is valid
 *			 0		- an error occurred
 *			-1		- a fatal error occurred
 *			MENU_IGNORE_OBJ	- noecho is not active
 *			MENU_SKIP_IO	- skip object I/O, but no post function
 *
 *	Call syntax:	ret_code = get_form_noecho(noecho_p);
 *
 *	Parameters:	form_noecho *	noecho_p;
 *
 *	Return value:	int		ret_code;
 */

#include <ctype.h>
#include <curses.h>
#include "menu.h"
#include "menu_impl.h"


int
get_form_noecho(noecho_p)
	form_noecho *	noecho_p;
{
	char		ch;			/* current character */
	char *		cp;			/* ptr into data buffer */
	int		done = 0;		/* are we done yet? */
	int		ret_code;		/* return code */


	if (noecho_p->fne_active == 0)		/* not an active noecho */
		return(MENU_IGNORE_OBJ);

	if (noecho_p->fne_prefunc) {		/* invoke the pre-function */
		ret_code = (*noecho_p->fne_prefunc)(noecho_p->fne_prearg,
						    noecho_p->fne_data);

		/*
		 *	Skip I/O for this object and just do the post
		 *	function.  If there is no post function, then
		 *	return MENU_SKIP_IO.
		 */
		if (ret_code == MENU_SKIP_IO && noecho_p->fne_postfunc)
			done = 1;

		else if (ret_code != 1)
			return(ret_code);
	}

	if (!done) {
		cp = noecho_p->fne_data;
		move((int) noecho_p->fne_y, (int) noecho_p->fne_x);
	}

	while (!done) {
		refresh();

		switch (ch = read_menu_char()) {
		case C_BACKWARD:
		case C_FORWARD:
		case '\n':
		case '\r':
			*cp = '\0';
			done = 1;
			break;

		case CERASE:
			/*
			 *	Nothing to erase
			 */
			if (cp == noecho_p->fne_data)
				break;

			*(--cp) = '\0';
			break;

		case CKILL:
			cp = noecho_p->fne_data;
			break;

		default:
			/*
			 *	No more room
			 */
			if (
		         cp >= &noecho_p->fne_data[noecho_p->fne_datasize - 1])
				continue;
			*(cp++) = ch;
			*(cp) = NULL;
			break;
		} /* end switch */
	} /* end while not done */

	if (noecho_p->fne_postfunc)		/* invoke the post-function */
		return((*noecho_p->fne_postfunc)(noecho_p->fne_postarg,
					         noecho_p->fne_data));

	return(1);
} /* end get_form_noecho() */
