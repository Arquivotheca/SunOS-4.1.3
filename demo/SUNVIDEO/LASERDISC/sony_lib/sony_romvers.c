#ifndef lint
static	char sccsid[] = "@(#)sony_romvers.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

void
sony_rom_version(device,version)
FILE *device;
unsigned char *version;
{
	putc(ROM_VERSION,device);
	*version = sony_read(device);
	if(*version == MARK_RETURN) {
		fprintf(stderr,"sony_rom_version : ");
		fprintf(stderr,"MARK_RETURN returned, ignored\n");
		*version = sony_read(device);
	}
		
	return;
}
