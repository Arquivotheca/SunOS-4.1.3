#ifndef lint
static	char sccsid[] = "@(#)sony_eject.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_eject(device)
FILE *device;
{
	unsigned char rc;

	sony_blockingread(device);
	putc(EJECT,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	rc = sony_handshake(device,LID_OPEN);
	sony_settimeout(device,5);
	return(rc);
}
