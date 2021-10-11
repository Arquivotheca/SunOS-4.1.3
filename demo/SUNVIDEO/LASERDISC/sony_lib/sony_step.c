#ifndef lint
static	char sccsid[] = "@(#)sony_step.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

unsigned char
sony_f_step(device,speed)
FILE *device;
char speed[];
{
	register int i;
	unsigned char rc;
	int spd;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	spd = atoi(speed);
	if((spd < 1) || (spd > 255))
		return(INVALID_SPEED);

	putc(F_STEP,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	
	for(i=0; speed[i] != '\0'; i++) {
		putc(speed[i],device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}
	rc = sony_enter(device);
	return(rc);
}

unsigned char
sony_r_step(device,speed)
FILE *device;
unsigned char speed[];
{
	register int i;
	unsigned char rc;
	int spd;

	if(sony_clv_disk(device))
		return(CLV_DISK);

	spd = atoi(speed);
	if((spd < 1) || (spd > 255))
		return(INVALID_SPEED);

	putc(R_STEP,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	for(i=0; speed[i] != '\0'; i++) {
		putc(speed[i],device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}
	rc= sony_enter(device);
	return(rc);
}
