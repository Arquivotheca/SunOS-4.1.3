#ifndef lint
static	char sccsid[] = "@(#)sony_hshake.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*
 * Check for the handshake code
 */
#include <stdio.h>
#include <fcntl.h>
#include <sys/filio.h>
#include "sony_codes.h"

unsigned char
sony_handshake(device,ack_char)
FILE *device;
unsigned char ack_char;
{
	unsigned char ack;

	/*
	 * Read back a character from the Laser Disk
	 * player, verifying that it is the one requested.
	 */
	if((ack = sony_read(device)) == ack_char) 
		return(ack);
	switch(ack) {
	case ERROR :	/*  RS232 Error */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"ERROR returned, check the serial line\n");
		break;
	case NAK :	/* Player rejects the command */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"NAK returned, player rejected last command\n");
		break;
	case NO_FRAME_NUMBER :	/* Frame/Chapter # not in active range */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"NO_FRAME_NUMBER returned, invalid frame");
		fprintf(stderr," specified\n");
		break;
	case NOT_TARGET :	/* Target Frame not Found */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"NOT_TARGET returned, target frame not");
		fprintf(stderr," found\n");
		break;
	case LID_OPEN :		/* The lid is open */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"LID_OPEN returned, the lid is open\n");
		break;
	case MARK_RETURN :	/* Player generated MARK return */
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"MARK_RETURN returned, ignored\n");
		ack = sony_handshake(device, ack_char);
		break;
	defualt :
		fprintf(stderr,"sony_handshake : ");
		fprintf(stderr,"0x%x returned, unknown character\n",ack);
		break;
	}
	return(ack);
}
