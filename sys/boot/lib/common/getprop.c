#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)getprop.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988-1990 Sun Microsystems, Inc.
 *
 * Support routine for the Open Boot Proms.
 * Look up a property by name, and fetch its value.
 * This version of getprop fetches an int-length property
 * into the space at *ip. It returns the length of the
 * requested property; the caller should test that the
 * return value == sizeof (int) for success.
 * A non-int-length property can be fetched by getlongprop(), q.v.
 */

#include <sys/param.h>
#include <mon/sunromvec.h>

int
getprop(id, name, ip)
	int id;
	char *name;
	int *ip;
{
	register int len;
	register char *cp;

	len = prom_getproplen(id, name);
	if (len == sizeof (int) && ip != (int *)0)
		(void)prom_getprop(id, name, (char *)ip);
	return (len);
}
