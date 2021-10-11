#ifndef lint
static	char sccsid[] = "@(#)conf.c	1.1 92/07/30	SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Configuration table for standalone I/O system.
 *
 * This file contains the device lists for drivers supported by the boot
 * program.  It is searched by open() to parse the device specification.
 * Note that this devsw[] list appears in the standalone library
 * (libsa.a), so programs linked with libsa before libboot (such as
 * tpboot, copy, and the bootblocks) will use this copy of the table.
 * 
 * The open boot proms initialize these tables at run time, not
 * statically, so they're just empty slots. Be sure to leave enough room
 * for all possible devices; these really ought to be linked lists, but
 * you know...
 * 
 * The bootblocks don't initialize the table at all, so the null first
 * entry means "use the prom driver you rode in on."
 */

#include <sys/types.h>
#include <stand/saio.h>

struct boottab *(devsw[8]) = {
	(struct boottab *)0,
};
