#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_int.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_int.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_int.c
 *
 *	Description:	Convert integers into strings and back again.
 */

/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Name:		cv_int_to_str()
 *
 *	Description:	Convert an integer into a string.
 */

char *
cv_int_to_str(int_p)
	int *		int_p;
{
	static	char		buf[80];	/* output buffer */


	(void) sprintf(buf, "%d", *int_p);
	return(buf);
} /* end cv_int_to_str() */




/*
 *	Name:		cv_str_to_int()
 *
 *	Description:	Convert a string into an integer.
 */

int
cv_str_to_int(str, int_p)
	char *		str;
	int *		int_p;
{
	*int_p = atoi(str);

	return(1);
} /* end cv_str_to_int() */
