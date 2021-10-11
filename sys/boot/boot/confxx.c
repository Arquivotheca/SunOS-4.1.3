/*
 * @(#)confxx.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Device configuration table and interface for standalone I/O system.
 *
 * This version contains exactly one driver; it is used in the boot block
 * (first 15 sectors after label) of disk drives.
 */

#include "../lib/stand/saio.h"

/*
 * Throughout this module, "xx" is replaced by the name
 * of the particular device being booted from.
 */
extern struct boottab xxdriver;

struct boottab *(devsw[]) = {
	&xxdriver,
	(struct boottab *)0,
};

