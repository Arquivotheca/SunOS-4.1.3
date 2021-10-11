#ifndef lint
static	char sccsid[] = "@(#)sony_chinq.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

sony_chapter_inq(device,chapter)
FILE *device;
unsigned char chapter[];
{
	putc(CHAPTER_INQ,device);
	chapter[0] = sony_read(device);

	/*
	 * MARK_RETURN can occur at any time ignore it
	 */
	if(chapter[0] == MARK_RETURN) {
		fprintf(stderr,"sony_chapter_inq : ");
		fprintf(stderr,"MARK_RETURN returned, ignored\n");
		chapter[0] = sony_read(device);
	}
	if(chapter[0] == NAK)
		return;
	chapter[1] = sony_read(device);
	if(chapter[1] == MARK_RETURN) {
		fprintf(stderr,"sony_chapter_inq : ");
		fprintf(stderr,"MARK_RETURN returned, ignored\n");
		chapter[1] = sony_read(device);
	}
	return;
}
