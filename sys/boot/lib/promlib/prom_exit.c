/*	@(#)prom_exit.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_exit_to_mon()
{
	montrap(OBP_EXIT_TO_MON);
	/* NOTREACHED */
}
