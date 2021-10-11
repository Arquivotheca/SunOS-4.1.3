/*	@(#)prom_free.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

void
prom_free(virt, size)
	caddr_t	virt;
	u_int	size;
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return;			/* XXX */

	default:
		(OBP_V2_FREE(virt, size));

	}
}
