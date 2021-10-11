/* @(#)dl.c 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 * Stub library for programmer's interface to the dynamic linker.  Used
 * to satisfy ld processing, and serves as a precedence place-holder at
 * execution-time.  If these routines are actually called, it is considered
 * a program-terminating error -- the dynamic linker has gotten very confused.
 */

#include <stdio.h>

static	char error[] = "%s: stub interception failed\n";

/*ARGSUSED*/
void *
dlopen(file, mode)
	char *file;			/* file to be added */
	int mode;			/* mode of addition */
{

	fprintf(stderr, error, "dlopen");
	abort();
	/*NOTREACHED*/
}

/*ARGSUSED*/
void *
dlsym(dl, cp)
	void *dl;			/* object to be searched */
	char *cp;			/* symbol to be retrieved */
{

	fprintf(stderr, error, "dlsym");
	abort();
	/*NOTREACHED*/
}

/*ARGSUSED*/
int
dlclose(dl)
	void *dl;			/* object to be removed */
{

	fprintf(stderr, error, "dlclose");
	abort();
	/*NOTREACHED*/
}

char *
dlerror()
{

	fprintf(stderr, error, "dlerror");
	abort();
	/*NOTREACHED*/
}
