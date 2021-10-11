#ifndef lint
static	char sccsid[] = "@(#)sony_ucodeinq.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

#define	VERS_TWO	0x20	/* Version 2 rom */
#define	POINT_ONE	0x02	/* Version .2 and greater */

void
sony_ucode_inq(device,code)
FILE *device;
unsigned char code[];
{
	register int i;
	unsigned char rom;

	/*
	 * Check rom version, this code is invalid for
	 * versions below 2.2, in the 1550 player
	 */
	sony_rom_version(device,&rom);
	if(!(rom&VERS_TWO)) {
		code[0] = NAK;
		return;
	}
	if(!(rom&POINT_ONE)) {
		code[0] = NAK;
		return;
	}

	putc(USERS_CODE_INQ,device);
	for(i=0; i<4; i++) {
		code[i] = sony_read(device);
		if(code[i] == MARK_RETURN) {
			fprintf(stderr,"sony_ucode_inq : ");
			fprintf(stderr,"MARK_RETURN returned ignored\n");
			code[i] = sony_read(device);
		}
		if(code[0] == NAK)
			return;
	}
	return;
}
