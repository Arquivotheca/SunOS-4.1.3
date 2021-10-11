#if !defined(lint) && !defined(BOOTBLOCK)
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
 *
 * The open boot proms initialize these tables at run time, not statically,
 * so they're just empty slots. Be sure to leave enough room for all
 * possible devices; these really ought to be linked lists, but you know...
 */

#include <sys/types.h>
#include "boot/conf.h"

/*
 * The device table
 *
 * This table lists all the nfs devices.  It is searched by
 * etherinit() to parse the device specification.
 */

struct boottab *(devsw[8]) = {
	(struct boottab *)0,
};

/*
 * This table lists all the ufs devices.
 *
 * Beware: in the following table, the entries must appear
 * in the slot corresponding to their major device number.
 * This is because other routines index into this table
 * using the major() of the dev_t returned by getblockdev.
 */

struct bdevlist bdevlist[8] = {
	"", 0, 0,
};

struct ndevlist ndevlist[8] = {
	0, 0, 
};

