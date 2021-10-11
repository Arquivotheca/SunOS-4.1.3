#ifndef lint
static	char sccsid[] = "@(#)sony_mark.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sys/types.h>
#include "sony_codes.h"

u_char
sony_mark_set(device,frame)
FILE *device;
u_char frame[];
{
	unsigned char rc;
	register int i;

	putc(MARK_SET,device);
	if((rc = sony_handshake(device,ACK)) != ACK)
		return(rc);
	for(i=0; frame[i] != '\0'; i++) {
		if(i == 5)
			return(NAK);
		putc(frame[i],device);
		if((rc = sony_handshake(device,ACK)) != ACK)
			return(rc);
	}
	rc = sony_enter(device);
	return(rc);
}
