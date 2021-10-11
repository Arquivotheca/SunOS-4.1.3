/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)get_view_surface77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "../libcore/coretypes.h"

int get_view_surface();

#define DEVNAMESIZE 20

extern char **xargv;

int getviewsurface_(surfname)
struct vwsurf *surfname;
	{
	char *sptr;
	int i;

	i = get_view_surface(surfname, xargv);
	sptr = surfname->screenname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->screenname[DEVNAMESIZE])
		*sptr++ = ' ';
	sptr = surfname->windowname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->windowname[DEVNAMESIZE])
		*sptr++ = ' ';
	sptr = surfname->cmapname;
	while (*sptr++)
		;
	--sptr;
	while (sptr < &surfname->cmapname[DEVNAMESIZE])
		*sptr++ = ' ';
	return(i);
	}
