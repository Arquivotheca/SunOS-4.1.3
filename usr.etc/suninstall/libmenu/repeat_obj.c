#ifndef lint
static	char	sccsid[] = "@(#)repeat_obj.c 1.1 92/07/30";
#endif

/*
 *	Name:		repeat_obj.c
 *
 *	Description:	Return a value that causes the object to be repeated.
 *
 *	Call syntax:	return(menu_repeat_obj());
 */

#include "menu.h"
#include "menu_impl.h"


int
menu_repeat_obj()
{
	return(MENU_REPEAT_OBJ);
} /* end menu_repeat_obj() */
