#ifndef lint
static	char	sccsid[] = "@(#)ignore_obj.c 1.1 92/07/30";
#endif

/*
 *	Name:		ignore_obj.c
 *
 *	Description:	Return a value that causes the object to be ignored.
 *
 *	Call syntax:	return(menu_ignore_obj());
 */

#include "menu.h"
#include "menu_impl.h"


int
menu_ignore_obj()
{
	return(MENU_IGNORE_OBJ);
} /* end menu_ignore_obj() */
