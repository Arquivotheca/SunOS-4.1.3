#ifndef lint
static	char	sccsid[] = "@(#)lex_no_ws.c 1.1 92/07/30";
#endif

/*
 *	Name:		lex_no_ws.c
 *
 *	Description:	Determine if the current input to a field, 'ch',
 *		is not whitespace.  If the input is not whitespace, then
 *		one is returned.  Otherwise, zero is returned.
 *
 *	Call syntax:	ret_code = lex_no_ws(start_p, end_p, ch);
 *
 *	Parameters:	char *		start_p;
 *			char *		end_p;
 *			char		ch;
 *
 *	Return value:	int		ret_code;
 */

#include <ctype.h>
#include "menu.h"


int
lex_no_ws(start_p, end_p, ch)
	char *		start_p;
	char *		end_p;
	char		ch;
{
#ifdef lint
	start_p = start_p;
	end_p = end_p;
#endif lint

	return((isprint(ch) && !isspace(ch)) ? 1 : 0);
} /* end lex_no_ws() */








