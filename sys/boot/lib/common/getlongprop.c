#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)getlongprop.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988-1990 Sun Microsystems, Inc.
 *
 * Support routine for the Open Boot Proms.
 * Look up a property by name, and fetch its value.
 * This version of getprop fetches an arbitrarily long property
 * into a newly allocated buffer. It uses kmem_alloc, and is
 * unsuitable for bootblocks and other programs without malloc().
 * See getprop.c.
 */

#include <sys/param.h>
#include <mon/sunromvec.h>

#ifdef KADB

#define	kmem_alloc(x)	malloc(x)
#define	_stop(s)	{ printf(s); exit(-1); }

#endif KADB

char *
getlongprop(id, name)
	int id;
	char *name;
{
	register int len;
	register char *cp;

	len = prom_getproplen(id, name);
	if (len <= 0)
		return ((char *)len);
	cp = (char *)kmem_alloc(len);
	if (cp == 0)
		_stop("couldn't allocate space for property");
	(void)prom_getprop(id, name, cp);
	return (cp);
}

#ifdef NOT
static void
__foo_()
{
	splnet();		/* Don't even ask... */
}
#endif
