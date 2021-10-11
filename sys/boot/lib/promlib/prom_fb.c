/*	@(#)prom_fb.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_stdout_is_framebuffer()
{
        extern phandle_t prom_getphandle();
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		return (OBP_V0_OUTSINK == OUTSCREEN);
	default:
		return (prom_devicetype((dnode_t) prom_getphandle(OBP_V2_STDOUT),
					OBP_DISPLAY));
	}
}
