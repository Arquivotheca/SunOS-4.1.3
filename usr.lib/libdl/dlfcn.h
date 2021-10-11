/* @(#)dlfcn.h 1.1 92/07/30 SMI */

#ifndef	_dlfcn_h
#define	_dlfcn_h

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 * Interface description for the contents of libdl -- simple programmer's
 * interfaces to the dynamic linker.
 */

/*
 * Manifest constants
 */
#define	RTLD_LAZY	1		/* deferred binding of procedures */

/*
 * Function declarations
 */
extern	void *dlopen();			/* open and map a shared object */
extern	void *dlsym();			/* obtain address of symbol */
extern	int dlclose();			/* remove a shared object */
extern	char *dlerror();		/* string representing last error */

#endif /* !_dlfcn_h */
