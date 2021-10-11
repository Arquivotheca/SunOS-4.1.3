#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)err_mesg.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)err_mesg.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		err_mesg()
 *
 *	Description:	Return the error message associated with
 *		system error 'code'.
 */

#include <errno.h>


/*
 *	External variables:
 */
extern	char *		sys_errlist[];


char *
err_mesg(code)
	int		code;
{
	return(sys_errlist[code]);
} /* end err_mesg() */
