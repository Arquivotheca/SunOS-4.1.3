#ifndef lint
static	char sccsid[] = "@(#)confpr.c	1.1 92/07/30	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Device configuration table and interface for standalone I/O system.
 *
 * This version contains no drivers; it uses the boottab in the PROMs.
 */

#include "../lib/stand/saio.h"

/* Dummy device table to satisfy external references */
struct boottab *(devsw[]) = {(struct boottab *)0, };
