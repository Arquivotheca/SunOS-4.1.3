#ifndef lint
static	char	sccsid[] = "@(#) ckf_octal.c 1.1@(#)ckf_octal.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_octal.c
 *
 *	Description:	Determine if a field is a octal integer.  If the
 *		field is a valid octal integer, then one is returned.
 *		Otherwise, zero is returned.
 *
 *	Call syntax:	ret_code = ckf_octal(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"


int
ckf_octal(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
	int		dummy;			/* scratch variable */


#ifdef lint
	arg_p = arg_p;
#endif

	if (sscanf(field_p, "%o", &dummy) != 1) {
		menu_mesg("Field is not a valid octal integer.");
		return(0);
	}

	return(1);
} /* end ckf_octal() */
