#ifndef lint
static	char	sccsid[] = "@(#)ckf_netpath.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_netpath.c
 *
 *	Description:	Determine if a field is a network path.  If
 *		the field is not a network path, then zero is returned.
 *		Otherwise, one is returned.
 *
 *	Call syntax:	ret_code = ckf_netpath(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include <string.h>
#include "menu.h"


int
ckf_netpath(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
	char *		cp;			/* scratch char pointer */


#ifdef lint
	arg_p = arg_p;
#endif

	if (field_p[0] == '/')
		return(1);

	cp = strchr(field_p, ':');
	if (cp && cp > field_p)
		return(1);

	menu_mesg(
	       "Field must be a network path (begin with a '/' or '<sys>:').");

	return(0);
} /* end ckf_netpath() */
