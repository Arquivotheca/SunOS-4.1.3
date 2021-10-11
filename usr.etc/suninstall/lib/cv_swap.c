#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_swap.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_swap.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include "install.h"
#include "menu.h"
#include <ctype.h>


/*
 *	External functions:
 */
extern	long		atol();


/****************************************************************************
** Function : (long) cv_swap_to_long()
**
** Return value : the converted number > 0
**		  a negative number if the number can't be converted
**			properly. 
**
** Purpose : 	convert thestring pointed to by  the cs and converts it
**		to a valid long integer.  If no units are specified,
**		then we assume bytes.
**
** Return Value : size in bytes of string converted.
**		  -1 : if the number can't be converted or is 0.
**
*****************************************************************************
*/
long
cv_swap_to_long(cp)
	register char  *cp;
{
	long		size = 0L;

	if (cp == CP_NULL)
		return(-1);

	while (isdigit(*cp))
		size = size * 10 + *cp++ - '0';

	/*
	**	If the units is not the last element in the string, it's an
	**	error
	*/
	for (;;)  {
		switch (*cp++) {
		case 'k':
		case 'K':
			if (*cp != '\0')
				break;
			size *= 0x400;
			continue;
		case 'm':
		case 'M':
			if (*cp != '\0')
				break;
			size *= MEGABYTE;
			continue;

		case 'b':
		case 'B':
			if (*cp != '\0')
				break;
			size *= 0x200;
			continue;
		case '\0':
			if (size <= 0L)  {
				menu_mesg("%s: swap size %D is out of range\n",
					       progname, size);
				return (-1L);
			}
			return(size);
		default:
			menu_mesg("%s: invalid character '%c' in swap size\n",
				progname, cp[-1]);
			return(-1L);
		}

		break; /* error hit in switch */
	}
	menu_mesg("%s: extra characters in in swap size\n",  progname);
	return(-1L);
	
	/* NOTREACHED */
}
