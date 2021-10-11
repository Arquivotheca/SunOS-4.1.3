#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)chk_swap.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)chk_swap.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include "menu.h"
#include <ctype.h>

extern long	cv_swap_to_long();

/****************************************************************************
**
**	Name:		chk_swap()
**
**	Description:	Determine if a field is a valid swap size.
**
**	Call syntax:	ret_code = chk_swap(arg_p, field_p);
**
**	Parameters:	pointer		arg_p;
**			char *		field_p;
**
**	Return value:	1 : if number can be converted
**			0 : if number can't be converted
**
****************************************************************************
*/
int
chk_swap(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{

#ifdef lint
	arg_p = arg_p;
#endif

	if (cv_swap_to_long(field_p) <= 0L)
		return(0);

	return(1);
} /* end chk_swap() */

