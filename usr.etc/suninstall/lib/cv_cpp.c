#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_cpp.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_cpp.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_cpp.c
 *
 *	Description:	Convert a string into a pointer to copy of that
 *		string and back again.
 */

#include <string.h>


#define NULL	0


extern	char *		malloc();


/*
 *	Name:		cv_cpp_to_str()
 *
 *	Description:	Convert a pointer to a string into a string.
 */

char *
cv_cpp_to_str(cpp)
	char **		cpp;
{
	return(*cpp);
} /* end cv_cpp_to_str() */




/*
 *	Name:		cv_str_to_cpp()
 *
 *	Description:	Convert a string into a pointer to a copy of the
 *		string.  Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_cpp(str, cpp)
	char *		str;
	char **		cpp;
{
						/* get a new buffer */
	*cpp = malloc((unsigned int) strlen(str) + 1);
	if (*cpp == NULL)
		return(0);

	(void) strcpy(*cpp, str);

	return(1);
} /* end cv_str_to_cpp() */
