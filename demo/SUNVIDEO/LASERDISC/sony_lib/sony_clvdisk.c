#ifndef lint
static	char sccsid[] = "@(#)sony_clvdisk.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "sony_codes.h"

int
sony_clv_disk(device)
FILE *device;
{
	register int i;
	unsigned char disk_type[5];

	sony_status_inq(device,disk_type);

	if(disk_type[2]&CLV_DISK)
		return(1);
	else
		return(0);
}
