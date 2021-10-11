#ifndef lint
static char sccsid[] = "@(#)conf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 *
 * Configuration table for standalone I/O system.
 *
 * This file contains the character and block device lists
 * for drivers supported by the boot program. Note that tpboot and copy
 * use a different link order, so that the devsw[] list in
 * the standalone library (libsa.a) supersedes this one.
 */

#include <stand/saio.h>
#include "boot/conf.h"

extern struct boottab ecdriver;
extern struct boottab gndriver;
extern struct boottab iedriver;
extern struct boottab ipidriver;
extern struct boottab ledriver;
extern struct boottab sddriver;
extern struct boottab xddriver;
extern struct boottab xydriver;

/*
 * The device table
 *
 * This table lists all the driver drivers.  It is searched by open()
 * to parse the device specification.
 */
struct boottab *(devsw[]) = {
	&iedriver,
	&ledriver,
	&gndriver,
	&ecdriver,
	(struct boottab *)0,
};

/*
 * Beware: in the following table, the entries must appear
 * in the slot corresponding to their major device number.
 *
 * This is because other routines index into this table
 * using the major() of the dev_t returned by getblockdev.
 */

struct bdevlist bdevlist[] = {
	"", 0, 0, 0,				/* (0, 0) */
	"id", &ipidriver, makedev(1, 0), 1,	/* (1, 0) */
	"", 0, 0, 0,				/* (2, 0) */
	"xy", &xydriver, makedev(3, 0), 0,	/* (3, 0) */
	"", 0, 0, 0,				/* (4, 0) */
	"", 0, 0, 0,				/* (5, 0) */
	"", 0, 0, 0,				/* (6, 0) */
	"sd", &sddriver, makedev(7, 0), 0,	/* (7, 0) */
	"xd", &xddriver, makedev(8, 0), 0,	/* (8, 0) */
	0,
};
