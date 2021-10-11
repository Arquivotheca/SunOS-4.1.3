#ifndef lint
static	char sccsid[] = "@(#)sony_statinq.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

void
sony_status_inq(device,status)
FILE *device;
unsigned char status[];
{
	register int i;

	putc(STATUS_INQ,device);
	for(i=0; i<5; i++) {
		status[i] = sony_read(device);
		/*
		 * MARK_RETURN can occur at any time ignore it
		 */
		if(status[i] == MARK_RETURN) {
			fprintf(stderr,"sony_status_inq : ");
			fprintf(stderr,"MARK_RETURN returned, ignored\n");
			status[i] = sony_read(device);
		}
	}
	return;
}
