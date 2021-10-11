#ifndef lint
static char sccsid[] = "@(#)error.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <varargs.h>

static char *progname;

setprogname(name)
	char *name;
{
	register char *p = name, c;

	if (p)
		while (c = *p++)
			if (c == '/')
				name = p;

	progname = name;
}

/* _error([no_perror, ] fmt [, arg ...]) */
/*VARARGS*/
int
_error(va_alist)
va_dcl
{
	int saved_errno;
	va_list ap;
	int no_perror = 0;
	char *fmt;
	extern int errno;

	saved_errno = errno;

	if (progname)
		(void) fprintf(stderr, "%s: ", progname);

	va_start(ap);
	if ((fmt = va_arg(ap, char *)) == 0) {
		no_perror = 1;
		fmt = va_arg(ap, char *);
	}
	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	if (no_perror)
		(void) fprintf(stderr, "\n");
	else {
		(void) fprintf(stderr, ": ");
		errno = saved_errno;
		perror("");
	}

	return 1;
}
