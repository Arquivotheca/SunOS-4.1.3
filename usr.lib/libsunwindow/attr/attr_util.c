#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)attr_util.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <varargs.h>
#include <sunwindow/attr.h>

/* attr_create_list creates an avlist from the VARARGS passed
 * on the stack.  The storage is always allocated.
 */
/*VARARGS*/
Attr_avlist
attr_create_list(va_alist)
va_dcl
{
    va_list	valist;
    Attr_avlist	avlist;

    va_start(valist);
    avlist = attr_make((char **)0, 0, valist);
    va_end(valist);
    return avlist;
}

/* attr_find searches and avlist for the first occurrence of
 * a specified attribute.
 */
Attr_avlist
attr_find(attrs, attr)
register Attr_avlist	attrs;
register Attr_attribute	attr;
{
    for (; *attrs; attrs = attr_next(attrs)) {
	if (*attrs == (caddr_t)attr) break;
    }
    return(attrs);
}
