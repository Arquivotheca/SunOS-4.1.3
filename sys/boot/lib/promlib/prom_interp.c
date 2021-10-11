/*	@(#)prom_interp.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_interpret(forthstring)
	char	*forthstring;
{
	OBP_INTERPRET(forthstring);
}
