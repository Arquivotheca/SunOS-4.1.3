#ifndef lint
static	char sccsid[] = "@(#)sony_repeat.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_repeat(device, frame, ntimes, wait_flag)
FILE *device;
unsigned char frame[];
unsigned char ntimes[];
int wait_flag;
{
	register int i;
	unsigned char rc;
	unsigned char address[5];

	sony_addr_inq(device,address);

	/*
	 * Cant play backwards with CLV disks
	 */
	if( (atoi(frame) < atoi(address)) && sony_clv_disk(device) )
		return(CLV_DISK);

	putc(REPEAT,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	for(i=0; frame[i] != '\0'; i++) {
		if(i == 5)
			return(NAK);
		putc(frame[i], device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}

	if((rc = sony_enter(device)) != ACK)
		return(rc);

	for(i=0; ntimes[i] != '\0'; i++) {
		putc(ntimes[i], device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}

	if(wait_flag) {
		sony_blockingread(device);
		if((rc = sony_enter(device,ACK)) != ACK)
			return(rc);
		rc = sony_handshake(device,COMPLETION);
		sony_settimeout(device, 5);
		return(rc);
	} else {	
		rc = sony_enter(device);
		return(rc);
	}
}
