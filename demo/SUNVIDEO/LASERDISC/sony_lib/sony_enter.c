#ifndef lint
static	char sccsid[] = "@(#)sony_enter.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_enter(device)
FILE *device;
{
	unsigned char rc;

	putc(ENTER,device);
	/*
	 * Return the error code received by sony_handshake
	 * if one occurs
	 */
	rc = sony_handshake(device,ACK);
	return(rc);
}
