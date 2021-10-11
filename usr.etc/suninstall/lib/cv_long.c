#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_long.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_long.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_long.c
 *
 *	Description:	Convert a long into a string and back again.
 */

/*
 *	External functions:
 */
extern	char *		sprintf();


/*
 *	Name:		cv_long_to_str()
 *
 *	Description:	Convert a long into a string.
 */

char *
cv_long_to_str(long_p)
	long *		long_p;
{
	static	char		buf[80];	/* output buffer */


	(void) sprintf(buf, "%ld", *long_p);
	return(buf);
} /* end cv_long_to_str() */




/*
 *	Name:		cv_str_to_long()
 *
 *	Description:	Convert a string into a long.
 */

int
cv_str_to_long(str, long_p)
	char *		str;
	long *		long_p;
{
	extern	long		atol();


	*long_p = atol(str);

	return(1);
} /* end cv_str_to_long() */
