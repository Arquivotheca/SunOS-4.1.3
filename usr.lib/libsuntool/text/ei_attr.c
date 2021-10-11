#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ei_attr.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Attribute support for entity interpreters.
 */

#include <varargs.h>
#include <sys/types.h>
#include <sunwindow/attr.h>
#include <suntool/primal.h>

#include <suntool/entity_interpreter.h>

/* VARARGS1 */
extern int
ei_set(eih, va_alist)
	register Ei_handle	eih;
	va_dcl
{
	caddr_t			attr_argv[ATTR_STANDARD_SIZE];
	va_list			args;

	va_start(args);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	return(eih->ops->set(eih, attr_argv));
}
