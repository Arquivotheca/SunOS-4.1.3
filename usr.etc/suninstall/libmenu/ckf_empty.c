#ifndef lint
static	char	sccsid[] = "@(#)ckf_empty.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_empty.c
 *
 *	Description:	Determine if a field is empty.  If the field is
 *		empty, then zero is returned.  Otherwise, one is returned.
 *
 *	Call syntax:	ret_code = ckf_empty(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"


int
ckf_empty(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
#ifdef lint
	arg_p = arg_p;
#endif

	if (strlen(field_p) == 0) {
		menu_mesg("Field must be non-empty.");
		return(0);
	}

	return(1);
} /* end ckf_empty() */
