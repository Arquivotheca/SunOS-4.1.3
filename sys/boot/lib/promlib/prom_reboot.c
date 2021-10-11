/*	@(#)prom_reboot.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_reboot(s)
	char *s;
{
	OBP_BOOT(s);
	/* NOTREACHED */
}
