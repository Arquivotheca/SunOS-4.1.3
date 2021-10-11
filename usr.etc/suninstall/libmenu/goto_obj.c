#ifndef lint
static	char	sccsid[] = "@(#)goto_obj.c 1.1 92/07/30";
#endif

/*
 *	Name:		goto_obj.c
 *
 *	Description:	Return a value that indicates we are going to another
 *		object and the status is okay.
 *
 *	Call syntax:	return(goto_obj());
 */

#include "menu.h"
#include "menu_impl.h"


int
menu_goto_obj()
{
	return(MENU_GOTO_OBJ);
} /* end menu_goto_obj() */
