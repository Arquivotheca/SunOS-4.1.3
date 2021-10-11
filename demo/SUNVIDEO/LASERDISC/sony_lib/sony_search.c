#ifndef lint
static	char sccsid[] = "@(#)sony_search.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_search(device,frame)
FILE *device;
unsigned char frame[];
{
	register int i;
	unsigned char rc;

	putc(SEARCH,device);
	if((rc =sony_handshake(device,ACK)) != ACK)
		return(rc);

	for(i=0; frame[i] != '\0'; i++) {
		if(i == 5)
			return(NAK);
		putc(frame[i], device);
		if((rc =sony_handshake(device,ACK)) != ACK)
			return(rc);
	}
	if((rc = sony_enter(device)) != ACK)
		return(rc);

	sony_blockingread(device);
	rc = sony_handshake(device,COMPLETION);
	sony_settimeout(device, 5);
	return(rc);
}
