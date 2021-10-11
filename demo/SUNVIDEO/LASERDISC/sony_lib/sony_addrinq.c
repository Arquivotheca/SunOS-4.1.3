#ifndef lint
static	char sccsid[] = "@(#)sony_addrinq.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

void
sony_addr_inq(device,address)
FILE *device;
unsigned char address[];
{
	register int i;

	putc(ADDR_INQ,device);
	for(i=0; i<5; i++) {
		address[i] = sony_read(device);
		/*
		 * MARK_RETURN can occur at any time ignore it
		 */
		if(address[i] == MARK_RETURN) {
			fprintf(stderr,"sony_addr_inq : ");
			fprintf(stderr,"MARK_RETURN returned, ignored\n");
			address[i] = sony_read(device);
		}
		if(address[0] == NAK)
			return;
	}

	/*
	 * If it is a CLV disk and no extended time code
	 * null out the seconds characters.
	 */
	if(sony_clv_disk(device)) {
		if( (address[3] == '9') && (address[4] = '9') ) {
			address[3] = '\0';
			address[4] = '\0';
		}
	}
	return;
}
