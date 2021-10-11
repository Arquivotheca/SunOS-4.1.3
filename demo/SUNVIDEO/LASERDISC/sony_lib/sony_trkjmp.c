#ifndef lint
static	char sccsid[] = "@(#)sony_trkjmp.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_f_track_jump(device,track)
FILE *device;
unsigned char track[];
{
	register int i;
	int trk;
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	trk = atoi(track);
	if((trk < 1) || (trk > 201))
		return(NAK);

	putc(F_TRACK_JUMP,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	for(i=0; track[i] != '\0'; i++) {
		putc(track[i],device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}
	if((rc = sony_enter(device)) != ACK)
		return(rc);
	rc = sony_handshake(device,COMPLETION);
	return(rc);
}

unsigned char
sony_r_track_jump(device,track)
FILE *device;
unsigned char track[];
{
	register int i;
	int trk;
	unsigned char rc;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	trk = atoi(track);
	if((trk < 1) || (trk > 201))
		return(NAK);

	putc(R_TRACK_JUMP,device);
	sony_handshake(device,ACK);
	for(i=0; track[i] != '\0'; i++) {
		putc(track[i],device);
		sony_handshake(device,ACK);
	}
	if((rc = sony_enter(device)) != ACK)
		return(rc);;
	rc = sony_handshake(device,COMPLETION);
	return(rc);
}
