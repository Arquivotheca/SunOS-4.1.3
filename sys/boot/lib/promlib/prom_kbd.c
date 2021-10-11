/*	@(#)prom_kbd.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

prom_stdin_is_keyboard()
{
	int i;
        extern phandle_t prom_getphandle();

	switch (obp_romvec_version)  {
	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		return (OBP_V0_INSOURCE == INKEYB);
	default:
		i = prom_getproplen((dnode_t) prom_getphandle(OBP_V2_STDIN), "keyboard");
		return (i == -1 ? 0 : 1);
	}
}
