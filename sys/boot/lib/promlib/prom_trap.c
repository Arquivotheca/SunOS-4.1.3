/*	@(#)prom_trap.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * Not for the kernel.  Dummy replacement for kernel montrap.
 */
int
montrap(funcptr)
	int (*funcptr)();
{
	return (*funcptr)();
}

