#ifndef lint
static	char sccsid[] = "@(#)conf.c	1.1 92/07/30	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Configuration table for standalone I/O system.
 *
 * This table lists all the supported drivers.  It is searched by open()
 * to parse the device specification.
 */

#include <stand/saio.h>

extern struct boottab xydriver;
extern struct boottab sddriver;
extern struct boottab xddriver;
extern struct boottab stdriver;
extern struct boottab mtdriver;
extern struct boottab xtdriver;
#ifdef sun2
extern struct boottab ardriver;
#endif

/*
 * The device table 
 */
struct boottab *(devsw[]) = {
	&xydriver,
	&sddriver,
	&xddriver,
	&stdriver,
	&mtdriver,
	&xtdriver,
#ifdef sun2
	&ardriver,
#endif
	(struct boottab *)0,
};
