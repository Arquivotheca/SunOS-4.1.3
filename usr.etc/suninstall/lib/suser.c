#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)suser.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)suser.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		suser()
 *
 *	Description:	Return 1 is the caller is the super-user and 0
 *		otherwise.
 */

int
suser()
{
#ifdef TEST_JIG
	return(1);
#else
	return(getuid() == 0 ? 1 : 0);
#endif
} /* end suser() */
