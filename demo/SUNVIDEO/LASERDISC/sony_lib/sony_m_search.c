#ifndef lint
static	char sccsid[] = "@(#)sony_m_search.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_m_search(device)
FILE *device;
{
	unsigned char rc;

	putc(M_SEARCH,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	sony_blockingread(device);
	rc = sony_handshake(device,COMPLETION);
	sony_settimeout(device, 5);
	return(rc);
}
