/*	@(#)prom_enter.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_enter_mon()
{
	(void)montrap(OBP_ENTER_MON);
}
