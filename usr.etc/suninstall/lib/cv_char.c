#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_char.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_char.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_char.c
 *
 *	Description:	Routines for converting single characters into
 *		strings and back again.
 */

#define NULL		0


/*
 *	Name:		cv_char_to_str()
 *
 *	Description:	Convert a single char into a string.
 */

char *
cv_char_to_str(char_p)
	char *		char_p;
{
	static	char		buf[2];		/* output buffer */


	buf[0] = *char_p;
	buf[1] = NULL;

	return(buf);
} /* end cv_char_to_str() */




/*
 *	Name:		cv_str_to_char()
 *
 *	Description:	Convert a string into a single char.
 */

int
cv_str_to_char(str, char_p)
	char *		str;
	char *		char_p;
{
	*char_p = *str;

	return(1);
} /* end cv_str_to_char() */
