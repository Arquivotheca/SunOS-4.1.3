#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)get_stdin.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)get_stdin.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_stdin()
 *
 *	Description:	Read a line of input from stdin and call
 *		delete_blanks() to clean it up.  This implementation
 *		assumes a buffer size of BUFSIZ bytes.
 */

#include <stdio.h>
#include "install.h"


void
get_stdin(buf)
	char buf[BUFSIZ];
{
	bzero(buf, BUFSIZ);
        (void) read(0, buf, BUFSIZ);
	delete_blanks(buf);
} /* end get_stdin() */
