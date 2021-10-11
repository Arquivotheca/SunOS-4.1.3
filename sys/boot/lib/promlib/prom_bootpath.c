/*	@(#)prom_bootpath.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

char *
prom_bootpath()
{
	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
		return ((char *)0);

	default:
		{
#ifdef	DPRINTF
			char *p = OBP_V2_BOOTPATH;
			DPRINTF("prom_bootpath: <%s>\n", p ? p : "unknown!");
#endif	DPRINTF
		}
		return (OBP_V2_BOOTPATH);
	}
}
