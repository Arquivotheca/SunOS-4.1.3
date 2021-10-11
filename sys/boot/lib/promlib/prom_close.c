/*	@(#)prom_close.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

int
prom_close(fd)
	int	fd;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((int)(OBP_V0_CLOSE(fd)));

	default:
		return (int)(OBP_V2_CLOSE((ihandle_t)fd));
	}
}
