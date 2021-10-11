#ifndef lint
static	char	sccsid[] = "@(#)ckf_inet_addr.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_inet_addr.c
 *
 *	Description:	Determine if a field is an internet address.
 *		If the field is an internet address, then one is returned.
 *		Otherwise, zero is returned.
 *
 *	Call syntax:	ret_code = ckf_inet_addr(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include "menu.h"

extern	unsigned long		inet_addr();


int
ckf_inet_addr(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
#ifdef lint
	arg_p = arg_p;
#endif

	if (strlen(field_p) == 0 || inet_addr(field_p) == -1L) {
		menu_mesg("Field is not a valid Internet address.");
		return(0);
	}

	return(1);
} /* end ckf_inet_addr() */
