#ifndef lint
static	char	sccsid[] = "@(#)menu_alloc.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_alloc.c
 *
 *	Description:	Allocate memory for the menu system.
 *
 *	Call syntax:	int_p = menu_alloc(n_bytes);
 *
 *	Parameters:	unsigned int	n_bytes;
 *
 *	Return value:	int *		char_p;
 */

#include "menu.h"


/*
 *	External functions:
 */
extern	char *		calloc();




int *
menu_alloc(n_bytes)
	unsigned int	n_bytes;
{
	int *		int_p;			/* pointer to return */


#ifdef lint
	int_p = NULL;
#else
	int_p = (int *) calloc(n_bytes, sizeof(char));
#endif lint
	if (int_p == NULL) {
		menu_log("Cannot allocate memory for menu system.");
		menu_abort(1);
	}

	return(int_p);
} /* end menu_alloc() */
