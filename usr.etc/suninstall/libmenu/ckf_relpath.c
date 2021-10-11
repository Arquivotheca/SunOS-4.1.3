#ifndef lint
static	char	sccsid[] = "@(#)ckf_relpath.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_relpath.c
 *
 *	Description:	Determine if a field is a relative path.  If
 *		the field is not a relative path, then 0 is returned.
 *		Otherwise, one is returned.
 *
 *	Call syntax:	ret_code = ckf_relpath(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"


int
ckf_relpath(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
#ifdef lint
	arg_p = arg_p;
#endif

	if (field_p[0] == '/') {
		menu_mesg(
		 "Field must be a relative path (does not begin with a '/').");
		return(0);
	}

	return(1);
} /* end ckf_relpath() */
