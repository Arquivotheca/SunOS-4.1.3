#ifndef lint
static	char	sccsid[] = "@(#)ckf_uint.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_uint.c
 *
 *	Description:	Determine if a field is an unsigned integer.  If the
 *		field is a valid unsigned integer, then one is returned.
 *		Otherwise, zero is returned.
 *
 *	Call syntax:	ret_code = ckf_uint(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include <string.h>
#include "menu.h"


int
ckf_uint(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
	extern	int		atoi();


#ifdef lint
	arg_p = arg_p;
#endif

	if (strchr(field_p, '-') || (atoi(field_p) == 0 && *field_p != '0')) {
		menu_mesg("Field is not a valid unsigned integer.");
		return(0);
	}

	return(1);
} /* end ckf_uint() */
