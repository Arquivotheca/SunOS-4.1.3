#ifndef lint
static	char	sccsid[] = "@(#)ckf_long.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_long.c
 *
 *	Description:	Determine if a field is a long integer.  If the field
 *		is a valid long integer, then one is returned.  Otherwise,
 *		zero is returned.
 *
 *	Call syntax:	ret_code = ckf_long(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"


int
ckf_long(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
	extern	long		atol();


#ifdef lint
	arg_p = arg_p;
#endif

	if (atol(field_p) == 0 && *field_p != '0') {
		menu_mesg("Field is not a valid long integer.");
		return(0);
	}

	return(1);
} /* end ckf_long() */
