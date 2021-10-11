/*	@(#)prom_writestr.c 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>

/*
 * Write string to PROM's notion of stdout.
 */

void
prom_writestr(buf, bufsize)
	char *buf;
	u_int bufsize;
{
	u_int written = 0;
	int i;

	switch (obp_romvec_version)  {

	case OBP_V0_ROMVEC_VERSION:
	case OBP_V2_ROMVEC_VERSION:
		(OBP_V0_FWRITESTR)(buf, bufsize);
		return;

	default:
		while (written < bufsize)  {
		    i =  OBP_V2_WRITE(OBP_V2_STDOUT, buf, bufsize - written);
		    if (i == -1)
			continue;
		    written += i;
		}
		return;
	}
}
