#ifndef lint
static	char	sccsid[] = "@(#)ckf_abspath.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_abspath.c
 *
 *	Description:	Determine if a field is an absolute path.  If
 *		the field is not an absolute path, then zero is returned.
 *		Otherwise, one is returned.
 *
 *	Call syntax:	ret_code = ckf_abspath(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"


int
ckf_abspath(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
#ifdef lint
	arg_p = arg_p;
#endif

	if (field_p[0] != '/') {
		menu_mesg("Field must be an absolute path (begin with a '/').");
		return(0);
	}

	return(1);
} /* end ckf_abspath() */
