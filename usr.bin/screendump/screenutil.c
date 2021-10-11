#ifndef lint
static char sccsid[] = "@(#)screenutil.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

#include "screendump.h"
#include <varargs.h>

char *Progname;

char *
basename(path)
	char *path;
{
	register char *p = path, c;

	while (c = *p++)
		if (c == '/')
			path = p;

	return path;
}

/*VARARGS*/
void
error(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	va_start(ap);

	if (fmt = va_arg(ap, char *)) 
		(void) fprintf(stderr, "%s: ", Progname);
	else
		fmt = va_arg(ap, char *);

	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	(void) fprintf(stderr, "\n");

	exit(1);
}

pr_has_overlay(pr)
	Pixrect *pr;
{
	char groups[PIXPG_OVERLAY + 1];

	(void) pr_available_plane_groups(pr, sizeof groups, groups);

	return groups[PIXPG_OVERLAY] && groups[PIXPG_OVERLAY_ENABLE];
}

static char *nomem = "Out of memory";

Pixrect *
xmem_create(w, h, d)
{
	Pixrect *pr;

	if (!(pr = mem_create(w, h, d)))
		error(nomem);

	return pr;
}

char *
xmalloc(n)
	int n;
{
	char *p, *malloc();

	if (!(p = malloc((unsigned) n)))
		error(nomem);

	return p;
}
