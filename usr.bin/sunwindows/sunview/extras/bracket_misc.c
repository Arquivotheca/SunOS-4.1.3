#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)bracket_misc.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 *	Utility routines for {insert,remove}_brackets
 */
#include <stdio.h>
#include <ctype.h>
#include <strings.h>

char	octal_char();

/*
 * 	process_string(s) -- returns string s with K&R backslash conventions
 *	applied.  Note that s is not overwritten; a new string is returned.
 */
char *
process_string(source)
	char	*source;
{
	char	*dest, *malloc();
	char	*t, *u;

	dest = malloc((unsigned)(strlen(source) + 1));

	for (t = source, u = dest; *t ; t++, u++ ) {
		/* The following treatment of backslashed characters is due to
		 * K&R, p. 180-1.
		 */
		if (*t == '\\')
			switch (*(++t)) {
			case 0:
				t--;
				break;
			case 'n':
				*u = '\n';
				break;
			case 't':
				*u = '\t';
				break;
			case 'b':
				*u = '\b';
				break;
			case 'r':
				*u = '\r';
				break;
			case 'f':
				*u = '\f';
				break;
			case '\\':
				*u = '\\';
				break;
			case '\'':
				*u = '\'';
				break;
			case '0':  case '1':  case '2':  case '3':  case '4':  
			case '5':  case '6':  case '7': 
				*u = octal_char(&t);
				t--;
				break;
			default:
				*u = *t;
				break;
			}
		else
			*u = *t;

	}

	*u = '\0';
	return (dest);

}

/*
 *	octal_char(s) -- return the char value represented by the string
 *		pointer pointed to by s.  The double indirection is used to 
 *		consume characters in the string.  Note that on return the
 *		string pointer points one beyond the end of the octal sequence.
 *		An octal sequence is defined by K&R p. 180-1; it is 1 to 3
 *		octal digits.
 */
char
octal_char(str)
	char	**str;
{
	register int	i;
	char	v;

	v = 0;
	for (i = 0; i < 3 && isdigit(**str) && **str <= '7'; i++, (*str)++)
		v = 8 * v + (**str - '0');

	return (v);

}
