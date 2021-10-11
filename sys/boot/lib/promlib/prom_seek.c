/*	@(#)prom_seek.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_seek(fd, low, high)
	int	fd, low, high;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return (int)(OBP_V0_SEEK(fd, low, high));

	default:
		return (OBP_V2_SEEK((ihandle_t)fd, (u_int)low, (u_int)high));
	}
}
