#ifndef lint
static	char sccsid[] = "@(#)sony_replay.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_replay_seq(device, sframe, eframe, ntimes, wait_flag)
FILE *device;
unsigned char sframe[];
unsigned char eframe[];
unsigned char ntimes[];
int wait_flag;
{
	register int i;
	unsigned char rc;

	/*
	 * Cant play backwards with CLV disks
	 */
	if(((int)atoi(eframe) < (int)atoi(sframe)) && sony_clv_disk(device) )
		return(CLV_DISK);

	if((rc = sony_search(device,sframe)) != COMPLETION)
		return(rc);
	putc(REPEAT,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	for(i=0; eframe[i] != '\0'; i++) {
		if(i == 5)
			return(NAK);
		putc(eframe[i], device);
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
		sony_settimeout(device,5);
		return(rc);
	} else {	
		rc = sony_enter(device);
		return(rc);
	}
}
