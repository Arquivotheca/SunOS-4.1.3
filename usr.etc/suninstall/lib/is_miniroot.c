#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)is_miniroot.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)is_miniroot.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		is_miniroot()
 *
 *	Description:	Are we booted on the MINIROOT?  Returns 1 if
 *		we are and 0 if we are not.
 */

#include <sys/file.h>


int
is_miniroot()
{
	static	int		init_done = 0;	/* initialization done? */
	static	int		value = 0;	/* value to return */


	if (init_done)
		return(value);

	if (access("/.MINIROOT", F_OK) == 0)
		value = 1;

	init_done = 1;

	return(value);
} /* end is_miniroot() */
